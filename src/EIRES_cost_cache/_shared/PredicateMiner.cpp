#include "PredicateMiner.h"

#include <assert.h>
#include <algorithm>
#include <iostream>
#include <thread>

using namespace std;


PredicateMiner::PredicateMiner(const QueryLoader& _queryLoader, const Query& _query)
{
	m_NumAttributes.resize(_queryLoader.numEventDecls());
	for (size_t e = 0; e < m_NumAttributes.size(); ++e)
		m_NumAttributes[e] = (uint8_t)_queryLoader.eventDecl(e)->attributes.size();

	m_EventTimeout = _query.within;

	loadEventMapping(_queryLoader);

	m_NumSlices = 32;
	m_SliceTime = _query.within / m_NumSlices;
	m_CurrentSlice = 0;

	m_Slice.resize(m_NumSlices * m_EventMapping.size());

	m_TaskList = 0;
	m_TaskToFetch = 0;
	m_TaskToFinish = 0;
	m_ExitTaskThreads = false;
}

PredicateMiner::~PredicateMiner()
{
	flushMatch();

	m_ExitTaskThreads = true;
	m_TaskCondition.notify_all();

	for (auto& it : m_TaskThread)
		it.join();
}

void PredicateMiner::initList(size_t _listIdx, size_t _eventType)
{
	m_Appearance[_listIdx].resize(m_EventMapping.size());

	for (size_t e = 0; e < m_EventMapping.size(); ++e)
	{
		if (e == _eventType)
			continue;

		AppearanceInfo& ai = m_Appearance[_listIdx][e];
		ai.todo.clear();

		const size_t numAttributes = m_EventMapping[_eventType].numAttributes;
		const uint8_t* mapping = &m_EventMapping[_eventType].eventMapping[e * numAttributes];

		// single attribute mining
		for (size_t a = 1; a < numAttributes; ++a)
		{
			if (mapping[a] == 0xff)
				continue; // there is no corresponding attribute in event type e
			
			AppearanceInfo::Todo item;
			item.attr1 = item.attr2 = a;
			item.attr1_mapped = item.attr2_mapped = mapping[a];
			
			for (uint32_t o = 0; o < O_MAX; ++o)
			{
				item.op1 = item.op2 = o;
				ai.todo.push_back(item);
			}
		}

		// combinational attribute mining
		for (size_t a1 = 1; a1 < numAttributes; ++a1)
		{
			if (mapping[a1] == 0xff)
				continue;

			for (size_t a2 = a1 + 1; a2 < numAttributes; ++a2)
			{
				if (mapping[a2] == 0xff)
					continue;

				for (size_t o1 = 0; o1 < O_MAX; ++o1)
				{
					for (size_t o2 = 0; o2 < O_MAX; ++o2)
					{
						AppearanceInfo::Todo item;
						item.attr1 = a1;
						item.attr2 = a2;
						item.attr1_mapped = mapping[a1];
						item.attr2_mapped = mapping[a2];
						item.op1 = o1;
						item.op2 = o2;
						ai.todo.push_back(item);
					}
				}
			}
		}
		ai.todo_count = ai.todo.size();
	}
}

void PredicateMiner::initWorkerThreads(size_t _numThreads)
{
	m_TaskThread.resize(_numThreads);
	for (auto& it : m_TaskThread)
		it = thread(std::bind(&PredicateMiner::taskThread, this));
}

void PredicateMiner::addEvent(uint32_t _type, const attr_t * _attributes)
{
	// add event to ring buffer
	EventItem item;
	item.type = _type;
	item.id = (uint32_t)_attributes[Query::DA_ID];
	item.nextIdByType = 0;
	for (size_t i = 0; i < m_EventMapping[_type].numAttributes; ++i)
		item.attributes[i] = _attributes[i];
	m_EventBuffer.push_back(item);

	// add event to time slice
	while (m_CurrentSlice != _attributes[0] / m_SliceTime)
	{
		if (_attributes[0] / m_SliceTime - m_CurrentSlice >= m_NumSlices)
		{
			for (auto& slice : m_Slice)
				slice.clear();
			m_CurrentSlice = _attributes[0] / m_SliceTime;
		}
		else
		{
			EventTypeSlice* base = &slice(++m_CurrentSlice, 0);
			for (EventTypeSlice* it = base; it != base + m_EventMapping.size(); ++it)
				it->clear();
		}
	}

	EventTypeSlice& mySlice = slice(m_CurrentSlice, _type);

	// insert attribute values
	const size_t numAttributes = m_NumAttributes[_type];
	for (size_t i = 1; i < numAttributes; ++i)
	{
		mySlice.attribute[i].insert(_attributes[i]);
		assert(slice(m_CurrentSlice, _type).attribute[i].contains(_attributes[i]));
	}

	// insert into event type chained list
	if (mySlice.empty())
	{
		mySlice.firstEventId = item.id;
		mySlice.lastEventId = item.id;
	}
	else
	{
		eventById(mySlice.lastEventId).nextIdByType = item.id;
		mySlice.lastEventId = item.id;
	}

}

void PredicateMiner::addMatch(uint32_t _event, uint32_t _eventEnd, size_t _listIdx)
{
	assert(_listIdx < sizeof(m_Appearance) / sizeof(m_Appearance[0]));
	assert(_event < _eventEnd);

	const EventItem& event_a = eventById(_event);

	size_t slice_begin = event_a.attributes[0] / m_SliceTime;
	size_t slice_end = slice_begin + m_NumSlices;
	if (slice_end > m_CurrentSlice)
		slice_end = m_CurrentSlice;

	if (_eventEnd <= m_EventBuffer.back().id)
		slice_end = eventById(_eventEnd).attributes[0] / m_SliceTime;

	MiningTask* task = new MiningTask;
	task->listIdx = _listIdx;
	task->event = &event_a;
	task->sliceBegin = slice_begin;
	task->sliceEnd = slice_end;
	task->current = slice_begin;
	task->next = m_TaskList;
	m_TaskList.store(task);

	m_TaskToFinish.fetch_add((int)(slice_end - slice_begin));
	m_TaskToFetch.fetch_add((int)(slice_end - slice_begin));
	m_TaskCondition.notify_all();
}

void PredicateMiner::flushMatch()
{
	while (processTask());
	while (m_TaskToFinish.load());

	MiningTask* it_next;
	for (MiningTask* it = m_TaskList; it; it = it_next)
	{
		it_next = it->next;
		delete it;
	}
	m_TaskList = 0;
}

void PredicateMiner::removeTimeouts(attr_t _timestamp)
{
	// remove old events from ring buffer
	while (!m_EventBuffer.empty() && m_EventBuffer.front().attributes[0] + m_EventTimeout < _timestamp)
		m_EventBuffer.pop_front();
}

static PatternMatcher::Operator resultingOperator(bool _le, bool _eq, bool _gr)
{
	if (_le && _eq && _gr)
		return PatternMatcher::OP_MAX;
	else if (!_eq && !_gr)
		return PatternMatcher::OP_LESS;
	else if (_le && _eq)
		return PatternMatcher::OP_LESSEQUAL;
	else if (_le && _gr)
		return PatternMatcher::OP_NOTEQUAL;
	else if (!_le && !_gr)
		return PatternMatcher::OP_EQUAL;
	else if (!_le && !_eq)
		return PatternMatcher::OP_GREATER;
	else if (_gr && _eq)
		return PatternMatcher::OP_GREATEREQUAL;
	else
		assert(!"bug");
	return PatternMatcher::OP_MAX;
}

static PatternMatcher::Operator resultingOperator(uint16_t _mask)
{
	return resultingOperator((_mask & 1) != 0, (_mask & 2) != 0, (_mask & 4) != 0);
}

std::vector<PredicateMiner::PredicateInfo> PredicateMiner::generateResult(size_t _posList, size_t _negList) const
{
	vector<PredicateInfo> ret;

	for (size_t e = 0; e < m_EventMapping.size(); ++e)
	{
		const AppearanceInfo& pos = m_Appearance[_posList][e];
		const AppearanceInfo& neg = m_Appearance[_negList][e];

		bool strongAttribute[PatternMatcher::MAX_ATTRIBUTES] = {};

		for (size_t a1 = 0; a1 < m_EventMapping[e].numAttributes; ++a1)
		{
			const bool le = !pos.contains(a1, O_LESS) && neg.contains(a1, O_LESS);
			const bool eq = !pos.contains(a1, O_EQUAL) && neg.contains(a1, O_EQUAL);
			const bool gr = !pos.contains(a1, O_GREATER) && neg.contains(a1, O_GREATER);

			if (le | eq | gr)
			{
				PredicateInfo p = {};
				p.op[0] = resultingOperator(le, eq, gr);
				p.attrIdx[0] = (uint32_t)a1;
				p.numAttr = p.op[0] != PatternMatcher::OP_MAX ? 1 : 0;

				p.eventId = (uint32_t)e;
				ret.push_back(p);
				strongAttribute[a1] = true;
			}
		}

		for (size_t a1 = 0; a1 < m_EventMapping[e].numAttributes; ++a1)
		{
			for (size_t a2 = a1 + 1; a2 < m_EventMapping[e].numAttributes; ++a2)
			{
				if (strongAttribute[a1] | strongAttribute[a2])
					continue;

				uint16_t opMask = ~pos.opmask(a1, a2) & neg.opmask(a1, a2);
				if (opMask)
				{
					map<uint16_t, uint16_t> ops;
					for (size_t i = 0; i < O_MAX; ++i)
					{
						ops[opMask & 0xF] |= 1 << i;
						opMask >>= 4;
					}

					PredicateInfo p = {};
					p.numAttr = 2;
					p.attrIdx[0] = (uint32_t)a1;
					p.attrIdx[1] = (uint32_t)a2;
					p.eventId = (uint32_t)e;

					for (auto& it : ops)
					{
						p.op[0] = resultingOperator(it.second);
						p.op[1] = resultingOperator(it.first);

						if(p.op[0] != PatternMatcher::OP_MAX && p.op[1] != PatternMatcher::OP_MAX)
							ret.push_back(p);
					}
				}
			}
		}
	}
	return ret;
}

void PredicateMiner::printResult(size_t _posList, size_t _negList) const
{
	size_t sumTodoPos = 0, sumTodoNeg = 0;
	for (const auto& e : m_Appearance[_posList])	sumTodoPos += e.todo_count;
	for (const auto& e : m_Appearance[_negList])	sumTodoNeg += e.todo_count;
	cerr << "miner: result of lists " << _posList << "(" << sumTodoPos << " todo's)|" << _negList << "(" << sumTodoNeg << " todo's) at event " << m_EventBuffer.back().id << endl;

	for (auto& it : generateResult(_posList, _negList))
	{
		cerr << "miner: event" << it.eventId << '[';
		for (size_t i = 0; i < it.numAttr; ++i)
		{
			const char* opcode = it.op[i] < PatternMatcher::OP_MAX ? PatternMatcher::operatorInfo(it.op[i]).sign : "any";
			cerr << it.attrIdx[i] << '(' << opcode << ") ";
		}
		cerr << ']' << endl;
	}
}

Query PredicateMiner::buildPredicateQuery(const QueryLoader& _queryLoader, const Query & _src, uint32_t _idx, const PredicateInfo & _predicate) const
{
	Query dst = _src;

	Query::Event e;
	e.kleenePlus = false;
	e.stopEvent = true;
	e.name = "stop_event";
	e.type = _queryLoader.eventDecl(_predicate.eventId)->name;
	dst.events.insert(dst.events.begin() + _idx + 1, e);

	// move all event indices :(

	for (auto& p : dst.where)
	{
		if (p.event1 > _idx)
			p.event1++;
		if (p.event2 > _idx)
			p.event2++;
	}

	for (auto& a : dst.aggregation)
	{
		if (a.source.first > _idx)
			a.source.first++;
	}

	for (auto& r : dst.returnAttr)
	{
		if (r.first > _idx)
			r.first++;
	}

	// add predicate
	for (size_t i = 0; i < _predicate.numAttr; ++i)
	{
		Query::Predicate p = {};
		p.event1 = _idx;
		p.event2 = _idx + 1;
		p.attr2 = _predicate.attrIdx[i];
		p.op = _predicate.op[i];

		const EventDecl* decl1 = _queryLoader.eventDecl(dst.events[p.event1].type.c_str());
		const string& attrName = _queryLoader.eventDecl(_predicate.eventId)->attributes[p.attr2];

		auto it = find(decl1->attributes.begin(), decl1->attributes.end(), attrName);
		p.attr1 = (uint32_t)(it - decl1->attributes.begin());

		dst.where.push_back(p);
	}

	dst.fillAttrMap();
	return dst;
}

bool PredicateMiner::processTask()
{
	if (m_TaskToFetch-- > 0)
	{
		for (MiningTask* it = m_TaskList.load(); it; it = it->next)
		{
			size_t slice = it->current.load();
			do
			{
				if (slice == it->sliceEnd)
					goto next_task;
			} while (!it->current.compare_exchange_weak(slice, slice + 1));

			processTask(it, slice);
			m_TaskToFinish--;
			return true;

		next_task:;
		}
	}
	m_TaskToFetch++;
	return false;
}

void PredicateMiner::processTask(const MiningTask * _task, size_t t)
{
	for (size_t e = 0; e < m_EventMapping.size(); ++e)
	{
		const EventTypeSlice& s = slice(t, e);

		if ((s.empty()) ||
			(t == _task->sliceBegin && _task->event->type == e))
			continue;

		AppearanceInfo& appearance = m_Appearance[_task->listIdx][e];

		size_t todo_count = appearance.todo_count.load();
		for (size_t i = 0; i < todo_count; ++i)
		{
			AppearanceInfo::Todo& item = appearance.todo[i];
			bool found = false;

			if (item.attr1 == item.attr2)
			{
				if (found = searchSingleAttribute(item, *_task->event, s))
					appearance.insert(item.attr1, (Op)item.op1);
			}
			else
			{
				if (appearance.contains(item.attr1, (Op)item.op1) &&
					appearance.contains(item.attr2, (Op)item.op2))
				{
					if (found = searchMultiAttribute(item, *_task->event, s))
						appearance.insert(item.attr1, (Op)item.op1, item.attr2, (Op)item.op2);
				}
			}

			if (found)
			{
				item = appearance.todo[--appearance.todo_count];
			}
		}
	}
}

void PredicateMiner::taskThread()
{
	while (!m_ExitTaskThreads)
	{
		while (processTask());

		{
			unique_lock<mutex> lock(m_TaskMutex);
			m_TaskCondition.wait(lock, [this]() {return this->m_TaskToFetch > 0 || this->m_ExitTaskThreads;});
		}
	}
}

bool PredicateMiner::searchSingleAttribute(const AppearanceInfo::Todo & _item, const EventItem & _event, const EventTypeSlice & _slice) const
{
	const SliceInfo& cmpslice = _slice.attribute[_item.attr1_mapped];
	const attr_t cmpattr = _event.attributes[_item.attr1];

	switch (_item.op1)
	{
	case O_LESS:
		return cmpattr > cmpslice.min;
	case O_GREATER:
		return cmpattr < cmpslice.max;
	default:
		if (cmpslice.contains(cmpattr))
		{
			size_t it_offset = _slice.firstEventId;
			const EventItem* it;
			do {
				it = &m_EventBuffer[it_offset - m_EventBuffer.front().id];
				it_offset = it->nextIdByType;

				if (it->attributes[_item.attr1_mapped] == cmpattr)
				{
					return true;
				}
			} while (it_offset);
		}
	}

	return false;
}

bool PredicateMiner::searchMultiAttribute(const AppearanceInfo::Todo & _item, const EventItem & _event, const EventTypeSlice & _slice) const
{
	const attr_t attr[] = { _event.attributes[_item.attr1], _event.attributes[_item.attr2] };
	const SliceInfo* slice[] = { &_slice.attribute[_item.attr1_mapped], &_slice.attribute[_item.attr2_mapped] };

	for (size_t i = 0; i < 2; ++i)
	{
		switch (i ? _item.op2 : _item.op1)
		{
		case O_LESS:
			if (attr[i] <= slice[i]->min)
				return false;
		case O_GREATER:
			if (attr[i] >= slice[i]->max)
				return false;
		default:
			if (!slice[i]->contains(attr[i]))
				return false;
		}
	}

	size_t it_offset = _slice.firstEventId;
	const EventItem* it;
	do {
		it = &m_EventBuffer[it_offset - m_EventBuffer.front().id];
		it_offset = it->nextIdByType;

		if (compareAttributes(it->attributes[_item.attr1_mapped], attr[0], (Op)_item.op1) &&
			compareAttributes(it->attributes[_item.attr2_mapped], attr[1], (Op)_item.op2))
		{
			return true;
		}
	} while (it_offset);

	return false;
}

void PredicateMiner::loadEventMapping(const QueryLoader & _queryLoader)
{
	const size_t numEvents = _queryLoader.numEventDecls();

	m_EventMapping.resize(numEvents);
	for (size_t i = 0; i < numEvents; ++i)
	{
		const EventDecl* decl_a = _queryLoader.eventDecl(i);

		m_EventMapping[i].numAttributes = decl_a->attributes.size();
		m_EventMapping[i].eventMapping.resize(decl_a->attributes.size() * numEvents);

		for (size_t j = 0; j < numEvents; ++j)
		{
			const EventDecl* decl_b = _queryLoader.eventDecl(j);

			for (size_t a = 0; a < decl_a->attributes.size(); ++a)
			{
				const string& attrName = decl_a->attributes[a];

				uint8_t& mapping = m_EventMapping[i].eventMapping[j * m_EventMapping[i].numAttributes + a];
				mapping = 0xff;

				for (size_t b = 0; b < decl_b->attributes.size(); ++b)
				{
					if (decl_b->attributes[b] == attrName)
					{
						mapping = (uint8_t)b;
						break;
					}
				}
			}
		}
	}
}
