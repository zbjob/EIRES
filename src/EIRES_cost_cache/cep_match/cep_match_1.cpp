#include "../Query.h"
#include "../PatternMatcher.h"
#include "../EventStream.h"
#include "../_shared/PredicateMiner.h"
#include "../_shared/MonitorThread.h"
#include "../_shared/GlobalClock.h"

#include "../freegetopt/getopt.h"

#include <vector>
#include <chrono>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <assert.h>
#include <iostream>

using namespace std;
using namespace std::chrono;

//static time_point<high_resolution_clock> g_BeginClock;
time_point<high_resolution_clock> g_BeginClock;
long int NumFullMatch = 0;
long int NumHighLatency = 0;

inline void init_utime() 
{
	g_BeginClock = high_resolution_clock::now();
}

inline uint64_t current_utime() 
{ 
	return duration_cast<microseconds>(high_resolution_clock::now() - g_BeginClock).count();
}

class CepMatch
{
public:
	CepMatch() : m_Query(0), m_DefAttrId(0), m_DefAttrOffset(0), m_NextMinerUpdateTime(0)
	{
        lastSheddingTime = 0;
        loadCnt = 0;
        time = 60000000;
        acctime = time;
        Rtime = time/50000;
        latency = 0;
        cntFullMatch = 1;
	}

	~CepMatch()
	{

	}

	bool init(const char* _defFile, const char* _queryName, const char* _miningPrefix, bool _generateTimeoutEvents, bool _appendTimestamp)
	{
		StreamEvent::setupStdIo();

		if (!m_Definition.loadFile(_defFile))
		{
			fprintf(stderr, "failed to load definition file %s\n", _defFile);
			return false;
		}

		m_Query = !_queryName ? m_Definition.query((size_t)0) : m_Definition.query(_queryName);
		if (!m_Query)
		{
			fprintf(stderr, "query not found");
			return false;
		}

		if(_miningPrefix)
		{
			m_MiningPrefix = _miningPrefix;
			m_Miner.reset(new PredicateMiner(m_Definition, *m_Query));
			_generateTimeoutEvents = true; // important for mining

			for (size_t i = 0; i < m_Query->events.size() - 1; ++i)
			{
				uint32_t eventType = m_Definition.findEventDecl(m_Query->events[i].type.c_str());
				m_Miner->initList(i * 2 + 0, eventType);
				m_Miner->initList(i * 2 + 1, eventType);
			}

			unsigned numCores = thread::hardware_concurrency();
			m_Miner->initWorkerThreads(std::min(numCores - 1, 16u));

			const uint64_t one_min = 60 * 1000 * 1000;
			m_NextMinerUpdateTime = current_utime() + one_min / 6;
		}

		QueryLoader::Callbacks cb;
		cb.insertEvent[PatternMatcher::ST_ACCEPT] = bind(&CepMatch::write_event, this, false, placeholders::_1, placeholders::_2);

		if(_generateTimeoutEvents)
			cb.timeoutEvent = bind(&CepMatch::write_event, this, true, placeholders::_1, placeholders::_2);

		if (!m_Definition.setupPatternMatcher(m_Query, m_Matcher, cb))
		{
			return false;
		}

		m_Query->generateCopyList(m_Query->returnAttr, m_OutEventAttrSrc);

		m_ResultEventType = m_Definition.findEventDecl(m_Query->returnName.c_str());
		m_ResultEventTypeHash = StreamEvent::hash(m_Query->returnName);
		m_ResultAttributeCount = (uint8_t)m_OutEventAttrSrc.size();
		m_GenerateTimeoutEvents = _generateTimeoutEvents;
		m_AppendTimestamp = _appendTimestamp;

		return true;
	}

	bool processEvent()
	{
		StreamEvent event;

		if (!event.read())
			return false;

		event.attributes[Query::DA_ZERO] = 0;
		event.attributes[Query::DA_MAX] = numeric_limits<attr_t>::max();
		event.attributes[Query::DA_CURRENT_TIME] = current_utime();
		//event.attributes[Query::DA_CURRENT_TIME] = (uint64_t)duration_cast<microseconds>(high_resolution_clock::now() - g_BeginClock).count(); 
		event.attributes[Query::DA_OFFSET] = m_DefAttrOffset;
		event.attributes[Query::DA_ID] = m_DefAttrId;

		const EventDecl* decl = m_Definition.eventDecl(event.typeIndex);
		assert(event.typeHash == StreamEvent::hash(decl->name));

		m_Matcher.event(event.typeIndex, (attr_t*)event.attributes);

		if (m_Miner)
		{
			m_Miner->flushMatch();
			m_Miner->addEvent(event.typeIndex, (const attr_t*)event.attributes);
			m_Miner->removeTimeouts(event.attributes[0]);
		}

		m_DefAttrId++;
		m_DefAttrOffset += event.offset;
		assert(event.offset > 0);

		if (m_Miner && m_NextMinerUpdateTime <= event.attributes[Query::DA_CURRENT_TIME])
		{
			const uint64_t one_min = 60 * 1000 * 1000;
			m_NextMinerUpdateTime = event.attributes[Query::DA_CURRENT_TIME] + 10 * one_min;
			update_miner();
		}

		return true;
	}

    void printContribution()
    {
        for(auto&& it: m_Matcher.m_States)
        {
            //std::cout << endl << "state contributions size == " << it.contributions.size() << endl;
            //for(auto&& iter: it.contributions)
                //cout << iter.first << "appears " << iter.second << "times" << endl;

            //std::cout << endl << "state consumptions size == " << it.consumptions.size() << endl;
            //for(auto&& iter: it.consumptions)
            //    cout << iter.first << "appears " << iter.second << "times" << endl;
            //std::cout << endl << "state scoreTable size == " << it.scoreTable.size() << endl;

            if(!it.attr.empty())
            std::cout << "state size == " <<  it.attr.front().size() << endl;

            
        }
    }

	void update_miner()
	{
		if (!m_Miner)
			return;

		string eqlFilename = m_MiningPrefix + ".eql";
		string scriptFilename = m_MiningPrefix + ".sh";

		ofstream script(scriptFilename, ofstream::out | ofstream::trunc);

		QueryLoader dst = m_Definition;
		for (size_t i = 0; i < m_Query->events.size() - 1; ++i)
		{
			m_Miner->printResult(i * 2, i * 2 + 1);

			auto options = m_Miner->generateResult(i * 2, i * 2 + 1);
			for (auto& it : options)
			{
				Query q = m_Miner->buildPredicateQuery(dst, *m_Query, (uint32_t)i, it);

				ostringstream namestr;
				namestr << q.name << "_mined_" << i << '_' << it.eventId;
				for (size_t i = 0; i < it.numAttr; ++i)
					namestr << '_' << it.attrIdx[i];

				q.name = namestr.str();

				script << "$CEP_CMD -c " << eqlFilename << " -q " << q.name << " < $CEP_IN > ${CEP_OUT}" << q.name << " &" << endl;
				dst.addQuery(move(q));
			}
		}

		dst.storeFile(eqlFilename.c_str());
	}

//protected:

	void write_event(bool _timeout, uint32_t _state, const attr_t* _attributes)
	{
		StreamEvent r;
		r.typeIndex = m_ResultEventType;
		r.typeHash = m_ResultEventTypeHash;
		r.attributeCount = m_ResultAttributeCount;
		r.flags = 0;

		if (_timeout)
		{
			r.flags = StreamEvent::F_TIMEOUT;
			r.timeoutState = (uint8_t)(_state - 1);
		}

		if (m_AppendTimestamp)
		{
			r.flags |= StreamEvent::F_TIMESTAMP;
			r.attributes[r.attributeCount++] = current_utime();
		}

		uint64_t* outattr_it = r.attributes;
		for (auto it : m_OutEventAttrSrc)
			*outattr_it++ = _attributes[it];


        //cout << "latency  in write event" << _attributes[Query::DA_FULL_MATCH_TIME] << "--" <<   _attributes[Query::DA_CURRENT_TIME] << "--" << r.attributes[Query::DA_CURRENT_TIME] << endl; 

		//r.write();

        //monitoring load shedding: if latency exceeds threshold for 2 times, call load shedding
        uint64_t la = _attributes[Query::DA_FULL_MATCH_TIME] - _attributes[Query::DA_CURRENT_TIME]; 
        if(_attributes[Query::DA_FULL_MATCH_TIME] / 50000 == time / 50000)
        {
            acctime += _attributes[Query::DA_FULL_MATCH_TIME];
            latency += la;
            Rtime = time/50000; 
            cntFullMatch++;
        }
        else
        {
            cout << Rtime << "," << long(latency/cntFullMatch) << endl;
            printContribution();

            if(latency/cntFullMatch > LATENCY) ++NumHighLatency;
            if(latency/cntFullMatch > LATENCY && _attributes[Query::DA_FULL_MATCH_TIME] - lastSheddingTime > 3000000)
            {
                //m_Matcher.randomLoadShedding();
                //m_Matcher.loadShedding();
                //m_Matcher.fixNumLoadShedding();
                lastSheddingTime = _attributes[Query::DA_FULL_MATCH_TIME];
                ++loadCnt;
            }
            cntFullMatch = 1;
            time = _attributes[Query::DA_FULL_MATCH_TIME];
            latency = la;
            acctime = time;
            Rtime = time/50000;
            
        }
        
        //if(_attributes[Query::DA_FULL_MATCH_TIME] - _attributes[Query::DA_CURRENT_TIME] > LATENCY && _attributes[Query::DA_FULL_MATCH_TIME] - lastSheddingTime > 3000000)
        //{
        //    
        //    if(m_Matcher.latencyFlag == true)
        //    {
        //        //lastSheddingTime = _attributes[Query::DA_FULL_MATCH_TIME]; 
        //        //m_Matcher.loadShedding();
        //        //m_Matcher.randomLoadShedding();
        //        lastSheddingTime = _attributes[Query::DA_FULL_MATCH_TIME]; 
        //        //++loadCnt;
        //        
        //    }

        //    else
        //        m_Matcher.latencyFlag = true;
        //}
        //else
        //{
        //    m_Matcher.latencyFlag = false;
        //}


        //////


		if (m_Miner)
		{
			if (_timeout)
			{
				size_t idx = (_state - 1) * 2 + 1;
				uint32_t eventA = (uint32_t)r.attributes[_state];
				uint32_t eventB = (uint32_t)m_DefAttrId;

				m_Miner->addMatch(eventA, eventB, idx);
			}
			else
			{
				for (uint32_t s = 1; s < _state; ++s)
				{
					size_t idx = (s - 1) * 2;
					uint32_t eventA = (uint32_t)r.attributes[s];
					uint32_t eventB = (uint32_t)r.attributes[s + 1];

					m_Miner->addMatch(eventA, eventB + 1, idx);
				}
			}
		}
	}

//private:
	QueryLoader			m_Definition;
	const Query*		m_Query;

	unique_ptr<PredicateMiner> m_Miner;
	string				m_MiningPrefix;
	PatternMatcher		m_Matcher;
	vector<uint32_t>	m_OutEventAttrSrc;

	uint16_t			m_ResultEventType;
	uint32_t			m_ResultEventTypeHash;
	uint8_t				m_ResultAttributeCount;
	bool				m_GenerateTimeoutEvents;
	bool				m_AppendTimestamp;

	uint64_t			m_DefAttrId;
	uint64_t			m_DefAttrOffset;

	uint64_t			m_NextMinerUpdateTime;
    uint64_t            lastSheddingTime;

    uint64_t            time;
    uint64_t            latency;
    uint64_t            acctime;
    uint64_t            Rtime;
    uint64_t            cntFullMatch;


    int loadCnt;
};

//bool PatternMatcher::loadMonitoringFlag = false;
int main(int _argc, char* _argv[])
{
	init_utime();

	const char* deffile = "default.eql";
	const char* queryName = 0;
	const char* monitorFile = 0;
	const char* miningPrefix = 0;
	bool captureTimeouts = false;
	bool appendTimestamp = false;

	int c;
	while ((c = getopt(_argc, _argv, "c:q:p:m:ts")) != -1)
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
		case 'm':
			miningPrefix = optarg;
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

	CepMatch prog;
	if (!prog.init(deffile, queryName, miningPrefix, captureTimeouts, appendTimestamp))
		return 1;

	volatile uint64_t eventCounter = 0;

	MonitorThread monitor;
	monitor.addValue(&current_utime);
	monitor.addValue(&eventCounter, true);

	if (monitorFile)
		monitor.start(monitorFile);

    //PatternMatcher::setMonitoringLoadOn();
    PatternMatcher::setMonitoringLoadOff();
    cout << "monitoring " << PatternMatcher::MonitoringLoad() << endl;
    //int loadCnt = 0;
	while(prog.processEvent())
    {
		eventCounter++;
        if(PatternMatcher::MonitoringLoad() == true){
        if(eventCounter  == 1000000 )
        {
            PatternMatcher::setMonitoringLoadOff();
            cout << "call loadshedding" << endl;
            
            time_point<high_resolution_clock> t0 = high_resolution_clock::now();
            
            //prog.m_Matcher.computeScores4LoadShedding();
            //prog.m_Matcher.loadShedding();

            time_point<high_resolution_clock> t1 = high_resolution_clock::now();
            cout << "1st load shedding time " << duration_cast<microseconds>(t1 - t0).count() << endl;
            //prog.loadCnt++;

        }

        //if(eventCounter > 100000 && eventCounter % 100000 == 1)
        //{

        //    time_point<high_resolution_clock> t0 = high_resolution_clock::now();
        //    prog.m_Matcher.loadShedding();
        //    time_point<high_resolution_clock> t1 = high_resolution_clock::now();
        //    cout << "laod shedding time " << duration_cast<microseconds>(t1 - t0).count() << endl;
        //    ++loadCnt;
        //}
        }
    }


        


	prog.update_miner();
    prog.printContribution();
    std::cout << "perform " << prog.loadCnt << "loadshedding" << endl;
    std::cout << "eventCn " << eventCounter << endl;
    std::cout << "#full Match " << NumFullMatch << endl;
    std::cout << "#high latecny (> " << LATENCY << ") " << NumHighLatency<< endl;
	return 0;
}
