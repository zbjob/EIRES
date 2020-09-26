#include "OutofOrderPatternMatcher.h"
#include "../Query.h"

#include <assert.h>

OutofOrderPatternMatcher::OutofOrderPatternMatcher() : m_LastEventTimestamp(0)
{
}

void OutofOrderPatternMatcher::init(const Query* _query, const std::function<void(const attr_t*)>& _revokeCallback)
{
	// find relevant event types
	for (auto& transition : m_Transitions)
	{
		if (transition.eventType >= m_StoreEvent.size())
			m_StoreEvent.resize(transition.eventType + 1);
		m_StoreEvent[transition.eventType] = true;
	}
	m_EventQueue.resize(m_StoreEvent.size());

	// setup magical _MAX attribute to be the timestamp of partial match termination
	for (auto& t : m_Transitions)
	{
		if (m_States[t.to].type != ST_REJECT)
			continue;

		if (m_States[t.from].type == ST_ACCEPT)
		{
			// update reject state to be accepting revoke state
			m_States[t.from].setAttributeCount((uint32_t)_query->attrMap.size());
			m_States[t.to].type = ST_ACCEPT;
			m_States[t.to].callback_insert = _revokeCallback;
			t.updateHandler(m_States[t.from], m_States[t.to]);
		}
		else
		{
			bool found = false;
			for (auto& t1 : m_Transitions)
			{
				if (t1.to == t.from)
				{
					for (const auto& a : t1.actions)
					{
						if (a.src == Query::DA_MAX)
						{
							assert(!found);
							found = true;
							uint32_t destinationAttribute = a.dst;

							t.setCustomExecuteHandler([=](State & _from, State & _to, uint32_t _idx, const attr_t* _attributes) {
								attr_t& rejectTimestamp = _from.attr[destinationAttribute][_idx];
								if (rejectTimestamp > _attributes[0])
									rejectTimestamp = _attributes[0];
							});
						}
					}
				}
			}

			// uncatched reject will result in attribute array overflow during event replay
			assert(found);
		}
	}
}

uint32_t OutofOrderPatternMatcher::event(uint32_t _type, const attr_t * _attr)
{
	if (_type >= m_StoreEvent.size() || !m_StoreEvent[_type])
		return 0;

	EventInfo cp;
	memcpy(cp.attribute, _attr, sizeof(cp.attribute));

	if (m_LastEventTimestamp > _attr[0])
		return ooevent(_type, cp);

	m_LastEventTimestamp = _attr[0];
	m_EventQueue[_type].push_back(cp);

	while (m_EventQueue[_type].front().attribute[0] < _attr[0] - m_Timeout)
		m_EventQueue[_type].pop_front();

	return PatternMatcher::event(_type, _attr);
}

uint32_t OutofOrderPatternMatcher::ooevent(uint32_t _type, EventInfo & _event)
{
	for (auto& it : m_Transitions)
	{
		if (it.eventType != _type)
			continue;

		auto& destState = m_States[it.to];
		it.checkEvent(m_States[it.from], destState, 0, _event.attribute);
	}

	for (auto& it : m_Transitions)
	{
		if (it.eventType != _type)
			continue;

		auto& destState = m_States[it.to];
		size_t numRuns = destState.count;
		destState.endTransaction();

		if (destState.count != numRuns)
		{
			replayEvents(it.to, _event.attribute[0], numRuns);
		}
	}

	// insert event
	for (auto it = m_EventQueue[_type].rbegin(); it != m_EventQueue[_type].rend(); ++it)
	{
		if ((*it).attribute[0] < _event.attribute[0])
		{
			m_EventQueue[_type].insert(it.base(), _event);
			return 0;
		}
	}

	m_EventQueue[_type].push_front(_event);
	return 0;
}

uint32_t OutofOrderPatternMatcher::replayEvents(uint32_t _state, attr_t _timestamp, size_t _runOffset)
{
	for (auto& t : m_Transitions)
	{
		if (t.from != _state)
			continue;

		auto revit = m_EventQueue[t.eventType].rbegin();
		while (revit != m_EventQueue[t.eventType].rend() && revit->attribute[0] > _timestamp)
			revit++;

		if (revit != m_EventQueue[t.eventType].rbegin())
		{
			auto& destState = m_States[t.to];
			size_t numRuns = destState.count;

			for (auto it = revit.base(); it != m_EventQueue[t.eventType].end(); ++it)
				t.checkEvent(m_States[t.from], destState, _runOffset, it->attribute);

			destState.endTransaction();

			if (destState.count != numRuns)
			{
				replayEvents(t.to, _timestamp, numRuns);
			}
		}
	}
	return 0;
}
