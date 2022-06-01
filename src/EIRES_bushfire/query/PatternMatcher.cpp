#include "../_shared/GlobalClock.h"
#include "PatternMatcher.h"
#include <assert.h>
#include <emmintrin.h>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include "Query.h"
#include <random>
#include "../event/EventStream.h"

using namespace std;
using namespace std::chrono;
bool PatternMatcher::loadMonitoringFlag = true;
default_random_engine _m_generator;
uniform_int_distribution<int> _m_distribution(1,100);
uniform_int_distribution<int> fetch_latency_distribution(1, 10);

std::map<uint32_t,Expression> expressions;

PatternMatcher::PatternMatcher()
{
	addState(0, 0);
	m_States[0].setCount(1);
    latencyFlag = false;
    Rflag = true;

	assert(OP_MAX == 6 && "update s_TestFunc1");
}

PatternMatcher::~PatternMatcher()
{
}

void PatternMatcher::setRandomPMShedding(double _ratio)
{
    int Ratio = _ratio*100;
    for(auto &s : m_States)
        s.randomPMDiceUB = Ratio;
}

void PatternMatcher::setSelectivityPMShedding(double _ratio)
{
    int Ratio = _ratio*100;
    m_States[4].selectivityPMDiceUB = Ratio;
    m_States[3].selectivityPMDiceUB = Ratio+2;
    m_States[2].selectivityPMDiceUB = Ratio+3;
    m_States[1].selectivityPMDiceUB = Ratio+4;
}

void PatternMatcher::addState(uint32_t _idx, uint32_t _numAttributes, PatternMatcher::StateType _type, uint32_t _kleenePlusAttrIdx)
{
	if (_idx >= m_States.size())
		m_States.resize(_idx + 1);

	State& s = m_States[_idx];
    s.ID = _idx;
	s.setAttributeCount(_numAttributes);
	s.type = _type;
	s.kleenePlusAttrIdx = _kleenePlusAttrIdx;
}

void PatternMatcher::setTimeSliceNum(int i)
{
    for(auto &s : m_States)
        s.setTimeSliceNum(i);
}

void PatternMatcher::setTimeWindow(attr_t tw)
{
    for(auto &s : m_States)
        s.timeWindow = tw;
}

void PatternMatcher::setCallback(uint32_t _state, CallbackType _type, const std::function<void(const attr_e *)>& _function)
{
	if (_type == CT_INSERT)
	{
		m_States[_state].callback_insert = _function;
	}
	else
	{
		m_States[_state].callback_timeout = _function;
	}
}

void PatternMatcher::addAggregation(uint32_t _state, AggregationFunction * _function, uint32_t _srcAttr, uint32_t _dstAttr)
{
	Aggregation a;
	a.function = _function;
	a.srcAttr = _srcAttr;
	a.dstAttr = _dstAttr;
	m_States[_state].aggregation.push_back(a);
}

void PatternMatcher::addTransition(uint32_t _fromState, uint32_t _toState, uint32_t _eventType)
{
	assert(_fromState < m_States.size() && _toState < m_States.size());

	Transition t;
	t.eventType = _eventType;
	t.from = _fromState;
	t.to = _toState;
	t.updateHandler(m_States[_fromState], m_States[_toState]);

	m_Transitions.push_back(t);
}

void PatternMatcher::addPrecondition(uint32_t _param1, attr_e _constant, Operator _op)
{
	Condition c;
	c.param[0] = _param1;
	c.op = _op;
	c.constant = _constant;
	m_Transitions.back().preconditions.push_back(c);
}

void PatternMatcher::addCondition(uint32_t _op1, uint32_t _op2, Operator _operator, attr_e _constant, uint32_t _iWhere,  bool _postAggregationCheck)
{
	Condition c;
	c.param[0] = _op1;
	c.param[1] = _op2;
	c.op = _operator;
	c.constant = _constant;
	c.indexInWhere = _iWhere;

	Transition& t = m_Transitions.back();

	if (_postAggregationCheck)
		t.postconditions.push_back(c);
	else
	{
		if (t.conditions.empty())
		{
			m_States[t.from].setIndexAttribute(_op1);
		}

		t.conditions.push_back(c);
	}

	t.updateHandler(m_States[t.from], m_States[t.to]);
}

void PatternMatcher::addActionCopy(uint32_t _src, uint32_t _dst)
{
	CopyAction a;
	a.src = _src;
	a.dst = _dst;
	m_Transitions.back().actions.push_back(a);
}

void PatternMatcher::Transition::testCondition(const Condition& _condition, const State& _state, size_t _runOffset, const attr_e* _attr, std::function<void(uint32_t)> _callback)
{
	attr_e eventParam = sum(_attr[_condition.param[1]],_condition.constant);


	if(expressions.find(_condition.indexInWhere + 1) != expressions.end()){
		eventParam = expressions.at(_condition.indexInWhere + 1).getValue(eventParam);
	}

	const auto* index = _state.indexMap(_condition.param[0]);
	if (index && _condition.op == OP_EQUAL)
	{
		auto range = index->equal_range(eventParam);
		for (auto it = range.first; it != range.second; ++it)
		{
			uint32_t runIdx = (uint32_t)(it->second - _state.firstMatchId);
            //if(_state.ID == 1 && false) 
            //{
            //    cout << "it->second " << it->second << endl;
            //    cout << "_state.firstMatchId: " << _state.firstMatchId << endl;
            //    cout << "_state.buffer.size: " << _state.attr[0].size() << endl;
            //    cout << "runIdx: " << runIdx << endl;
            //}
			if (runIdx < _state.count && runIdx >= _runOffset && _state.runValid(runIdx))
			{
				_callback(runIdx);
			}
		}
	}
	else
	{
		const auto& list = _state.attr[_condition.param[0]];
			
		for (uint32_t idx = (uint32_t)_runOffset; idx < _state.count; ++idx)
		{
			attr_e curEvent = list[idx];
			if(expressions.find(_condition.indexInWhere)!=expressions.end())
				curEvent = expressions.at(_condition.indexInWhere).getValue(curEvent);

			if (checkCondition(curEvent, eventParam, _condition.op) && _state.runValid(idx))
			{
				_callback(idx);
			}
		}
	}
}

void PatternMatcher::testCondition(const Condition& _condition, const State& _state,const attr_e * _attr, uint32_t _matchOffset, std::vector<uint32_t>& _matchOut)
{
	attr_e eventParam = sum(_attr[_condition.param[1]], _condition.constant);
			

	
	const auto& attribute = _state.attr[_condition.param[0]];
	if(expressions.find(_condition.indexInWhere + 1) != expressions.end())
		eventParam =  expressions.at(_condition.indexInWhere + 1).getValue(eventParam);

	for (uint32_t cur = _matchOffset; cur < _matchOut.size();)
	{
		attr_e curAttr  = attribute[_matchOut[cur]];
		if(expressions.find(_condition.indexInWhere) != expressions.end())
			curAttr = expressions.at(_condition.indexInWhere).getValue(eventParam);
		if (checkCondition(curAttr , eventParam, _condition.op))
		{
			cur++;
		}
		else
		{
			_matchOut[cur] = _matchOut.back();
			_matchOut.pop_back();
		}
	}
}


void PatternMatcher::loadShedding()
{
    cout << "perform load shedding" << endl;
    for(auto && it : this->m_States)
    {
        if(it.type == ST_ACCEPT)
            break;
        if(it.ID == 0)
            continue;
            
        int loadThreshold = it.attr.front().size() /5 ;
        int cnt = 0;
        for(auto && iterScore : it.scoreTable)
        {
            if(cnt >= loadThreshold)
                break;
            auto range = it.index.equal_range(iterScore.first);
            for(auto i = range.first; i != range.second; ++i, ++cnt)
            {
                uint32_t id = i->second - it.firstMatchId;
                if(id < it.attr.front().size())
                it.attr.front()[id] = attr_e((attr_t)0);
            }
        }
    }
}

void PatternMatcher::randomLoadShedding()
{
    srand((uint64_t)duration_cast<microseconds>(high_resolution_clock::now() - g_BeginClock).count()<<2);
    
    cout << "perform random load shedding" << endl;
    for(auto && it : this->m_States)
    {
        if(it.type == ST_ACCEPT)
            break;
        if(it.ID == 0)
            continue;
            
        int loadThreshold = it.attr.front().size() / 5;
        int cnt = 0;

        for(int cnt=0; cnt < loadThreshold; ++cnt)
        {
                int size = it.attr.front().size();
                int ran = rand() % size + 1;
                
                it.attr.front()[ran] = attr_e((attr_t)0);
        }
        
        
    }
    
}
uint32_t PatternMatcher::event(uint32_t _typeId, const attr_e* _attributes)
{
	uint32_t matchCount = 0;

	if(_attributes[0].i >= m_Timeout)
		for (State& state : m_States)
			state.removeTimeouts(_attributes[0].i - m_Timeout);

	for (Transition& t : m_Transitions)
	{
		if (t.eventType != _typeId)
			continue;

		matchCount += t.checkEvent(m_States[t.from], m_States[t.to], 0, _attributes);
	}

	for (State& state : m_States)
		state.endTransaction();
	return matchCount;
}

void PatternMatcher::run(uint32_t _stateId, uint32_t _runIdx, attr_e* _attrOut) const
{
	assert(_stateId < m_States.size());
	assert(_runIdx < m_States[_stateId].count);

	for (const auto& it : m_States[_stateId].attr)
	{
		*_attrOut++ = it[_runIdx];
	}
}

void PatternMatcher::clearState(uint32_t _stateId)
{
	m_States[_stateId].setCount(_stateId == 0 ? 1 : 0);
}

const PatternMatcher::OperatorInfo & PatternMatcher::operatorInfo(PatternMatcher::Operator _op)
{
	static OperatorInfo info[OP_MAX] = {
		{ "less", "<" },
		{ "lessequal", "<=" },
		{ "greater", ">" },
		{ "greaterequal", ">=" },
		{ "equal", "==" },
		{ "notequal", "!=" }
	};
	return info[_op];
}

void PatternMatcher::resetRuns()
{
	for (uint32_t i = 0; i < m_States.size(); ++i)
	{
		clearState(i);
	}
}

PatternMatcher::State::State() :
	type(ST_NORMAL), kleenePlusAttrIdx(0), count(0), index_attribute(0), firstMatchId(0), KeyAttrIdx(0), accNum(0), transNum(0)
{
}

PatternMatcher::State::~State()
{
}

void PatternMatcher::State::setCount(uint32_t _count)
{
	assert(_count <= count || attr.empty());

	count = _count;

	for (auto& it : attr)
		it.resize(_count);

}

void PatternMatcher::State::setAttributeCount(uint32_t _count)
{
	assert(count == 0);
	attr.resize(_count);
}

void PatternMatcher::State::setTimeSliceNum(int i)
{
    assert(PMBooks.size() == 0);
    PMBooks.resize(i);
    numTimeSlice = i;
}

void PatternMatcher::State::setIndexAttribute(uint32_t _idx)
{
    
	assert(index.empty());
	index_attribute = _idx;
    setKeyAttrIdx(_idx);
}

void PatternMatcher::State::insert(const attr_e * _attributes)
{
	if(callback_insert)
		callback_insert(_attributes);

    int timeSliceID = (_attributes[timePointIdx].i - _attributes[0].i)/timeSliceSpan;
    attr_e key = _attributes[PMKeyIdx];

    if(timeSliceID < 0 || timeSliceID >3)
    {
        return;
    }
	const attr_e * src_it = _attributes;
	for (auto& it : attr)
		it.push_back(*src_it++);

	if (index_attribute)
	{
		const uint64_t matchId = firstMatchId + attr[0].size() - 1; 
		index.insert(make_pair(_attributes[index_attribute], matchId));
	}



	if (attr.empty())
		count++;

    ++accNum;
}

void PatternMatcher::State::endTransaction()
{
	static_assert(std::numeric_limits<attr_t>::min() < 0, "invalid attr_t type");

	if (!attr.empty())
	{
		count = (uint32_t)attr.front().size();

		for(auto idx : remove_list)
			attr.front()[idx] = std::numeric_limits<attr_t>::min();
	}
	else
	{
		count -= (uint32_t)remove_list.size();
	}

	remove_list.clear();
}

template<typename T> auto begin(const std::pair<T, T>& _obj) { return _obj.first; }
template<typename T> auto end(const std::pair<T, T>& _obj) { return _obj.second; }

void PatternMatcher::State::removeTimeouts(attr_t _value) 
{
	if (attr.empty())
		return; 

	while (!attr.front().empty() && attr.front().front() < _value) 
	{
		if (index_attribute)
		{
			auto range = index.equal_range(attr[index_attribute].front());
			for (auto it = range.first; it != range.second; ++it)
			{
				if (it->second == firstMatchId)
				{
					index.erase(it);
					break;
				}
			}
		}

        if(PatternMatcher::MonitoringLoad() == true)
        {
            for(auto&& it : *states)
            {
               if(it.ID > this->ID || it.KeyAttrIdx ==0)
               {   
                   continue;
            
               }
               it.consumptions[attr[it.KeyAttrIdx].front()] += 1;
            }
        }

		for (auto&& it : attr)
			it.pop_front();
		firstMatchId++;
        --NumPartialMatch;
	}

	count = (uint32_t)attr.front().size();
	timeout = _value;
}

void PatternMatcher::State::attributes(uint32_t _idx, attr_e * _out) const
{
	for (const auto& it : attr)
		*_out++ = it[_idx];
}

void PatternMatcher::Transition::updateHandler(const State& _from, const State& _to)
{
	switch (conditions.size())
	{
	case 0:
		checkForMatch = &Transition::checkNoCondition;
		break;
	case 1:
		checkForMatch = &Transition::checkSingleCondition;
		break;
	default:
		checkForMatch = &Transition::checkMultipleConditions;
		break;
	}

	if (_to.type == ST_REJECT)
	{
		executeMatch = &Transition::executeReject;
	}
	else
	{
		executeMatch = &Transition::executeTransition;
	}

	if (_from.kleenePlusAttrIdx && _to.type != ST_REJECT)
	{
		executeMatchOrg = executeMatch;
		checkForMatchOrg = checkForMatch;

		executeMatch = &Transition::executeKleene;
		checkForMatch = &Transition::checkKleene;
	}
}

void PatternMatcher::Transition::setCustomExecuteHandler(std::function<void(State&, State&, uint32_t, const attr_e*)> _handler)
{
	m_CustomExecuteHandler = move(_handler);
	executeMatch = &Transition::executeCustom;
}

uint32_t PatternMatcher::Transition::checkEvent(State & _from, State & _to, size_t _runOffset,const attr_e * _attr)
{
	for (auto pc : preconditions)
	{
		if (!checkCondition(_attr[pc.param[0]], pc.constant, pc.op))
			return 0;
	}

	return (this->*checkForMatch)(_from, _to, _runOffset, _attr);
}

void PatternMatcher::Transition::updateContribution(State& _from, State& _to, uint32_t idx, attr_e valFrom, attr_e valTo, attr_e * _attributes)
{
    if (_from.ID == 0)
        return;

    _from.transNum++;

    if(_to.type == ST_ACCEPT)
    {
        if(!this->states)
        {
            cout << "states == " << this->states << endl;
            return;
        }
    }
}

void PatternMatcher::setStates2Transitions()
{
    
    for(auto && it: m_Transitions)
    {
        it.states = &m_States;
    }
}

void PatternMatcher::setStates2States()
{
    for(auto && it : m_States)
    {
        it.states = &m_States;
    }
}

bool PatternMatcher::Transition::executeTransition(State & _from, State & _to, uint32_t _idx, const attr_e * _attributes)
{
	attr_e attributes[MAX_ATTRIBUTES];
	for (uint32_t a = 0; a < _from.attr.size(); ++a)
	{
		attributes[a] = _from.attr[a][_idx];
	}


	for (auto it : actions)
	{
		attributes[it.dst] = _attributes[(int32_t)it.src];

		attributes[it.dst] = _attributes[(int32_t)it.src];
		if(_attributes[(int32_t)it.src].tag == attr_e::POLYGON & _from.attr.size() > 0){
			std::deque<poly_t> output;
			bg::intersection(_from.attr[3][_idx].poly,_attributes[(int32_t)it.src].poly,output);
			if(output.size() > 0){
				attr_e a = attr_e();
				a.tag = attr_e::POLYGON;
				a.poly = output.at(0);
				attributes[3] = a;
			} else {
				return false;
			}
		}
	}

    const double temperature = 46.51;
    const double humidity = 23.38;


    if(_to.m_externalIdx || _to.ID == 1 ) 
    {

        uint64_t key = attributes[_to.m_externalIdx].i;
        int Dice_roll  = _m_distribution(_m_generator);
        _to.keys.insert(key);

        if(FLAG_prefetch  && Dice_roll < fetchProbability) 
        {
            _to.m_Cache->fetchWorker[fetchCnt++ % _to.m_num_FetchWorker].fetch(key,1); 
            fetchCnt = fetchCnt % _to.m_num_FetchWorker;
        }

        if(!FLAG_delay_fetch && _to.m_externalComIdx && attributes[_to.m_externalFlagIdx].i) 
        {

            cachePld_t payload;
            bool cacheHitFlag = _to.m_Cache->lookUp(key, payload); 

            if(!FLAG_baseline && cacheHitFlag) 
            {
                ++(_to.m_Cache->cacheHit);
                if(!(payload.temperature >= temperature  && payload.humidity < humidity)) 
                {
                    return true;
                }

                attributes[_to.m_externalFlagIdx].i = 0;
                
                _to.m_Cache->update(key, payload, (*_to.m_KeyUtility)[key], 0);
            }
            else
            {
                ++(_to.m_Cache->cacheMiss);

                int latency = fetch_latency_distribution(_m_generator) * fetch_latency; 
                this_thread::sleep_for(chrono::microseconds(latency));

                payload = cachePld_t(46.56, 23.3);
                _to.m_Cache->update(key, payload, (*_to.m_KeyUtility)[key], 3);
                ++(_to.m_Cache->BlockFectchCnt);

                if(!(payload.temperature >= temperature  && payload.humidity < humidity)) 
                {
                    return true;
                }
            }
        }
        if(FLAG_delay_fetch && _to.m_externalComIdx && attributes[_to.m_externalFlagIdx].i) 
        {
            cachePld_t payload;
            bool cacheHitFlag = _to.m_Cache->lookUp(key, payload); 

            if(cacheHitFlag) 
            {
                ++(_to.m_Cache->cacheHit);
                if(!(payload.temperature >= temperature  && payload.humidity < humidity)) 
                    return true;

                attributes[_to.m_externalFlagIdx].i = 0;

                _to.m_Cache->update(key, payload, (*_to.m_KeyUtility)[key], 0);

            }
            else
            {
                ++(_to.m_Cache->cacheMiss);

                if(_to.type == ST_ACCEPT)
                {
                    int latency = fetch_latency_distribution(_m_generator) * fetch_latency; 
                    this_thread::sleep_for(chrono::microseconds(latency));

                    payload = cachePld_t(46.56, 23.3);
                    _to.m_Cache->update(key, payload, (*_to.m_KeyUtility)[key], 3);

                    ++(_to.m_Cache->BlockFectchCnt);

                    if(!(payload.temperature >= temperature  && payload.humidity < humidity)) 
                        return true;

                }
                else
                {
                    _to.m_Cache->fetchWorker[fetchCnt++ % _to.m_num_FetchWorker].fetch(key,2); 
                    fetchCnt = fetchCnt % _to.m_num_FetchWorker;
                }
            }
        }
        (*_to.m_KeyUtility)[key]++ ;
    }
    ++NumPartialMatch;

    if(_to.type == ST_ACCEPT)
    {
        ++NumFullMatch;
        attributes[Query::DA_FULL_MATCH_TIME] = attr_e(duration_cast<microseconds>(high_resolution_clock::now() - g_BeginClock).count());  
        if(attributes[Query::DA_FULL_MATCH_TIME].i - _attributes[Query::DA_CURRENT_TIME].i > 150) 
        ++NumHighLatency;
        attributes[Query::DA_CURRENT_TIME] = _attributes[Query::DA_CURRENT_TIME];
    }

	_to.insert(attributes);
	return true;
}

bool PatternMatcher::Transition::executeReject(State & _from, State & _to, uint32_t _idx, const attr_e * _attributes)
{
	_from.remove(_idx);
	return true;
}

bool PatternMatcher::Transition::executeCustom(State & _from, State & _to, uint32_t _idx, const attr_e * _attributes)
{
	m_CustomExecuteHandler(_from, _to, _idx, _attributes);
	return true;
}

uint32_t PatternMatcher::Transition::checkNoCondition(State & _from, State & _to, size_t _runOffset, const attr_e * _eventAttr)
{
	uint32_t counter = 0;
	for (uint32_t i = (uint32_t)_runOffset; i < _from.count; ++i)
	{
		if (_from.runValid(i))
		{
			if((this->*executeMatch)(_from, _to, i, _eventAttr))
			counter++;
		}
	}
	return counter;
}

uint32_t PatternMatcher::Transition::checkSingleCondition(State & _from, State & _to, size_t _runOffset,const attr_e * _eventAttr)
{
	uint32_t counter = 0;

	const Condition& c = conditions.front();
	testCondition(c, _from, _runOffset, _eventAttr, [&](uint32_t _idx) {
            
		if((this->*executeMatch)(_from, _to, _idx, _eventAttr))
			counter++;
	});

	return counter;
}

uint32_t PatternMatcher::Transition::checkMultipleConditions(State & _from, State & _to, size_t _runOffset,const attr_e * _eventAttr)
{
	uint32_t counter = 0;

	const Condition& c = conditions.front();
	testCondition(c, _from, _runOffset, _eventAttr, [&](uint32_t _idx) {

		bool valid = true;

		for (size_t i = 1; i < conditions.size(); ++i)
		{
			const Condition& c = conditions[i];
			attr_e runParam = _from.attr[c.param[0]][_idx];

			attr_e eventParam = sum(_eventAttr[c.param[1]],c.constant);
	

			if(expressions.find(c.indexInWhere+1)!=expressions.end())
				eventParam = expressions.at(c.indexInWhere+1).getValue(eventParam);
			if(expressions.find(c.indexInWhere)!=expressions.end())
				runParam = expressions.at(c.indexInWhere).getValue(runParam);
			if (!checkCondition(runParam, eventParam, c.op))
			{
				valid = false;
				break;
			}
		}

		if(valid)
		{
			if((this->*executeMatch)(_from, _to, _idx, _eventAttr))
				counter++;
		}

	});

	return counter;
}

static attr_t aggregat_avg(const vector<attr_t>& _attr)
{
	attr_t sum = 0;
	for (attr_t i : _attr)
		sum += i;
	return sum / _attr.size();
}

uint32_t PatternMatcher::Transition::checkKleene(State & _from, State & _to, size_t _runOffset, const attr_e * _eventAttr)
{
	(this->*checkForMatchOrg)(_from, _to, _runOffset, _eventAttr);

	std::sort(runs.begin(), runs.end(), [](const runs_t::value_type& _a, const runs_t::value_type& _b) -> bool {
		return _a.second < _b.second;
	});

	uint32_t counter = 0;
	const function<void(runs_t::const_iterator)> func = [&](runs_t::const_iterator _top)
	{
		for (auto& a : _from.aggregation)
		{
			attr_e runAttr = _from.attr[a.srcAttr][_top->first];
			(attr_e&)_eventAttr[a.dstAttr] = a.function->push(runAttr);
			
		}

		bool predicateOk = true;
		for (const auto & c : postconditions){
			attr_e p2 = sum(_eventAttr[c.param[1]],c.constant);
			predicateOk &= checkCondition(_eventAttr[c.param[0]], p2, c.op);
		}

		if (predicateOk)
		{
			if((this->*executeMatchOrg)(_from, _to, runs.front().first, _eventAttr))

			counter++;
		}

		for (runs_t::const_iterator it = _top + 1; it != runs.end(); ++it)
			func(it);

		for (auto& a : _from.aggregation)
			a.function->pop(_from.attr[a.srcAttr][_top->first]);
	};

	if (!runs.empty())
	{
		if (runs.size() > 20)
			fprintf(stderr, "warning: aggregation over %u runs...", (unsigned)runs.size());

		for (auto& a : _from.aggregation)
			a.function->clear();
		for (runs_t::const_iterator it = runs.begin(); it != runs.end(); ++it)
			func(it);

		if (runs.size() > 20)
			fprintf(stderr, "done\n");

		runs.clear();
	}
	return counter;
}

bool PatternMatcher::Transition::executeKleene(State & _from, State & _to, uint32_t _idx,const attr_e * _attributes)
{
	runs.push_back(make_pair(_idx, _from.attr[_from.kleenePlusAttrIdx][_idx]));
	return true;
}

void PatternMatcher::print(){
    cout << "states: " << endl;
    for(auto &&s : m_States)
    {
        cout << "state ID : " << s.ID << endl;
		cout << "state type: " << s.type << endl;
        cout << "KeyAttrIdx :" << s.KeyAttrIdx << endl;
        cout << "index_attribute : " << s.index_attribute << endl;
        if(s.ID)
            cout << "attribute Cnt: " << s.attr.size() << endl;
    }
}

    
