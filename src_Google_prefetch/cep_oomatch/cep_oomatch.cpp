#include "../Query.h"
#include "../EventStream.h"
#include "../_shared/OutofOrderPatternMatcher.h"
#include "../_shared/MonitorThread.h"

#include "../freegetopt/getopt.h"

#include <iostream>
#include <vector>
#include <chrono>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <set>
#include <assert.h>

using namespace std;
using namespace std::chrono;

static time_point<high_resolution_clock> g_BeginClock;

inline void init_utime()
{
	g_BeginClock = high_resolution_clock::now();
}

inline uint64_t current_utime()
{
	return duration_cast<microseconds>(high_resolution_clock::now() - g_BeginClock).count();
}

class CepOoMatch
{
public:
	CepOoMatch() : m_DefAttrId(0), m_DefAttrOffset(0)
	{
	}

	~CepOoMatch()
	{

	}

	bool init(const char* _defFile, const char* _queryName, size_t _extraDelay, bool _generateTimeoutEvents, bool _appendTimestamp)
	{
		StreamEvent::setupStdIo();

		if (!m_Definition.loadFile(_defFile))
		{
			fprintf(stderr, "failed to load definition file %s\n", _defFile);
			return false;
		}

		const Query* q = !_queryName ? m_Definition.query((size_t)0) : m_Definition.query(_queryName);
		if (!q)
		{
			fprintf(stderr, "query not found");
			return false;
		}
		m_Query = *q;
		modifyQueryForCompletness(m_Query, _extraDelay);

		uint8_t falgs = _appendTimestamp ? StreamEvent::F_TIMESTAMP : 0;

		QueryLoader::Callbacks cb;
		cb.insertEvent[PatternMatcher::ST_ACCEPT] = bind(&CepOoMatch::write_event, this, falgs, placeholders::_1, placeholders::_2);

		if (_generateTimeoutEvents)
			cb.timeoutEvent = bind(&CepOoMatch::write_event, this, falgs | StreamEvent::F_TIMEOUT, placeholders::_1, placeholders::_2);

		if (!m_Definition.setupPatternMatcher(&m_Query, m_Matcher, cb))
		{
			return false;
		}
		m_Matcher.init(&m_Query, bind(&CepOoMatch::write_event, this, falgs | StreamEvent::F_REVOKE, 0, placeholders::_1));

		m_Query.generateCopyList(m_Query.returnAttr, m_OutEventAttrSrc);

		m_ResultEventType = m_Definition.findEventDecl(m_Query.returnName.c_str());
		m_ResultEventTypeHash = StreamEvent::hash(m_Query.returnName);
		m_ResultAttributeCount = (uint8_t)m_OutEventAttrSrc.size();
		m_GenerateTimeoutEvents = _generateTimeoutEvents;
		m_AppendTimestamp = _appendTimestamp;

		return true;
	}

	void copyAndInsertRejectEvent(Query& _q, string _postfix, uint32_t _src, uint32_t _dst, uint32_t _prevEvent, uint32_t _nextEvent)
	{
		Query::Event e = _q.events[_src];
		e.name = e.name + "_" + _postfix;

		_q.insertEvent(e, _dst);

		// copy conditions
		size_t numWhere = _q.where.size();
		for (size_t k = 0; k < numWhere; ++k)
		{
			if (_q.where[k].event2 == _src)
			{
				_q.where.push_back(_q.where[k]);
				_q.where.back().event2 = _dst;
			}
		}

		// add out of order timestamp check
		_q.where.push_back({ _prevEvent, _dst, 0, 0, PatternMatcher::OP_LESS });
		_q.where.push_back({ _nextEvent, _dst, 0, 0, PatternMatcher::OP_GREATER });
	}

	void modifyQueryForCompletness(Query& _q, uint64_t _extraDelay)
	{
		vector<uint32_t> generatedEvent;
		const string acceptingEvent = _q.events.back().name;

		uint32_t lastNormalEvent = 0;
		for (uint32_t i = 1; i < _q.events.size(); ++i)
		{
			auto it = lower_bound(generatedEvent.begin(), generatedEvent.end(), i);
			if (it == generatedEvent.end() || *it != i)
			{
				if (_q.events[i].stopEvent)
				{
					// duplicate stop event to every but last event
					uint32_t nextNormalEvent = i + 1;
					while (_q.events[nextNormalEvent].stopEvent) nextNormalEvent++;

					for (uint32_t j = nextNormalEvent; j < _q.events.size(); j++)
					{
						if (_q.events[j].name == acceptingEvent)
							break;
						if (!_q.events[j].stopEvent)
						{
							string postfix = _q.events[j].name;
							copyAndInsertRejectEvent(_q, postfix, i, ++j, lastNormalEvent, nextNormalEvent);
							generatedEvent.push_back(j);
						}
					}

					copyAndInsertRejectEvent(_q, "revoke", i, (uint32_t)_q.events.size(), lastNormalEvent, nextNormalEvent);
				}
				else
				{
					// store dummy attribute. holding the reject timestamp
					_q.where.push_back({ lastNormalEvent, i, Query::DA_MAX, 0, PatternMatcher::OP_GREATER });
				}

				// check in order (ei.timestamp > ej.timestamp)
				_q.where.push_back({ lastNormalEvent, i, 0, 0, PatternMatcher::OP_LESS });

				// check timeout (e0.timestamp >= ei.timestamp - time window)
				_q.where.push_back({ 0, i, 0, 0, PatternMatcher::OP_GREATEREQUAL, -(attr_t)_q.within });
			}

			if (!_q.events[i].stopEvent)
				lastNormalEvent = i;
			if (_q.events[i].name == acceptingEvent)
				break;
		}

		_q.name += "_oo";
		_q.within += _extraDelay;
		_q.fillAttrMap();

		m_Definition.addQuery(_q);
		m_Definition.storeFile("_work\\oo.eql");

	}


	bool processEvent()
	{
		StreamEvent event;

		if (!event.read())
			return false;

		event.attributes[Query::DA_ZERO] = 0;
		event.attributes[Query::DA_MAX] = numeric_limits<attr_t>::max();
		event.attributes[Query::DA_CURRENT_TIME] = current_utime();
		event.attributes[Query::DA_OFFSET] = m_DefAttrOffset;
		event.attributes[Query::DA_ID] = m_DefAttrId;

		const EventDecl* decl = m_Definition.eventDecl(event.typeIndex);
		assert(event.typeHash == StreamEvent::hash(decl->name));

		m_Matcher.event(event.typeIndex, (attr_t*)event.attributes);

		m_DefAttrId++;
		m_DefAttrOffset += event.offset;
		assert(event.offset > 0);

		return true;
	}

protected:

	void write_event(uint8_t _flags, uint32_t _state, const attr_t* _attributes)
	{
		StreamEvent r;
		r.typeIndex = m_ResultEventType;
		r.typeHash = m_ResultEventTypeHash;
		r.attributeCount = m_ResultAttributeCount;
		r.flags = _flags;

		if (_flags & StreamEvent::F_TIMEOUT)
			r.timeoutState = (uint8_t)(_state - 1);

		if (_flags & StreamEvent::F_TIMESTAMP)
			r.attributes[r.attributeCount++] = current_utime();

		uint64_t* outattr_it = r.attributes;
		for (auto it : m_OutEventAttrSrc)
			*outattr_it++ = _attributes[it];

		r.write();
	}

private:
	QueryLoader			m_Definition;
	Query				m_Query;

	OutofOrderPatternMatcher		m_Matcher;
	vector<uint32_t>	m_OutEventAttrSrc;

	uint16_t			m_ResultEventType;
	uint32_t			m_ResultEventTypeHash;
	uint8_t				m_ResultAttributeCount;
	bool				m_GenerateTimeoutEvents;
	bool				m_AppendTimestamp;

	uint64_t			m_DefAttrId;
	uint64_t			m_DefAttrOffset;
};

int main(int _argc, char* _argv[])
{
	init_utime();

	const char* deffile = "default.eql";
	const char* queryName = 0;
	const char* monitorFile = 0;
	bool captureTimeouts = false;
	bool appendTimestamp = false;
	size_t delay = 0;

	int c;
	while ((c = getopt(_argc, _argv, "c:q:p:d:ts")) != -1)
	{
		switch (c)
		{
		case 'c':
			deffile = optarg;
			break;
		case 'q':
			queryName = optarg;
			break;
		case 'p':
			monitorFile = optarg;
			break;
		case 'd':
			delay = atoll(optarg);
			break;
		case 't':
			captureTimeouts = true;
			break;
		case 's':
			appendTimestamp = true;
			break;
		default:
			abort();
		}
	}

	CepOoMatch prog;
	if (!prog.init(deffile, queryName, delay, captureTimeouts, appendTimestamp))
		return 1;

	volatile uint64_t eventCounter = 0;

	MonitorThread monitor;
	monitor.addValue(&current_utime);
	monitor.addValue(&eventCounter, true);

	if (monitorFile)
		monitor.start(monitorFile);

	while (prog.processEvent())
		eventCounter++;

	return 0;
}
