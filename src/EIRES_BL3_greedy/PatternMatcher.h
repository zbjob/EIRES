#pragma once
//#include "pythonCallerWrapper.hpp"
#include <inttypes.h>
#include <vector>
#include <deque>
#include <functional>
#include <map>
#include <unordered_map>
#include "Cache_ordered.h"
#include "FetchWorker.h"
//#include <chrono>

typedef int64_t attr_t;
//typedef double attr_t;
typedef int64_t acceptCounter_t;
using namespace std;

//uint64_t pack(const uint32_t& lhs, const uint32_t& rhs)
//{
//    uint64_t val = lhs;
//    val <<=32;
//    val |= rhs;
//    return val;
//}
//
//std::pair<uint32_t, uint32_t> unpack(const uint64_t& val)
//{
//    return {val >>32, val &0x00000000ffffffff};
//}

struct BufferIdx
{
    int timesliceID;
    int clusterID;
    uint32_t bufferID;

    BufferIdx(int t, int c, int b):timesliceID(t),clusterID(c),bufferID(b){}

};

class PatternMatcher
{
public:
    static bool loadMonitoringFlag  ;
    //static bool loadMonitoringFlag; 
    
    std::string partialMatchOutPutFilePrefix;
	enum Operator
	{
		OP_LESS,
		OP_LESSEQUAL,
		OP_GREATER,
		OP_GREATEREQUAL,
		OP_EQUAL,
		OP_NOTEQUAL,
		OP_MAX,
        OP_ADD
	};
	enum StateType
	{
		ST_NORMAL,
		ST_ACCEPT,
		ST_REJECT,

		ST_MAX
	};
	enum CallbackType
	{
		CT_INSERT,
		CT_TIMEOUT
	};
	enum
	{
		MAX_ATTRIBUTES = 32
	};

	struct OperatorInfo
	{
		const char* name;
		const char* sign;
	};

	class AggregationFunction
	{
	public:
		virtual ~AggregationFunction() {}

		virtual void clear() = 0;
		virtual attr_t push(attr_t) = 0;
		virtual void pop(attr_t) = 0;
	};

	PatternMatcher();
	~PatternMatcher();

    
    attr_t TTL = 0;
    void setTTL(int _ttl) {TTL = _ttl;}
	// setup
	virtual void setTimeout(attr_t _timeout) { m_Timeout = _timeout; }

	virtual void addState(uint32_t _idx, uint32_t _numAttributes, StateType _type = ST_NORMAL, uint32_t _kleenePlusAttrIdx = 0);
	virtual void addState(uint32_t _idx, int _numTimeSlice, int _numCluser, uint32_t _numAttributes, StateType _type = ST_NORMAL, uint32_t _kleenePlusAttrIdx = 0);
	virtual void setCallback(uint32_t _state, CallbackType _type, const std::function<void(const attr_t*)>& _function);
	virtual void addAggregation(uint32_t _state, AggregationFunction* _function, uint32_t _srcAttr, uint32_t _dstAttr);
	virtual void addTransition(uint32_t _fromState, uint32_t _toState, uint32_t _eventType);
	virtual void addPrecondition(uint32_t _param, attr_t _constant, Operator _op); // preconditions get checked before conditions and match = event[param1] op constant

	// match = state[param1] operator (event[param2] + _constant) OR
	// match = event[param1] operator (event[param2] + _constant) in case of _postAggregationCheck
	virtual void addCondition(uint32_t _param1, uint32_t _param2, Operator _op, attr_t _constant, bool _postAggregationCheck = false);
	virtual void addActionCopy(uint32_t _src, uint32_t _dst);
	void resetRuns();

	// process
	virtual uint32_t event(uint32_t _type, const attr_t* _attr);

	// query results
	uint32_t numRuns(uint32_t _stateId) const										{ return m_States[_stateId].stateBufferCount; }
	//size_t numAttributes(uint32_t _stateId) const									{ return m_States[_stateId].attr.size(); }
	size_t numAttributes(uint32_t _stateId) const									{ return m_States[_stateId].buffers[0][0].size(); }
	void run(uint32_t _stateId, uint32_t _runIdx, attr_t* _attrOut) const;

	void clearState(uint32_t _stateId);

    //for loadShedding learning
    void setStates2Transitions();
    void setStates2States();
    void computeScores4LoadShedding();
    void computeScores4LoadShedding_VLDB16();
    void loadShedding();
    uint64_t loadShedding_VLDB16(uint64_t quota, int state1, int state2);

    //uint32_t PMloadShedding(uint32_t stateID);
    void randomLoadShedding();
    void fixNumLoadShedding();
    bool compareRunScore(const std::pair<attr_t, double> & lhs, const std::pair<attr_t, double> & rhs) { return lhs.second < rhs.second;}
    static void setMonitoringLoadOn() {loadMonitoringFlag = true;}
    static void setMonitoringLoadOff() {loadMonitoringFlag = false;}
    static bool MonitoringLoad() { return loadMonitoringFlag;}

   // void MonitorLatency(std::chrono::time_point t) {g_BeginClock = g;}
    void print();

    //void setPythonLearning(string _pFile, string _pFunc);
    void setTimeSliceSpan(uint64_t);

    uint64_t clustering_classification_PM_shedding(int _stateID, int _timeSliceID, int clusterID, double _quota=1 );
    uint64_t clustering_classification_PM_shedding(); //shedding based on the pre=defined latency threshold
    void     clustering_classification_PM_shedding_4_typeC(int _state, int lowerBound, int upperBound);
    void     clustering_classification_PM_shedding_random(int _state, double _ratio);
    void     clustering_classification_PM_shedding_selectivity(double _ratio);
    void     clustering_classification_PM_shedding_semantic(double _ratio);
    void     clustering_classification_PM_shedding_semantic_setPMSheddingCombo(int _combo);
    uint64_t clustering_classification_PM_random_shedding(uint64_t _quota);
    uint64_t clustering_classification_PM_random_shedding(int _stateID, int _timeSliceID, int _clusterID, uint64_t _quota);

    void printContributions();




	static const OperatorInfo& operatorInfo(Operator _op);

    void setPartialMatchOutPutFilePrefix(std::string str) { partialMatchOutPutFilePrefix = str;}

    void addClusterTag(int stateID, int tsID, int clusterID, long double contribution, long double consumption);

    void sortClusterTag();

    uint64_t approximate_BKP_PMshedding(double LOF);
    uint64_t DP_BKP_PMshedding(double LOF);
//protected:
	struct Condition
	{
		uint32_t	param[3];
		Operator	op;
        Operator    op2;
		attr_t		constant;

        Condition(){
            param[0] = 0;
            param[1] = 0;
            param[2] = 0;
        }
	};

	struct CopyAction
	{
		uint32_t	src;
		uint32_t	dst;
	};

	struct Aggregation
	{
		AggregationFunction*	function;
		uint32_t				srcAttr;
		uint32_t				dstAttr;
	};


    struct ClusterTag
    {
        int stateID;
        int timeSliceID;
        int clusterID;
        long double contribution;
        long double consumption;
        double   ccRatio;

        uint64_t size;
        uint64_t TotalConsumption;
        

        ClusterTag(int s, int ts, int c, long double cont, long double cons) 
        {
            stateID = s;
            timeSliceID = ts;
            clusterID = c;
            contribution = cont; 
            consumption = cons;
            ccRatio = contribution/consumption;
            size = 0;
        }


    };

	struct State
    {
		State();
		~State();

		void setCount(uint32_t _count);
		void setAttributeCount(uint32_t _count);
		void setIndexAttribute(uint32_t _idx);

		void insert(const attr_t* _attributes);
		void remove(int t, int c, uint32_t _index) { remove_list.push_back(BufferIdx(t,c,_index)); }
		void endTransaction();

		void removeTimeouts(attr_t _value);

		void attributes(int _timeslice, int _cluster, uint32_t _idx, attr_t* _out) const;
		//bool runValid(uint32_t _idx) const { return attr.empty() || attr.front()[_idx] > timeout; }
		bool runValid(int _timeslice, int _cluster, uint32_t _idx) const { return buffers[_timeslice][_cluster].empty() || buffers[_timeslice][_cluster].front()[_idx] > timeout; }

        //void loadShedding();


		//const std::unordered_multimap<attr_t, uint64_t>* indexMap(uint32_t _attributeIdx) const { return _attributeIdx == index_attribute ? &index : NULL; }
		const std::multimap<attr_t, uint64_t>* indexMap(int cluster, int timeslice, uint32_t _attributeIdx) const { return _attributeIdx == index_attribute ? &index[cluster][timeslice] : NULL; }
        
        void setTimesliceClusterAttributeCount(int t, int c, int _count);

        //void setUpPythonCaller(string _pFile, string _pFunc){
        //  cout << "[PatternMatcher::State::setUpPythonCaller]  flag1 " << endl; 
        //  PythonCallerInstance.set(_pFile, _pFunc); 
        //}

        void addClusterAttrIdx(int idx) {ClusterAttrIdxs.push_back(idx);}
        void setTimeSliceSpan(uint64_t timewindow) { timeSliceSpan = (timewindow+1)/numTimeSlice;}
        uint64_t pack(const uint32_t& lhs, const uint32_t& rhs)
        {
            uint64_t val = lhs;
            val <<=32;
            val |= rhs;
            return val;
        }

        void setCache(Cache * _cache, int _num) {m_Cache = _cache; m_num_FetchWorker = _num;}
        void setUtilityMap(unordered_map<uint64_t, uint64_t> * _map) {m_KeyUtility = _map;}
        void setExternalIndex(int _idx) {m_externalIdx = _idx;}
        void setExternalComIndex(int _idx) {m_externalComIdx = _idx;}
        void setExternalFlagIndex(int _idx) {m_externalFlagIdx = _idx;}




		StateType	type;
		uint32_t	kleenePlusAttrIdx;
		uint32_t	buffers_count;
        uint32_t    Kleene_LastStateTimeStampIdx;

        attr_t      Kleene_lastTimeStamp;
		//uint64_t	firstMatchId;  // only used to maintain the multiMap index ?
        uint32_t stateBufferCount;
        std::vector<std::vector<uint32_t> >	count;
        std::vector<std::vector<uint64_t> >	firstMatchId;  // only used to maintain the multiMap index ?
		//std::vector<std::deque<attr_t> > attr;
        std::vector<std::vector<std::vector<std::deque<attr_t> > > > buffers;
		std::vector<Aggregation> aggregation;

		std::function<void(const attr_t*)> callback_timeout;
		std::function<void(const attr_t*)> callback_insert;

        // for learning contributions
        std::vector<int> ClusterAttrIdxs;

        vector<uint32_t>    KeyAttrIdx; 
        int         timePointIdx=0;
        acceptCounter_t   accNum;
        acceptCounter_t   transNum;
        //std::unordered_map<std::pair<attr_t, attr_t>, acceptCounter_t> tranCounter;
        std::map<std::pair<attr_t, attr_t>, acceptCounter_t> tranCounter;
        std::unordered_map<attr_t,acceptCounter_t> valAccCounter;
        std::vector<std::pair<attr_t,double>> rankingTable;
        std::unordered_map<attr_t, acceptCounter_t> contributions;
        std::unordered_map<attr_t, acceptCounter_t> consumptions;
        std::vector<std::pair<attr_t, double>>  scoreTable; 
        std::vector<std::pair<attr_t, uint64_t>>  scoreTable_VLDB16; 
        

        void addKeyAttrIdx(uint32_t _idx) {KeyAttrIdx.push_back(_idx);}

	//private:
		//std::vector<uint32_t>						remove_list; // execution reject, remove negate event
		std::vector<BufferIdx>						remove_list; // execution reject, remove negate event
		uint32_t									index_attribute;
		//std::unordered_multimap<attr_t, uint64_t>				index;
		//std::multimap<attr_t, uint64_t>				index;
        std::vector<std::vector<std::multimap<attr_t, uint64_t> > >			index;
        //std::vector<std::vector<std::unordered_multimap<attr_t, uint64_t> > >			index;


        //for contribution learning, to minic VLDB'16 paper
        std::vector<std::vector<std::multimap<attr_t, uint64_t> > >			KeyAttributeIndex;
		//std::multimap<attr_t, uint64_t>				KeyAttributeIndex;
    
		attr_t										timeout = 0; // the possible earlist timestamp. The lower bound of a living time window. 
        uint32_t                                    ID;
        std::vector<State>*                         states = NULL;
        int                                         numTimeSlice=1;
        uint64_t                                    timeSliceSpan=1;

        int TypeClowerBound = 0;
        int TypeCupperBound = 0;
        int PMDiceUB = 0;
        int SelectivityPMDiceUB = 0;

        int PMSmDropRatio = 0;
        int PMSheddingCombo = 0;

        bool PMKeepingBook[21];

        //Prefetching members

        Cache * m_Cache;
        unordered_map<uint64_t, uint64_t>*  m_KeyUtility;
        
        int     m_num_FetchWorker;

        int     m_externalIdx;
        int     m_externalComIdx;
        int     m_externalFlagIdx;
        //FetchWorker* m_FetchWorker;

        //PCWrapper                                   PythonCallerInstance;


	};

	struct Transition
	{
		uint32_t	eventType;
		uint32_t	from;
		uint32_t	to;

		std::vector<Condition>	preconditions;	// checked per incoming event
		std::vector<Condition>	conditions;		// checked per run per incoming event
		std::vector<Condition>	postconditions;	// checked per kleene result post aggregation
		std::vector<CopyAction>	actions;
        std::vector<State>* states = NULL;


		void updateHandler(const State& _from, const State& _to);
		void setCustomExecuteHandler(std::function<void(State&, State&, int, int, uint32_t, const attr_t*)> _handler);

		uint32_t checkEvent(State& _from, State& _to, size_t _runOffset, const attr_t* _attr);
	//private:
		uint32_t	(Transition::*checkForMatch)(State&, State&, size_t, const attr_t*);
		void		(Transition::*executeMatch)(State&, State&, int, int, uint32_t, const attr_t*);

		void executeTransition(State& _from, State& _to, int timesliceId, int clusterId, uint32_t _idx, const attr_t* _attributes);
		void executeReject(State& _from, State& _to, int timesliceId,int clusterId, uint32_t _idx, const attr_t* _attributes);
		void executeCustom(State& _from, State& _to, int timesliceId, int clusterId, uint32_t _idx, const attr_t* _attributes);

		uint32_t checkNoCondition(State& _from, State& _to, size_t _runOffset, const attr_t* _eventAttr);
		uint32_t checkSingleCondition(State& _from, State& _to, size_t _runOffset, const attr_t* _eventAttr);
		uint32_t checkMultipleCondotions(State& _from, State& _to, size_t _runOffset, const attr_t* _eventAttr);

        void updateContribution( State& _from, State& _to, uint32_t idx, attr_t valFrom, attr_t valTo, attr_t * _attributes);

		
		// there are two batch testing methods. the first checks every run in _state and appends every match to _matchOut. The second method checks every run in _matchOut starting at _matchOffsets and removes rejects.
		static void testCondition(const Condition& _condition, const State& _state, size_t _runOffset, const attr_t* _attr, std::function<void(int, int , uint32_t)> _callback);

		// kleene member
		uint32_t checkKleene(State& _from, State& _to, size_t _runOffset, const attr_t* _eventAttr);
		void executeKleene(State& _from, State& _to, int _timeslice, int _cluster, uint32_t _idx, const attr_t* _attributes);

		uint32_t	(Transition::*checkForMatchOrg)(State&, State&, size_t, const attr_t*);
		void		(Transition::*executeMatchOrg)(State&, State&, int, int, uint32_t, const attr_t*);

        static int fetchCnt;
        static int prefetchCnt;
        static int prefetchFrequency;
        static int fetchProbability;
        static int consumptionPolicy;
        static int fetch_latency;
        static bool FLAG_delay_fetch;
        static bool FLAG_prefetch;
        



        uint64_t pack(const uint32_t& lhs, const uint32_t& rhs)
        {
            uint64_t val = lhs;
            val <<=32;
            val |= rhs;
            return val;
        }



		//typedef std::vector<std::pair<uint32_t, attr_t> > runs_t;
        struct runs_element{
            int _timeslice;
            int _cluster;
            uint32_t _idx;
            attr_t _attr;

            runs_element(int t, int c, uint32_t id, attr_t a): _timeslice(t), _cluster(c), _idx(id), _attr(a){}
        };

        typedef std::vector<runs_element> runs_t;

		runs_t runs;

        std::vector<std::pair<size_t,size_t> > run_range;

        size_t run_lastPos=0;

		std::function<void(State&, State&, int, int, uint32_t, const attr_t*)>	m_CustomExecuteHandler;
	};

	void testCondition(const Condition& _condition, const State& _state, const attr_t* _attr, uint32_t _matchOffset, std::vector<uint32_t>& _matchOut);

//protected:
    public:

	std::vector<State> m_States;
	std::vector<Transition> m_Transitions;
    vector<ClusterTag>  m_ClusterTags;
    int  m_CntCall_AppKNP_solver = 0;


    bool latencyFlag;

	attr_t		m_Timeout; //time windows for CEP 
    bool Rflag;

    bool m_ClusterTags_sorted = false;
    
    int  typeC_keeping_value = 0;
    int  typeC_keeping_value_UB = 0;
    int  typeC_keeping_value_LB = 0;

};


inline bool checkCondition(attr_t _p1, attr_t _p2, PatternMatcher::Operator _op)
{
	switch (_op)
	{
        case PatternMatcher::OP_LESS:
            return _p1 < _p2;
        case PatternMatcher::OP_LESSEQUAL:
            return _p1 <= _p2;
        case PatternMatcher::OP_GREATER:
            return _p1 > _p2;
        case PatternMatcher::OP_GREATEREQUAL:
            return _p1 >= _p2;
        case PatternMatcher::OP_EQUAL:
            return _p1 == _p2;
        case PatternMatcher::OP_NOTEQUAL:
            return _p1 != _p2;
	}
    return false;
}
