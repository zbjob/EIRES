#pragma once

#include <inttypes.h>
#include <vector>
#include <deque>
#include <functional>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <set>

#include "../event/EventStream.h"
#include "../utils/cparse/shunting-yard.h"
#include "Cache_ordered.h"
#include "FetchWorker.h"

typedef int64_t acceptCounter_t;

struct Expression {
		std::string varName;
		calculator cal;
		Expression(std::string var, std::string exp){
			this->varName = var;
			this->cal = calculator(exp.c_str());
		};
		attr_e getValue(attr_e a){
			TokenMap vars;
			switch (a.tag)
			{
			case attr_e::INT64_T:
				vars[this->varName] = a.i;
				return attr_e((attr_t)this->cal.eval(vars).asInt());
				break;
			default:
				vars[this->varName] = a.i;
				return attr_e((double)this->cal.eval(vars).asDouble());
				break;
			}
			return a;
		};
};

extern std::map<uint32_t,Expression> expressions;	

class PatternMatcher
{
public:
    static bool loadMonitoringFlag;

    
	enum Operator
	{
		OP_LESS,
		OP_LESSEQUAL,
		OP_GREATER,
		OP_GREATEREQUAL,
		OP_EQUAL,
		OP_NOTEQUAL,
		OP_MAX
	};
	enum Operation{
		INTERSECT,
		PLUS,
		MINUS,
		MULTIPLY,
		DIVIDE
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
		virtual attr_e push(attr_e) = 0;
		virtual void pop(attr_e) = 0;
	};

	PatternMatcher();
	~PatternMatcher();

    
	virtual void setTimeout(attr_t _timeout) { m_Timeout = _timeout; }

	virtual void addState(uint32_t _idx, uint32_t _numAttributes, StateType _type = ST_NORMAL, uint32_t _kleenePlusAttrIdx = 0);
	virtual void setCallback(uint32_t _state, CallbackType _type, const std::function<void(const attr_e*)>& _function);
	virtual void addAggregation(uint32_t _state, AggregationFunction* _function, uint32_t _srcAttr, uint32_t _dstAttr);
	virtual void addTransition(uint32_t _fromState, uint32_t _toState, uint32_t _eventType);
	virtual void addPrecondition(uint32_t _param, attr_e _constant, Operator _op); 

	virtual void addCondition(uint32_t _param1, uint32_t _param2, Operator _op, attr_e _constant, uint32_t _iWhere, bool _postAggregationCheck = false);
	virtual void addActionCopy(uint32_t _src, uint32_t _dst);
	void resetRuns();

	virtual uint32_t event(uint32_t _type, const attr_e* _attr);

	uint32_t numRuns(uint32_t _stateId) const										{ return m_States[_stateId].count; }
	size_t numAttributes(uint32_t _stateId) const									{ return m_States[_stateId].attr.size(); }
	void run(uint32_t _stateId, uint32_t _runIdx, attr_e* _attrOut) const;

	void clearState(uint32_t _stateId);

    void setStates2Transitions();
    void setStates2States();
    void computeScores4LoadShedding();
    void loadShedding();
    void randomLoadShedding();
    bool compareRunScore(const std::pair<attr_e, double> & lhs, const std::pair<attr_e, double> & rhs) { return lhs.second < rhs.second;}
    static void setMonitoringLoadOn() {loadMonitoringFlag = true;}
    static void setMonitoringLoadOff() {loadMonitoringFlag = false;}
    static bool MonitoringLoad() { return loadMonitoringFlag;}

    void setTimeSliceNum(int i=0);
    void setTimeWindow(attr_t tw=0);

    void setRandomPMShedding(double _ratio);
    void setSelectivityPMShedding(double _ratio);


    void print();

	static const OperatorInfo& operatorInfo(Operator _op);

	struct Condition
	{
		uint32_t	param[2];
		Operator	op;
		attr_e		constant;
		uint32_t indexInWhere;
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

	struct State
    {
        State();
		~State();

		void setCount(uint32_t _count);
		void setAttributeCount(uint32_t _count);
		void setIndexAttribute(uint32_t _idx);

		void insert(const attr_e * _attributes);
		void remove(uint32_t _index) { remove_list.push_back(_index); }
		void endTransaction();

		void removeTimeouts(attr_t _value);

		void attributes(uint32_t _idx, attr_e* _out) const;
		bool runValid(uint32_t _idx) const { return attr.empty() || attr.front()[_idx].i > timeout; }

        void setTimeSliceNum(int i = 0);

        void setPMKeyIdx(int i=0) { PMKeyIdx = i;}


		const std::multimap<attr_e, uint64_t>* indexMap(uint32_t _attributeIdx) const { return _attributeIdx == index_attribute ? &index : NULL; }


        void setCache(Cache * _cache, int _num) {m_Cache = _cache; m_num_FetchWorker = _num;}
        void setUtilityMap(unordered_map<uint64_t, uint64_t> * _map) {m_KeyUtility = _map;}
        void setExternalIndex(int _idx) {m_externalIdx = _idx;} 
        void setExternalComIndex(int _idx) {m_externalComIdx = _idx;}
        void setExternalFlagIndex(int _idx) {m_externalFlagIdx = _idx;}

    

		StateType	type;
		uint32_t	kleenePlusAttrIdx;
		uint32_t	count;
		uint64_t	firstMatchId;  
		std::vector<std::deque<attr_e> > attr;
		std::vector<Aggregation> aggregation;

        vector<unordered_set<attr_t>> PMBooks;
        

		std::function<void(const attr_e*)> callback_timeout;
		std::function<void(const attr_e*)> callback_insert;

        uint32_t    KeyAttrIdx; 
        acceptCounter_t   accNum;
        acceptCounter_t   transNum;
        std::map<std::pair<attr_e, attr_e>, acceptCounter_t> tranCounter;
        std::unordered_map<attr_e,acceptCounter_t> valAccCounter;
        std::vector<std::pair<attr_e,double>> rankingTable;
        std::unordered_map<attr_e, acceptCounter_t> contributions;
        std::unordered_map<attr_e, acceptCounter_t> consumptions;
        std::vector<std::pair<attr_e, double>>  scoreTable; 
        

        void setKeyAttrIdx(uint32_t _idx) {KeyAttrIdx = _idx;}

		std::vector<uint32_t>						remove_list; 
		uint32_t									index_attribute;
		std::multimap<attr_e, uint64_t>				index;
		attr_t										timeout = 0;
        uint32_t                                    ID;
        std::vector<State>*                         states = NULL;

        int PMKeyIdx = 0;
        attr_t timeWindow = 0;
        int numTimeSlice = 0;
        int timePointIdx = 0;
        attr_t timeSliceSpan = 1;
        bool sheddingIrrelaventFlag = false;

        int randomPMDiceUB = 0;
        int selectivityPMDiceUB = 0; 

        std::set<uint64_t> keys;

        Cache * m_Cache;
        std::unordered_map<uint64_t, uint64_t>*  m_KeyUtility;

        int m_num_FetchWorker; 
        int m_externalIdx;
        int m_externalComIdx;
        int m_externalFlagIdx; 
	};

	struct Transition
	{
		uint32_t	eventType;
		uint32_t	from;
		uint32_t	to;

		std::vector<Condition>	preconditions;	
		std::vector<Condition>	conditions;		
		std::vector<Condition>	postconditions;	
		std::vector<CopyAction>	actions;
        std::vector<State>* states = NULL;

        static int fetchCnt;
        static int prefetchCnt;
        static int fetchProbability;
        static int consumptionPolicy;
        static int fetch_latency;
        static bool FLAG_delay_fetch;
        static bool FLAG_prefetch; 
        static bool FLAG_baseline; 

		void updateHandler(const State& _from, const State& _to);
		void setCustomExecuteHandler(std::function<void(State&, State&, uint32_t, const attr_e*)> _handler);

		uint32_t checkEvent(State& _from, State& _to, size_t _runOffset, const attr_e* _attr);
		uint32_t	(Transition::*checkForMatch)(State&, State&, size_t, const attr_e*);
		bool		(Transition::*executeMatch)(State&, State&, uint32_t, const attr_e*);

		bool executeTransition(State& _from, State& _to, uint32_t _idx, const attr_e*  _attributes);
		bool executeReject(State& _from, State& _to, uint32_t _idx, const attr_e* _attributes);
		bool executeCustom(State& _from, State& _to, uint32_t _idx, const attr_e* _attributes);

		uint32_t checkNoCondition(State& _from, State& _to, size_t _runOffset, const attr_e* _eventAttr);
		uint32_t checkSingleCondition(State& _from, State& _to, size_t _runOffset, const attr_e* _eventAttr);
		uint32_t checkMultipleConditions(State& _from, State& _to, size_t _runOffset, const attr_e* _eventAttr);

        void updateContribution(State& _from, State& _to, uint32_t idx, attr_e valFrom, attr_e valTo, attr_e * _attributes);

		
		static void testCondition(const Condition& _condition, const State& _state, size_t _runOffset, const attr_e* _attr, std::function<void(uint32_t)> _callback);

		uint32_t checkKleene(State& _from, State& _to, size_t _runOffset, const attr_e* _eventAttr);
		bool executeKleene(State& _from, State& _to, uint32_t _idx, const attr_e* _attributes);

		uint32_t	(Transition::*checkForMatchOrg)(State&, State&, size_t, const attr_e*);
		bool		(Transition::*executeMatchOrg)(State&, State&, uint32_t, const attr_e*);

		typedef std::vector<std::pair<uint32_t, attr_e> > runs_t;
		runs_t runs;

		std::function<void(State&, State&, uint32_t, const attr_e*)>	m_CustomExecuteHandler;

	};

	void testCondition(const Condition& _condition, const State& _state, const attr_e* _attr, uint32_t _matchOffset, std::vector<uint32_t>& _matchOut);

    public:

	std::vector<State> m_States;
	std::vector<Transition> m_Transitions;

    bool latencyFlag;


	attr_t		m_Timeout;
    bool Rflag;
};


inline bool checkCondition(attr_e _p1, attr_e _p2, PatternMatcher::Operator _op)
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

inline attr_e sum(attr_e a, attr_e b)
{

	switch (b.tag)
	{
	case attr_e::INT64_T:
		return attr_e(a.i + b.i);
		break;
	case attr_e::DOUBLE:
		return attr_e(a.d + b.d);
	default:
		return attr_e((attr_t)0);
		break;
	}
};

#ifndef PATTERNMATCHER_H
#define PATTERNMATCHER_H
#endif
