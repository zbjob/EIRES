#pragma once

#include <inttypes.h>
#include <vector>
#include <deque>
#include <functional>
#include <map>
#include <unordered_map>
#include <string>

#include "Cache_ordered.h"
#include "FetchWorker.h"

typedef int64_t attr_t;
typedef uint64_t acceptCounter_t;

using namespace std;

class PatternMatcher
{
    public:
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

        virtual void setTimeout(attr_t _timeout) { m_Timeout = _timeout; }
        virtual void addState(uint32_t _idx, uint32_t _numAttributes, StateType _type = ST_NORMAL, uint32_t _kleenePlusAttrIdx = 0);
        virtual void setCallback(uint32_t _state, CallbackType _type, const std::function<void(const attr_t*)>& _function);
        virtual void addAggregation(uint32_t _state, AggregationFunction* _function, uint32_t _srcAttr, uint32_t _dstAttr);
        virtual void addTransition(uint32_t _fromState, uint32_t _toState, uint32_t _eventType);
        virtual void addPrecondition(uint32_t _param, attr_t _constant, Operator _op); 
        virtual void addCondition(uint32_t _param1, uint32_t _param2, Operator _op, attr_t _constant, bool _postAggregationCheck = false);
        virtual void addActionCopy(uint32_t _src, uint32_t _dst);
        void resetRuns();

        virtual uint32_t event(uint32_t _type, const attr_t* _attr);

        uint32_t numRuns(uint32_t _stateId) const										{ return m_States[_stateId].count; }
        size_t numAttributes(uint32_t _stateId) const									{ return m_States[_stateId].attr.size(); }
        void run(uint32_t _stateId, uint32_t _runIdx, attr_t* _attrOut) const;

        void clearState(uint32_t _stateId);

        void dumpStatesConsumptions(std::string);



        static const OperatorInfo& operatorInfo(Operator _op);
        void print();

        void PM_shedding_random(int _state, double _ratio);
        void PM_shedding_selectivity(double _ratio);

        //protected:
        struct Condition
        {
            uint32_t	param[2];
            Operator	op;
            attr_t		constant;
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

            void insert(const attr_t* _attributes);
            void remove(uint32_t _index) { remove_list.push_back(_index); }
            void endTransaction();

            void removeTimeouts(attr_t _value);

            void attributes(uint32_t _idx, attr_t* _out) const;
            bool runValid(uint32_t _idx) const { return attr.empty() || attr.front()[_idx] > timeout; }

            void loadShedding();

            void setCache(Cache * _cache, int _num) {m_Cache = _cache; m_num_FetchWorker = _num;}
            void setUtilityMap(unordered_map<uint64_t, uint64_t> * _map) {m_KeyUtility = _map;}
            void setExternalIndex(int _idx) {m_externalIdx = _idx;}
            void setExternalComIndex(int _idx) {m_externalComIdx = _idx;}
            void setExternalFlagIndex(int _idx) {m_externalFlagIdx = _idx;}



            const std::multimap<attr_t, uint64_t>* indexMap(uint32_t _attributeIdx) const { return _attributeIdx == index_attribute ? &index : NULL; }


            StateType	type;
            uint32_t	kleenePlusAttrIdx;
            uint32_t	count;
            uint64_t	firstMatchId;  // only used to maintain the multiMap index ?
            std::vector<std::deque<attr_t> > attr;
            std::vector<Aggregation> aggregation;

            std::function<void(const attr_t*)> callback_timeout;
            std::function<void(const attr_t*)> callback_insert;

            void readFilters(string inputfile, double sheddingRatio); 

            uint32_t    KeyAttrIdx; 
            acceptCounter_t   accNum;
            acceptCounter_t   transNum;
            std::map<std::pair<attr_t, attr_t>, acceptCounter_t> tranCounter;
            std::unordered_map<attr_t,acceptCounter_t> valAccCounter;
            std::vector<std::pair<attr_t,double>> rankingTable;

            void setKeyAttrIdx(uint32_t _idx) {KeyAttrIdx = _idx;}
            void setConsumptionIdx(uint32_t _idx) {ConsumptionIdx = _idx;}



            //private:
            std::vector<uint32_t>						remove_list; 
            uint32_t									index_attribute;
            std::multimap<attr_t, uint64_t>				index;
            attr_t										timeout = 0;
            uint32_t                                    ID;
            std::map<attr_t, attr_t>                    Consumption;
            std::map<attr_t, attr_t>                    Contribution;
            uint32_t                                    ConsumptionIdx;
            std::unordered_map<uint64_t,double>         Filter;

            uint64_t                                    NumPartialMatchers = 0;
            uint64_t                                    ZeroContributionPM;
            uint64_t                                    allConsumption; 

            int PMDice = 0; 
            int SelectivityPMDice = 0;

            //Prefetching members
            Cache * m_Cache;
            unordered_map<uint64_t, uint64_t>*  m_KeyUtility;
            int     m_num_FetchWorker;
            int     m_externalIdx;
            int     m_externalComIdx;
            int     m_externalFlagIdx;
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

            static int fetchCnt;
            static int prefetchCnt;
            static int prefetchFrequency;
            static int fetchProbability;
            static int consumptionPolicy;
            static int fetch_latency;
            static bool FLAG_delay_fetch;
            static bool FLAG_prefetch;

            void updateHandler(const State& _from, const State& _to);
            void setCustomExecuteHandler(std::function<void(State&, State&, uint32_t, const attr_t*)> _handler);

            uint32_t checkEvent(State& _from, State& _to, size_t _runOffset, const attr_t* _attr);
            //	private:
            uint32_t	(Transition::*checkForMatch)(State&, State&, size_t, const attr_t*);
            void		(Transition::*executeMatch)(State&, State&, uint32_t, const attr_t*);

            void executeTransition(State& _from, State& _to, uint32_t _idx, const attr_t* _attributes);
            void executeReject(State& _from, State& _to, uint32_t _idx, const attr_t* _attributes);
            void executeCustom(State& _from, State& _to, uint32_t _idx, const attr_t* _attributes);

            uint32_t checkNoCondition(State& _from, State& _to, size_t _runOffset, const attr_t* _eventAttr);
            uint32_t checkSingleCondition(State& _from, State& _to, size_t _runOffset, const attr_t* _eventAttr);
            uint32_t checkMultipleCondotions(State& _from, State& _to, size_t _runOffset, const attr_t* _eventAttr);

            void updateContribution(State& _from, State& _to, uint32_t idx, attr_t valFrom, attr_t valTo);


            static void testCondition(const Condition& _condition, const State& _state, size_t _runOffset, const attr_t* _attr, std::function<void(uint32_t)> _callback);

            // kleene member
            uint32_t checkKleene(State& _from, State& _to, size_t _runOffset, const attr_t* _eventAttr);
            void executeKleene(State& _from, State& _to, uint32_t _idx, const attr_t* _attributes);

            uint32_t	(Transition::*checkForMatchOrg)(State&, State&, size_t, const attr_t*);
            void		(Transition::*executeMatchOrg)(State&, State&, uint32_t, const attr_t*);

            typedef std::vector<std::pair<uint32_t, attr_t> > runs_t;
            runs_t runs;

            std::function<void(State&, State&, uint32_t, const attr_t*)>	m_CustomExecuteHandler;
        };

        void testCondition(const Condition& _condition, const State& _state, const attr_t* _attr, uint32_t _matchOffset, std::vector<uint32_t>& _matchOut);

        //protected:
        std::vector<State> m_States;
        std::vector<Transition> m_Transitions;


        attr_t		m_Timeout;
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
}
