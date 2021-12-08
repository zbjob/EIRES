#include "../query/Query.h"
#include "../query/PatternMatcher.h"
#include "../query/Cache_ordered.h"
#include "../query/FetchWorker.h"
#include "../event/EventStream.h"
#include "../_shared/PredicateMiner.h"
#include "../_shared/MonitorThread.h"
#include "../_shared/GlobalClock.h"
#include "../utils/freegetopt/getopt.h"
#include "../event/SatelliteEvent.h"
#include <vector>
#include <chrono>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <queue>
#include <string>
#include <random>

using namespace std;
using namespace std::chrono;

time_point<high_resolution_clock> g_BeginClock;
uint64_t  NumFullMatch = 0;
uint64_t  NumHighLatency = 0;
uint64_t  NumPartialMatch = 0;
uint64_t  NumSheddingPM = 0;
uint64_t ACCLantency = 0;

int PatternMatcher::Transition::fetchCnt = 0;
int PatternMatcher::Transition::prefetchCnt = 0;
int PatternMatcher::Transition::fetchProbability= 100;
int PatternMatcher::Transition::fetch_latency = 10;
bool PatternMatcher::Transition::FLAG_prefetch= false;
bool PatternMatcher::Transition::FLAG_delay_fetch = false;
bool PatternMatcher::Transition::FLAG_baseline = false;
int FetchWorker::fetch_latency = 1;

default_random_engine m_generator;
uniform_int_distribution<int> m_distribution(1,100);

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

    bool processSatelliteEvent()
    {
        StreamEvent event;

        SatelliteEvent RawEvent;
        if(!RawEventQueue.empty())
        {
            RawEvent = RawEventQueue.front();
            RawEventQueue.pop();
        }
        else{
            cout << "event stream queue is empty now!" << endl;
            return false;
        }

        event.attributes[0] = attr_e(RawEvent.time);
        event.attributes[1] = attr_e(RawEvent.channelID);
        event.attributes[2] = attr_e(RawEvent.level);
        event.attributes[3] = attr_e(RawEvent.boundary);

        event.attributes[4] = attr_e(RawEvent.humidity);
        event.attributes[5] = attr_e(RawEvent.temperature);
        
        uint64_t t = 86400000000;
        uint64_t ts= 300000000;
        const uint64_t one_min = 60*1000*1000;

		event.attributes[Query::DA_ZERO] = attr_e((attr_t)0);
		event.attributes[Query::DA_MAX] = attr_e(numeric_limits<attr_t>::max());
		event.attributes[Query::DA_CURRENT_TIME] = attr_e((attr_t)current_utime());
		event.attributes[Query::DA_OFFSET] = attr_e((attr_t)m_DefAttrOffset);
		event.attributes[Query::DA_ID] = attr_e((attr_t)m_DefAttrId);

		const EventDecl* decl = m_Definition.eventDecl(event.typeIndex);

		m_Matcher.event(event.typeIndex, event.attributes);

        if(event.attributes[0].i > Tick_PM_one_min + one_min) 
        {
            m_one_min_cnt++;
            Tick_PM_one_min = event.attributes[0].i;
            Tick_PM_Cnt = NumPartialMatch;
        }
        return true;
    }

    bool readEventStreamFromFiles(string file,bool header = true)
    {
       ifstream ifs;
       ifs.open(file.c_str());
       if( !ifs.is_open())
       {
           cout << "can't open file " << file << endl;
           return false;
       }
       string line;
       if(header)
           getline(ifs,line);
       while(std::getline(ifs,line))
       {
           vector<string> dataEvent;
           stringstream lineStream(line);
           string cell;
           string polygon_;

           while(getline(lineStream,cell,','))
           {
               if(cell.find("POLYGON") != string::npos){
                   polygon_ = cell + ",";
                   while(getline(lineStream,cell,',')){
                       polygon_ += cell.erase(0,1);
                       if(cell.find("))") != string::npos)
                           break;
                       else 
                           polygon_ += ",";
                   }
                   polygon_ = polygon_.erase(0,1);
                   polygon_ = polygon_.erase(polygon_.length()-1,1);

                   dataEvent.push_back(polygon_);
               } else {
                   dataEvent.push_back(cell);
               }
           }

            SatelliteEvent RawEvent;

            RawEvent.setTime(dataEvent[0]);
            RawEvent.setChannelID(dataEvent[1]);
            RawEvent.setLevel(dataEvent[2]);
            RawEvent.setBoundary(dataEvent[3]);
            RawEvent.setHumidity(dataEvent[4]);
            RawEvent.setTemperature(dataEvent[5]);
            RawEvent.setPressure(dataEvent[6]);
            RawEventQueue.push(RawEvent);
        }
        return true;
    }

    uint64_t RandomInputShedding(double ratio, volatile uint64_t & _eventCnt) 
    {
        uint64_t ShedCnt = 0;

        while(!RawEventQueue.empty())
        {
            ++_eventCnt;
            int dice_roll = m_distribution(m_generator);
            if(dice_roll <=  (ratio *100) )
            {
                RawEventQueue.pop();
                ++ShedCnt;
            }
            else
                processSatelliteEvent();
        }
        return ShedCnt;
    }

    uint64_t SelectivityInputShedding(double _ratio, volatile uint64_t & _eventCnt)
    {
        int ratio = _ratio*100;
        uint64_t ShedCnt = 0;
        while(!RawEventQueue.empty())
        {
            ++_eventCnt;
            int dice_roll = m_distribution(m_generator);
            processSatelliteEvent();

        }
        return ShedCnt;
    }

    uint64_t HybridIrrelevantInputShedding(volatile uint64_t & _eventCnt)
    {
        uint64_t ShedCnt = 0;
        while(!RawEventQueue.empty())
        {
            ++_eventCnt;
            processSatelliteEvent();
        }
        return ShedCnt;
    }

    bool readPMKeepBookings(string file)
    {
       ifstream ifs;
       ifs.open(file.c_str());
        if( !ifs.is_open())
        {
            cout << "can't open file " << file << endl;
            return false;
        }

        string line;

        while(getline(ifs,line))
        {
            vector<string> dataEvent;
            stringstream lineStream(line);
            string cell;

            while(getline(lineStream,cell,','))
            {
                dataEvent.push_back(cell);
            }
            
            int stateID         =  stoi(dataEvent[0]);
            int timeSliceID     =  stoi(dataEvent[1]) - 1;
            int PMkey           =  stoi(dataEvent[2]);
        }
    }

    void printContribution()
    {
        for(auto&& it: m_Matcher.m_States)
        {
            std::cout << endl << "state contributions size == " << it.contributions.size() << endl;
            std::cout << endl << "state consumptions size == " << it.consumptions.size() << endl;
            std::cout << endl << "state scoreTable size == " << it.scoreTable.size() << endl;
        }
    }

    void dumpLatencyBooking(string file)
    {
        ofstream outFile;
        outFile.open(file.c_str());

        for(auto iter : m_Latency_booking)
            outFile << iter.first << "," << iter.second <<  endl;
    }

	void update_miner()
	{
		if (!m_Miner)
			return;

		string eqlFilename = m_MiningPrefix + ".eql";
		string scriptFilename = m_MiningPrefix + ".sh";

		QueryLoader dst = m_Definition;
		for (size_t i = 0; i < m_Query->events.size() - 1; ++i)
		{
			m_Miner->printResult(i * 2, i * 2 + 1);
			// auto options = m_Miner->generateResult(i * 2, i * 2 + 1);
			// for (auto& it : options)
			// {
			// 	Query q = m_Miner->buildPredicateQuery(dst, *m_Query, (uint32_t)i, it);

			// 	ostringstream namestr;
			// 	namestr << q.name << "_mined_" << i << '_' << it.eventId;
			// 	for (size_t i = 0; i < it.numAttr; ++i)
			// 		namestr << '_' << it.attrIdx[i];

			// 	q.name = namestr.str();

			// 	script << "$CEP_CMD -c " << eqlFilename << " -q " << q.name << " < $CEP_IN > ${CEP_OUT}" << q.name << " &" << endl;
			// 	dst.addQuery(move(q));
			// }
		}
		// dst.storeFile(eqlFilename.c_str());
	}

//protected:
    int hot = 0;
	void write_event(bool _timeout, uint32_t _state, const attr_e* _attributes)
	{
		StreamEvent r;
        cout << "Pattern Matcher : " << ++hot << endl;
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
			r.attributes[r.attributeCount++] = attr_e((attr_t)current_utime());
		}

		attr_e* outattr_it = r.attributes;
        string hotPath;
        bool inOperation = false;

        uint32_t indexOp = 0;

        for(uint32_t it=0; it<m_OutEventAttrSrc.size(); ++it){
			*outattr_it++ = _attributes[it];

            if(m_Query->returnOp.size() > 0){
                uint32_t f_ = m_Query->returnOp[indexOp].rangeOp.first;
                uint32_t s_ = m_Query->returnOp[indexOp].rangeOp.second;

                if(it == f_){
                    switch (m_Query->returnOp[indexOp].operation)
                    {
                        case 0:
                        {
                            std::deque<poly_t> output = attr_e::intersect(_attributes[m_OutEventAttrSrc[f_]],_attributes[m_OutEventAttrSrc[f_+1]]);;
                            for(uint32_t i = f_ + 2; i <= s_; i++){
                                if(output.size() > 0){
                                    std::ostringstream ostr;
                                    ostr << bg::wkt(output.at(0));
                                    output = attr_e::intersect(attr_e(ostr.str()),_attributes[m_OutEventAttrSrc[i]]);
                                }
                            }
                            if(output.size()>0){
                                cout << endl;
                                for(uint32_t i=0;i<output.size();i++)
                                    cout <<  "boundary: " << bg::wkt(output.at(i)) << endl;
                                break;
                            }
                        }
                        default:
                            break;
                    }
                }
                if(it >= f_ && it <= s_){
                    if(it == s_)
                        indexOp++;
                    continue;
                }
            }
            if(_attributes[m_OutEventAttrSrc[it]].tag == attr_e::INT64_T)
                cout << to_string( _attributes[m_OutEventAttrSrc[it]].i) + ",";
            else if(_attributes[m_OutEventAttrSrc[it]].tag == attr_e::DOUBLE)
                cout << to_string( _attributes[m_OutEventAttrSrc[it]].d) + ",";
            else
                cout << endl <<  "boundary: " << bg::wkt(_attributes[m_OutEventAttrSrc[it]].poly) << endl;
        }
        cout << endl;

        uint64_t lateny = _attributes[Query::DA_FULL_MATCH_TIME].i - _attributes[Query::DA_CURRENT_TIME].i;
         m_Latency_booking[lateny]++;
        ACCLantency += lateny;
        
        if(_attributes[Query::DA_FULL_MATCH_TIME].i - _attributes[Query::DA_CURRENT_TIME].i > 150 && _attributes[Query::DA_FULL_MATCH_TIME].i - lastSheddingTime > 3000000)
        {
            
            if(m_Matcher.latencyFlag == true)
            {
                lastSheddingTime = _attributes[Query::DA_FULL_MATCH_TIME].i; 
                ++loadCnt;
            }
            else
                m_Matcher.latencyFlag = true;
        }
        else
        {
            m_Matcher.latencyFlag = false;
        }

		if (m_Miner)
		{
			if (_timeout)
			{
				size_t idx = (_state - 1) * 2 + 1;
				uint32_t eventA = (uint32_t)r.attributes[_state].i;
				uint32_t eventB = (uint32_t)m_DefAttrId;

				m_Miner->addMatch(eventA, eventB, idx);
			}
			else
			{
				for (uint32_t s = 1; s < _state; ++s)
				{
					size_t idx = (s - 1) * 2;
					uint32_t eventA = (uint32_t)r.attributes[s].i;
					uint32_t eventB = (uint32_t)r.attributes[s + 1].i;

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
    multimap<uint32_t,uint32_t> m_OutEventAttrOp;

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

    uint64_t            Tick_PM_Cnt = 0;
    uint64_t            Tick_PM_one_min = 0;
    uint64_t            m_one_min_cnt = 1;

    queue<SatelliteEvent>   RawEventQueue;

    uint64_t            lastEventTime = 0;

    uint64_t cnt = 0;

    int loadCnt;
    map<string, attr_t> m_HotPaths;
    map<attr_t, string> m_Sation_Book;
    map<int,uint64_t>  m_Latency_booking;
    unordered_set<attr_e> eventKeepBooking;
};

int main(int _argc, char* _argv[])
{
	init_utime();

	const char* deffile = "../bushfire.eql";
	const char* queryName = 0;
	const char* monitorFile = 0;
	const char* miningPrefix = 0;
	bool captureTimeouts = false;
	bool appendTimestamp = false;

    bool SheddingIrrelevantFlag = false;
    bool RPMSFlag = false;
    bool SlPMSFlag = false;
    bool RISFlag = false;
    bool SlISFlag = false;

    double RPMSRatio = 0;
    double SlPMSRatio = 0;
    double RISRatio = 0;
    double SlISRatio = 0;

    int timeSlice = 1;
    string streamFile = "../datasets/streams/Full_Stream_Events_day_8to11FullThreshold_Polygon.csv";

    int num_fetchWoker = 3;
    uint64_t cacheSize = 1000;
    string suffix = "none";

	int c;
	while ((c = _free_getopt(_argc, _argv, "f:c:q:p:m:T:w:x:y:z:n:F:Z:L:C:tsIABb")) != -1)
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
        case 'T':
            timeSlice = stoi(string(optarg));
            break;
        case 'I':
            SheddingIrrelevantFlag = true;
            break;
        case 'w':
            RPMSFlag = true;
            RPMSRatio = stod(string(optarg));
            break;
        case 'x':
            SlPMSFlag = true;
            SlPMSRatio = stod(string(optarg));
            break;
        case 'y':
            RISFlag = true; 
            RISRatio = stod(string(optarg));
            break;
        case 'z':
            SlISFlag = true;
            SlISRatio = stod(string(optarg));
            break;
        case 'f':
            streamFile = optarg;
            break;
        case 'n':
            suffix = string(optarg);
            break;
        case 'F':
            num_fetchWoker = stoi(string(optarg)); 
            break;
        case 'Z':
            PatternMatcher::Transition::fetchProbability = stoi(string(optarg));
            break;
        case 'L':
            PatternMatcher::Transition::fetch_latency = stoi(string(optarg));
            FetchWorker::fetch_latency = stoi(string(optarg));
            break;
        case 'C':
            cacheSize = stoull(string(optarg));
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

	volatile uint64_t eventCounter = 0;

    PatternMatcher::setMonitoringLoadOff();

    attr_t _timeWindow = 1;
    _timeWindow *=60;
    _timeWindow *=60;
    _timeWindow *=1000000;

    attr_t _tsSpan = _timeWindow/timeSlice + 1;
    
    prog.m_Matcher.setTimeWindow();
    prog.m_Matcher.setTimeSliceNum(timeSlice);

    prog.readEventStreamFromFiles(streamFile);

	MonitorThread monitor;
	monitor.addValue(&current_utime);
	monitor.addValue(&eventCounter, true);
    monitor.addValue(&NumPartialMatch,false);
	monitor.addValue(&NumFullMatch, false);

	if (monitorFile)
		monitor.start(monitorFile);
    
    for(auto &&s: prog.m_Matcher.m_States)
    {
        s.sheddingIrrelaventFlag = SheddingIrrelevantFlag;
        s.timeSliceSpan = _tsSpan;

        s.setCache(&cache, num_fetchWoker);
        s.setUtilityMap(&UtilityMap);
    }
    
    cache.start(); 
    for(int i=0; i<num_fetchWoker; ++i)
        fetchWorker[i].start();

    if(PatternMatcher::Transition::FLAG_prefetch)
    {
        prog.m_Matcher.m_States[1].setExternalIndex(0);
        prog.m_Matcher.m_States[2].setExternalIndex(4);
        prog.m_Matcher.m_States[3].setExternalIndex(8);
    }

    prog.m_Matcher.m_States[3].setExternalComIndex(2);
    prog.m_Matcher.m_States[3].setExternalFlagIndex(2);

    if(RPMSFlag)
        prog.m_Matcher.setRandomPMShedding(RPMSRatio);

    if(SlPMSFlag)
        prog.m_Matcher.setSelectivityPMShedding(SlPMSRatio);

   uint64_t sheddingEventCnt = 0;

        while(prog.processSatelliteEvent())
        {
            eventCounter++;
           if(RISFlag)
               sheddingEventCnt = prog.RandomInputShedding(RISRatio, eventCounter);

           if(SlISFlag)
                sheddingEventCnt = prog.SelectivityInputShedding(SlISRatio, eventCounter);

            if(SheddingIrrelevantFlag)
                sheddingEventCnt = prog.HybridIrrelevantInputShedding(eventCounter);
        }

        std::cout << "perform " << prog.loadCnt << "loadshedding" << endl;
        std::cout << "eventCn " << eventCounter << endl;
        std::cout << "#full Match " << NumFullMatch << endl;
        std::cout << "#high latecny (> 150)" << NumHighLatency<< endl;
        std::cout << "#NumPartialMatch " << NumPartialMatch << endl;
        std::cout << "#NumDropedPM " << NumSheddingPM << endl;
        std::cout << "#NumDropedInputEvents " << sheddingEventCnt << endl;
        if(NumFullMatch)
            cout << "avg latency: " << (double) (ACCLantency/NumFullMatch) << endl;

        prog.dumpLatencyBooking("latency.csv");

        multimap<attr_t, string> SortedHotPaths;
        for(auto &a: prog.m_HotPaths)
            if(a.second > 10)
            {
                SortedHotPaths.insert( {a.second, a.first});	
            }
        for(auto &a: SortedHotPaths)
            cout << a.first << " : " << a.second << endl;

        cout << "expression size: " << expressions.size() << endl;
        for(auto& it: expressions){
            cout << "expression index: " << it.first << " string value: "<< it.second.varName << endl;
        }

        prog.m_HotPaths.clear();

        string _file = string("latency_")+suffix+".csv";
        prog.dumpLatencyBooking(_file);

        string result_file = string("results_")+suffix+".txt";

        ofstream outResultFile;
        outResultFile.open(result_file.c_str());

        outResultFile << "perform, " << prog.loadCnt << "loadshedding" << endl;
        outResultFile << "eventCn, " << eventCounter << endl;
        outResultFile << "#FM, " << NumFullMatch << endl;
        outResultFile << "#PM, " << NumPartialMatch << endl;

        outResultFile << "#high-latecny, "  << NumHighLatency<< endl;
        if(NumFullMatch)
            outResultFile << "avg-latency, " << (double) (ACCLantency/NumFullMatch) << endl; 
        outResultFile << "#cache-update, " << cache.KeyUpdateCnt << endl;
        outResultFile << "#cache-heap-update, " << cache.HeapUpdateCnt << endl;
        outResultFile << "#cache-hit, " << cache.cacheHit << endl;
        outResultFile << "#cache-miss, " << cache.cacheMiss << endl;
        outResultFile << "#block-fetch, " << cache.BlockFectchCnt << endl;

        outResultFile << "#Pcache, " << cache.FetchCached[1] << endl;
        outResultFile << "#Dcache, " << cache.FetchCached[2] << endl;
        outResultFile << "#Bcache, " << cache.FetchCached[3] << endl;

        outResultFile << "#Phit, " << cache.FetchHit[1] << endl;
        outResultFile << "#Dhit, " << cache.FetchHit[2] << endl;
        outResultFile << "#Bhit, " << cache.FetchHit[3] << endl;

        outResultFile << "#PNhit, " << cache.FetchNeverUse[1] << endl;
        outResultFile << "#DNhit, " << cache.FetchNeverUse[2] << endl;

        cout << "fetcher, cache worker done " << endl;

//        for(auto &&s: prog.m_Matcher.m_States)
//        {
//            outResultFile << s.ID << " " << s.keys.size() << endl;
//        }

//        for(int i=0; i<num_fetchWoker; ++i)
//            outResultFile << i << " " << fetchWorker[i].keys.size() << endl;
//
//        outResultFile << "cached key # " << cache.keys.size() << endl;

        exit(0);

        for(int i=0; i<num_fetchWoker; ++i) 
        {    
            fetchWorker[i].stop();
            fetchWorker[i].m_Thread.detach();
        }    
        cache.m_Thread.detach(); 
        cache.stop();

        delete [] fetchWorker;
        return 0;
}
