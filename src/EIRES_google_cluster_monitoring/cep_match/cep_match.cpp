#include "../Query.h"
#include "../PatternMatcher.h"
#include "../EventStream.h"
#include "../_shared/PredicateMiner.h"
#include "../_shared/MonitorThread.h"
#include "../_shared/GlobalClock.h"

#include "../freegetopt/getopt.h"

#include "../Cache_ordered.h"
#include "../FetchWorker.h"


#include <vector>
#include <chrono>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <random>
#include <vector>


using namespace std;
using namespace std::chrono;

time_point<high_resolution_clock> g_BeginClock;
long int NumFullMatch = 0;
long int NumHighLatency = 0;
long int NumPM= 0;
long int NumSheddingPM=0;

default_random_engine G_generator;
uniform_int_distribution<int> G_distribution(1,100);

int PatternMatcher::Transition::fetchCnt = 0;
int PatternMatcher::Transition::prefetchCnt = 0;
int PatternMatcher::Transition::prefetchFrequency = 1000;
int PatternMatcher::Transition::fetchProbability= 100;
int PatternMatcher::Transition::consumptionPolicy = 0;
int PatternMatcher::Transition::fetch_latency = 1;
bool PatternMatcher::Transition::FLAG_delay_fetch = false;
bool PatternMatcher::Transition::FLAG_prefetch = false;
bool PatternMatcher::Transition::FLAG_baseline = false;
int FetchWorker::fetch_latency = 1;


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


        void addTypeForAppCnt(string _typeName)
        {
            uint16_t typeHash = StreamEvent::hash(_typeName);
            m_TypeHash_String_map[typeHash] = _typeName;
            m_TypeHash_AppCnt_map[typeHash] = 0;
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
            uint16_t TypeHash = StreamEvent::hash(decl->name);
            //assert(event.typeHash == StreamEvent::hash(decl->name));
            assert(event.typeHash == TypeHash);
            m_TypeHash_AppCnt_map[TypeHash]++;

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

        bool processEventInputRandom(int _ratio)
        {

            int dice_roll = G_distribution(G_generator); 

            StreamEvent event;

            if (!event.read())
                return false;

            event.attributes[Query::DA_ZERO] = 0;
            event.attributes[Query::DA_MAX] = numeric_limits<attr_t>::max();
            event.attributes[Query::DA_CURRENT_TIME] = current_utime();
            event.attributes[Query::DA_OFFSET] = m_DefAttrOffset;
            event.attributes[Query::DA_ID] = m_DefAttrId;

            const EventDecl* decl = m_Definition.eventDecl(event.typeIndex);
            uint16_t TypeHash = StreamEvent::hash(decl->name);
            assert(event.typeHash == TypeHash);

            if(dice_roll <=  _ratio)
                ++m_NumSheddingEvent;
            else
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

        bool processEventInputSelective()
        {

            int dice_roll = G_distribution(G_generator); 

            StreamEvent event;

            if (!event.read())
                return false;

            event.attributes[Query::DA_ZERO] = 0;
            event.attributes[Query::DA_MAX] = numeric_limits<attr_t>::max();
            event.attributes[Query::DA_CURRENT_TIME] = current_utime();
            event.attributes[Query::DA_OFFSET] = m_DefAttrOffset;
            event.attributes[Query::DA_ID] = m_DefAttrId;

            const EventDecl* decl = m_Definition.eventDecl(event.typeIndex);
            uint16_t TypeHash = StreamEvent::hash(decl->name);
            assert(event.typeHash == TypeHash);

            if(dice_roll <=  TypeIndexHashMap[TypeHash])
                ++m_NumSheddingEvent;
            else
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

        bool processEventInputHybrid()
        {

            int dice_roll = G_distribution(G_generator); 

            StreamEvent event;

            if (!event.read())
                return false;

            event.attributes[Query::DA_ZERO] = 0;
            event.attributes[Query::DA_MAX] = numeric_limits<attr_t>::max();
            event.attributes[Query::DA_CURRENT_TIME] = current_utime();
            event.attributes[Query::DA_OFFSET] = m_DefAttrOffset;
            event.attributes[Query::DA_ID] = m_DefAttrId;

            const EventDecl* decl = m_Definition.eventDecl(event.typeIndex);
            uint16_t TypeHash = StreamEvent::hash(decl->name);
            assert(event.typeHash == TypeHash);

            if(!TypeIndexHashMap.count(TypeHash))
                ++m_NumSheddingEvent;
            else
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


        void dumpLatencyBooking(string file)
        {
            ofstream outFile;
            outFile.open(file.c_str());

            for(auto iter : m_Latency_booking)
                outFile << iter.first << "," << iter.second <<  endl;

            outFile.close();
        }

        void dumpAttrBooking(string file)
        {

            multimap<uint64_t, uint64_t> sortedList;
            multimap<double, uint64_t> rankingTable;

            int i = 0;
            for(auto &&s : m_Matcher.m_States)
            {

                if(s.Contribution.size() == 0 && s.Consumption.size()==0)
                {
                    ++i;
                    continue;
                }

                string fileIter = file + "_" + to_string(i) + ".csv";
                ofstream outFile;
                outFile.open(fileIter.c_str());

                outFile << "NumFullMatch : " << NumFullMatch << endl;
                outFile << "===============================" << endl;
                for(auto iter: s.Consumption)
                {
                    if(s.Contribution.count(iter.first)>0)
                        rankingTable.insert(make_pair(double(s.Contribution[iter.first])/double(iter.second), iter.first));
                    else
                        m_ZeroContributionPMmap[i] += iter.second;
                    m_ConsumptionMap[i] += iter.second;
                }

                outFile << m_ZeroContributionPMmap[i] << endl;
                outFile << m_ConsumptionMap[i] << endl;

                for(auto iter: rankingTable)
                    outFile << iter.first << "," <<  iter.second << "," << s.Consumption[iter.second] << "," << s.Contribution[iter.second] << endl;
                rankingTable.clear();

                outFile.close();
                ++i;
            }
        }

        void printMatcher()
        {
            m_Matcher.print();
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
            {
                *outattr_it++ = _attributes[it];
                cout << *outattr_it << " " << flush;
            }
            cout << endl;
                

            uint64_t latency = _attributes[Query::DA_FULL_MATCH_TIME] - _attributes[Query::DA_CURRENT_TIME]; 
            m_Latency_booking[latency]++;
            m_attr_booking[_attributes[1]]++;
            for(auto &&s: m_Matcher.m_States)
                if(s.ConsumptionIdx && s.ConsumptionIdx < m_ResultAttributeCount)
                    s.Contribution[_attributes[s.ConsumptionIdx]]++;

            //r.write();
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

        map<uint64_t, uint64_t> m_Latency_booking;
        map<uint64_t, uint64_t> m_attr_booking;
        map<uint16_t, uint64_t> m_TypeHash_AppCnt_map;
        map<uint16_t, string>   m_TypeHash_String_map;
        map<int, uint64_t>      m_ZeroContributionPMmap;
        map<int, uint64_t>      m_ConsumptionMap;
        uint64_t            m_NumSheddingEvent = 0;

        map<uint16_t, int> TypeIndexHashMap;
        const int T_GSubmit = 0;
        const int T_GSchedule = 1;
        const int T_GFail = 2;
        const int T_GEvict = 3;
};

int main(int _argc, char* _argv[])
{
    init_utime();

    const char* deffile = "default.eql";
    const char* queryName = 0;
    const char* monitorFile = 0;
    const char* miningPrefix = 0;
    const char* Suffix = 0;
    bool captureTimeouts = false;
    bool appendTimestamp = false;

    bool FLAG_PM_random_shedding = false;
    bool FLAG_Input_random_shedding =  false;
    bool FLAG_Input_selective_shedding =  false;
    bool FLAG_PM_selective_shedding =  false;
    bool FLAG_Hybrid_shedding =  false;

    double sheddingRatio = 0;
    string sheddingFlagName = "none";

    int num_fetchWoker = 5; 
    uint64_t cacheSize = 5000;
    uint64_t eventCnt = 1000000;


    int c;
    while ((c = getopt(_argc, _argv, "c:q:p:m:n:r:f:L:Z:C:u:tsABDEb")) != -1)
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
            case 'n':
                Suffix = optarg;
                sheddingFlagName = string(optarg);
                break;
            case 'r':
                sheddingRatio = stod(string(optarg));
                break;
            case 't':
                captureTimeouts = true;
                break;
            case 's':
                appendTimestamp = true;
                break;
            case 'u':
                PatternMatcher::Transition::prefetchFrequency = stoi(string(optarg));
                break;
            case 'A':
                PatternMatcher::Transition::FLAG_prefetch = true;
                break;
            case 'B':
                PatternMatcher::Transition::FLAG_delay_fetch = true;
                break;
            case 'b':
                PatternMatcher::Transition::FLAG_baseline = true;
                break;
            case 'C':
                cacheSize = stoull(string(optarg));
                break;
            case 'D':
                eventCnt = stoi(string(optarg));
                break;
            case 'f':
                num_fetchWoker = stoi(string(optarg));
                break;
            case 'L':
                PatternMatcher::Transition::fetch_latency = stoi(string(optarg));
                FetchWorker::fetch_latency = stoi(string(optarg));
                break;
            case 'E':
                FLAG_Hybrid_shedding = true;
                break;
            default:
                abort();
        }
    }

    FetchWorker* fetchWorker = new FetchWorker[num_fetchWoker];
    Cache cache(cacheSize, fetchWorker, num_fetchWoker);
    unordered_map<uint64_t, uint64_t> UtilityMap; 
    cache.setUtilityMap(&UtilityMap);


    CepMatch prog;
    if (!prog.init(deffile, queryName, miningPrefix, captureTimeouts, appendTimestamp))
        return 1;

    for(auto && s : prog.m_Matcher.m_States)
    {
        s.setCache(&cache, num_fetchWoker);
        s.setUtilityMap(&UtilityMap);
    }

    cache.start();
    for(int i=0; i<num_fetchWoker; ++i)
        fetchWorker[i].start();


    if(PatternMatcher::Transition::FLAG_prefetch)
    {

        prog.m_Matcher.m_States[2].setExternalIndex(2);
        prog.m_Matcher.m_States[3].setExternalIndex(2);
        prog.m_Matcher.m_States[4].setExternalIndex(2);
        //    prog.m_Matcher.m_States[5].setExternalIndex(2);
        //    prog.m_Matcher.m_States[6].setExternalIndex(2);
    }
    prog.m_Matcher.m_States[5].setExternalIndex(2);
    prog.m_Matcher.m_States[5].setExternalComIndex(3);
    prog.m_Matcher.m_States[5].setExternalFlagIndex(2);

    if(PatternMatcher::Transition::FLAG_delay_fetch)
    {
        prog.m_Matcher.m_States[6].setExternalIndex(2);
        prog.m_Matcher.m_States[6].setExternalComIndex(3);
        prog.m_Matcher.m_States[6].setExternalFlagIndex(2);

        prog.m_Matcher.m_States[7].setExternalIndex(2);
        prog.m_Matcher.m_States[7].setExternalComIndex(3);
        prog.m_Matcher.m_States[7].setExternalFlagIndex(2);
    }

    volatile uint64_t eventCounter = 0;
    //************setting up monitoring thread for throughput ***********
    MonitorThread monitor;
    monitor.addValue(&current_utime);
    monitor.addValue(&eventCounter, true);

    if (monitorFile)
        monitor.start(monitorFile);
    //*******************************************************************

    cout << "CEP engine working... " << endl;

    while(prog.processEvent())
        eventCounter++;


    string latencyFile = string("Latency_") + sheddingFlagName + ".csv"; 
    prog.dumpLatencyBooking(latencyFile);

    prog.update_miner();

    string runInforFile = "RunInfor_" + sheddingFlagName + ".txt";

    ofstream runInforOutFile;
    runInforOutFile.open(runInforFile.c_str());
    if(!runInforOutFile.is_open())
        cout << "[ERROR!] can't open file : "<< runInforFile << endl;

    runInforOutFile << "#events : " << eventCounter << endl;
    runInforOutFile << "#FullMatches " << NumFullMatch << endl;
    runInforOutFile << "#PMs         " << NumPM << endl;
    int i = 0;
    runInforOutFile.close();
    cout << "fetcher, cache worker done " << endl;
    exit(0);
    return 0;
}
