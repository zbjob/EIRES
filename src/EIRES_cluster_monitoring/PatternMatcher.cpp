#include "PatternMatcher.h"
#include <assert.h>
#include <emmintrin.h>
#include <algorithm>
#include <iostream>
#include "_shared/GlobalClock.h"
#include "Query.h"
#include <sstream>
#include <fstream>
#include <string>
#include <random>
#include <time.h>
#include <assert.h>



using namespace std;
using namespace std::chrono;


default_random_engine _m_generator;
uniform_int_distribution<int> _m_distribution(1,100);
uniform_int_distribution<int> fetch_latency_distribution(1, 10);

PatternMatcher::PatternMatcher()
{
	addState(0, 0);
	m_States[0].setCount(1);

	assert(OP_MAX == 6 && "update s_TestFunc1");
}

PatternMatcher::~PatternMatcher()
{
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

void PatternMatcher::setCallback(uint32_t _state, CallbackType _type, const std::function<void(const attr_t*)>& _function)
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

void PatternMatcher::addPrecondition(uint32_t _param1, attr_t _constant, Operator _op)
{
	Condition c;
	c.param[0] = _param1;
	c.op = _op;
	c.constant = _constant;
	m_Transitions.back().preconditions.push_back(c);
}

void PatternMatcher::addCondition(uint32_t _op1, uint32_t _op2, Operator _operator, attr_t _constant, bool _postAggregationCheck)
{
	Condition c;
	c.param[0] = _op1;
	c.param[1] = _op2;
	c.op = _operator;
	c.constant = _constant;

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

void PatternMatcher::Transition::testCondition(const Condition& _condition, const State& _state, size_t _runOffset, const attr_t* _attr, std::function<void(uint32_t)> _callback)
{
	const attr_t eventParam = _attr[_condition.param[1]] + _condition.constant;

	const auto* index = _state.indexMap(_condition.param[0]);
	if (index && _condition.op == OP_EQUAL)
	{
		auto range = index->equal_range(eventParam);
		for (auto it = range.first; it != range.second; ++it)
		{
			uint32_t runIdx = (uint32_t)(it->second - _state.firstMatchId);
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
			if (checkCondition(list[idx], eventParam, _condition.op) && _state.runValid(idx))
			{
				_callback(idx);
			}
		}
	}
}

void PatternMatcher::testCondition(const Condition& _condition, const State& _state, const attr_t* _attr, uint32_t _matchOffset, std::vector<uint32_t>& _matchOut)
{
	const attr_t eventParam = _attr[_condition.param[1]] + _condition.constant;
	const auto& attribute = _state.attr[_condition.param[0]];

	for (uint32_t cur = _matchOffset; cur < _matchOut.size();)
	{
		const attr_t eventParam = _attr[_condition.param[1]] + _condition.constant;
		if (checkCondition(attribute[_matchOut[cur]], eventParam, _condition.op))
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

uint32_t PatternMatcher::event(uint32_t _typeId, const attr_t* _attributes)
{
	uint32_t matchCount = 0;

	if(_attributes[0] >= m_Timeout)
		for (State& state : m_States)
			state.removeTimeouts(_attributes[0] - m_Timeout);

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

void PatternMatcher::run(uint32_t _stateId, uint32_t _runIdx, attr_t* _attrOut) const
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

void PatternMatcher::PM_shedding_random(int _state, double ratio)
{
    m_States[_state].PMDice = ratio*100;
}

void PatternMatcher::PM_shedding_selectivity(double ratio)
{
    int R = ratio * 100;

    int Selector = 0;
    
    if(R<=10)
        Selector = 3;
    else if(R>10 && R<=20)
        Selector = 4;
    else if(R>20 && R<=30)
        Selector = 5;
    else if(R>30 && R<=40)
        Selector = 6;
    else if(R>40 && R<=50)
        Selector = 9;
    else if(R>50 && R<=60)
        Selector = 10;
    else if(R>60 && R <= 70)
        Selector = 11;
    else if(R>70 && R<=80)
        Selector = 12;
    else if(R>80 && R<=90)
        Selector = 5;
    else
        Selector = 0;

    for(int i=1; i <= 6; ++i)
        if(i== 2)
            m_States[i].PMDice = R+Selector;
        else
            m_States[i].PMDice = R-Selector;
}

void PatternMatcher::State::setAttributeCount(uint32_t _count)
{
	assert(count == 0);
	attr.resize(_count);
}

void PatternMatcher::State::setIndexAttribute(uint32_t _idx)
{
    
	assert(index.empty());
	index_attribute = _idx;
    setKeyAttrIdx(_idx);
}

void PatternMatcher::State::readFilters(string inputfile, double sheddingRatio)
{
    ifstream ifs;
    ifs.open(inputfile.c_str());
    if(!ifs.is_open())
    {
        cout << "can't open file " << inputfile << endl;
        return;
    }

    string line;
    int cnt = 1;
    uint64_t ConsumptionQuota = 0;
    uint64_t ConsumptionCounted = 0;
    uint64_t ConsumptionALL=0;
    vector<pair<uint64_t, uint64_t>> PMs;
    while(getline(ifs, line))
    {
        if(cnt <=2)
        {
           ++cnt; 
            continue;
        }
        else if(cnt == 3)
        {
            ZeroContributionPM = stoull(line);
            ConsumptionCounted += ZeroContributionPM;
        }
        else if(cnt == 4)
        {
            allConsumption = stoull(line);
            ConsumptionQuota = allConsumption * sheddingRatio;

            cout << ConsumptionCounted << " , " << ConsumptionQuota << endl;
            assert(ConsumptionQuota > 0);
        }
        else
        { 
            vector<string> data;
            stringstream lineStream(line);

            string cell;

            while(getline(lineStream, cell, ','))
                data.push_back(cell);

            assert(data.size() >= 4);
            ConsumptionCounted += stoull(data[2]);
            ConsumptionALL += stoull(data[2]);
            if(ConsumptionCounted > ConsumptionQuota)
                Filter.insert(make_pair(stoull(data[1]),stod(data[0])));
        }
        ++cnt;
    }
}

void PatternMatcher::State::insert(const attr_t * _attributes)
{
    if(!Filter.empty() && ConsumptionIdx)
    {
        if(Filter.count(_attributes[ConsumptionIdx]) == 0)
        {
            NumSheddingPM++;
            return;
        }
    }


    ++NumPM;
    ++NumPartialMatchers;
        
	if(callback_insert)
		callback_insert(_attributes);

	const attr_t* src_it = _attributes;
	for (auto& it : attr)
		it.push_back(*src_it++);

	if (index_attribute)
	{
		const uint64_t matchId = firstMatchId + attr[0].size() - 1; 
		index.insert(make_pair(_attributes[index_attribute], matchId));
	}

	if (attr.empty())
		count++;
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

		for (auto& it : attr)
			it.pop_front();
		firstMatchId++;
	}

	count = (uint32_t)attr.front().size();
	timeout = _value;
}

void PatternMatcher::State::attributes(uint32_t _idx, attr_t * _out) const
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
		checkForMatch = &Transition::checkMultipleCondotions;
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

void PatternMatcher::Transition::setCustomExecuteHandler(std::function<void(State&, State&, uint32_t, const attr_t*)> _handler)
{
	m_CustomExecuteHandler = move(_handler);
	executeMatch = &Transition::executeCustom;
}

uint32_t PatternMatcher::Transition::checkEvent(State & _from, State & _to, size_t _runOffset, const attr_t * _attr)
{
	for (auto pc : preconditions)
	{
		if (!checkCondition(_attr[pc.param[0]], pc.constant, pc.op))
			return 0;
	}

	return (this->*checkForMatch)(_from, _to, _runOffset, _attr);
}

void PatternMatcher::Transition::updateContribution(State& _from, State& _to, uint32_t idx, attr_t valFrom,attr_t valTo)
{
    if (_from.ID == 0)
        return;
    
    acceptCounter_t trans = 0;
    double learningScore = 0;
    
    _from.transNum++;

    std::pair<attr_t, attr_t> matchPair = std::make_pair(valFrom, valTo);
    auto iterAcc = _from.valAccCounter.find(valFrom);
    auto iterTran = _from.tranCounter.find(matchPair);

    if(iterTran == _from.tranCounter.end())
    {
        _from.tranCounter.insert(std::make_pair(matchPair,1));
        trans = 1; 
    }
    else
        trans = ++(iterTran->second);
    if(iterAcc != _from.valAccCounter.end())
        learningScore = iterAcc->second * trans; 
}

void PatternMatcher::Transition::executeTransition(State & _from, State & _to, uint32_t _idx, const attr_t* _attributes)
{
	attr_t attributes[MAX_ATTRIBUTES];
	for (uint32_t a = 0; a < _from.attr.size(); ++a)
	{
		attributes[a] = _from.attr[a][_idx];
	}

	for (auto it : actions)
	{
		attributes[it.dst] = _attributes[(int32_t)it.src];
    }

    //prefetching & lazy evaluation
    if(_to.m_externalIdx)
    {
        uint64_t key = attributes[_to.m_externalIdx];
        int Dice_roll  = _m_distribution(_m_generator);

        //prefetching
        if(FLAG_prefetch && !_to.m_externalComIdx && !_to.m_externalFlagIdx && Dice_roll <= fetchProbability)
        {
            if(prefetchCnt++ % prefetchFrequency == 0)
            {
                _to.m_Cache->fetchWorker[fetchCnt++ % _to.m_num_FetchWorker].fetch(key,1);
                fetchCnt = fetchCnt % _to.m_num_FetchWorker;
            }
            prefetchCnt = prefetchCnt%prefetchFrequency;
        }

        //prefetchin - block fetching for cache misses.
        if(!FLAG_delay_fetch && _to.m_externalComIdx && attributes[_to.m_externalFlagIdx])
        {
            uint64_t eventParam = attributes[_to.m_externalComIdx];
            uint64_t payload = _to.m_Cache->lookUp(key);

            if(payload) //cache hit
            {
                ++(_to.m_Cache->cacheHit);
                if(payload == eventParam) //check cache
                    return;

                //this PM has been checked against external remote data
                //no need to check in the following delay fetches
                attributes[_to.m_externalFlagIdx] = 0;

                _to.m_Cache->update(key, payload, (*_to.m_KeyUtility)[key], 0);
            }
            else
            {
                ++(_to.m_Cache->cacheMiss);

                int latency = fetch_latency_distribution(_m_generator) * fetch_latency;
                this_thread::sleep_for(chrono::microseconds(latency));

                payload = key;
                _to.m_Cache->update(key, payload, (*_to.m_KeyUtility)[key], 3);
                ++(_to.m_Cache->BlockFectchCnt);

                if(payload == eventParam)
                    return;
            }
        }
        //lazy evaluation
        if(FLAG_delay_fetch && _to.m_externalComIdx && attributes[_to.m_externalFlagIdx])
        {
            uint64_t eventParam = attributes[_to.m_externalComIdx];
            uint64_t payload = _to.m_Cache->lookUp(key);

            if(payload) 
            {
                ++(_to.m_Cache->cacheHit);
                if(payload == eventParam) 
                    return;

                attributes[_to.m_externalFlagIdx] = 0;

                _to.m_Cache->update(key, payload, (*_to.m_KeyUtility)[key], 0);

            }
            else
            {
                ++(_to.m_Cache->cacheMiss);

                if(_to.type == ST_ACCEPT)
                {
                    int latency = fetch_latency_distribution(_m_generator) * fetch_latency;
                    this_thread::sleep_for(chrono::microseconds(latency));

                    payload = key;
                    _to.m_Cache->update(key, payload, (*_to.m_KeyUtility)[key], 3);
                    ++(_to.m_Cache->BlockFectchCnt);

                    if(payload == eventParam)
                        return;
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

    if(_to.type == ST_ACCEPT)
    {
        ++NumFullMatch;
        attributes[Query::DA_FULL_MATCH_TIME] = (uint64_t)duration_cast<microseconds>(high_resolution_clock::now() - g_BeginClock).count();
        attributes[Query::DA_CURRENT_TIME] = _attributes[Query::DA_CURRENT_TIME];
    }

	_to.insert(attributes);

}

void PatternMatcher::Transition::executeReject(State & _from, State & _to, uint32_t _idx, const attr_t * _attributes)
{
	_from.remove(_idx);
}

void PatternMatcher::Transition::executeCustom(State & _from, State & _to, uint32_t _idx, const attr_t * _attributes)
{
	m_CustomExecuteHandler(_from, _to, _idx, _attributes);
}

uint32_t PatternMatcher::Transition::checkNoCondition(State & _from, State & _to, size_t _runOffset, const attr_t * _eventAttr)
{
	uint32_t counter = 0;
	for (uint32_t i = (uint32_t)_runOffset; i < _from.count; ++i)
	{
		if (_from.runValid(i))
		{
			(this->*executeMatch)(_from, _to, i, _eventAttr);
			counter++;
		}
	}
	return counter;
}

uint32_t PatternMatcher::Transition::checkSingleCondition(State & _from, State & _to, size_t _runOffset, const attr_t * _eventAttr)
{
	uint32_t counter = 0;

	const Condition& c = conditions.front();
	testCondition(c, _from, _runOffset, _eventAttr, [&](uint32_t _idx) {
		(this->*executeMatch)(_from, _to, _idx, _eventAttr);
		counter++;
	});

	return counter;
}

uint32_t PatternMatcher::Transition::checkMultipleCondotions(State & _from, State & _to, size_t _runOffset, const attr_t * _eventAttr)
{
	uint32_t counter = 0;

	const Condition& c = conditions.front();
	testCondition(c, _from, _runOffset, _eventAttr, [&](uint32_t _idx) {

		bool valid = true;

		for (size_t i = 1; i < conditions.size(); ++i)
		{
			const Condition& c = conditions[i];
			const attr_t runParam = _from.attr[c.param[0]][_idx];
			const attr_t eventParam = _eventAttr[c.param[1]] + c.constant;
			if (!checkCondition(runParam, eventParam, c.op))
			{
				valid = false;
				break;
			}
		}

		if(valid)
		{
			(this->*executeMatch)(_from, _to, _idx, _eventAttr);
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

uint32_t PatternMatcher::Transition::checkKleene(State & _from, State & _to, size_t _runOffset, const attr_t * _eventAttr)
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
			attr_t runAttr = _from.attr[a.srcAttr][_top->first];
			(attr_t&)_eventAttr[a.dstAttr] = a.function->push(runAttr);
		}

		bool predicateOk = true;
		for (const auto & c : postconditions)
			predicateOk &= checkCondition(_eventAttr[c.param[0]], _eventAttr[c.param[1]] + c.constant, c.op);

		if (predicateOk)
		{
			(this->*executeMatchOrg)(_from, _to, runs.front().first, _eventAttr);
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

void PatternMatcher::Transition::executeKleene(State & _from, State & _to, uint32_t _idx, const attr_t * _attributes)
{
	runs.push_back(make_pair(_idx, _from.attr[_from.kleenePlusAttrIdx][_idx]));
}

void PatternMatcher::print()
{
   cout << "print states: =====================" << endl; 
   for(auto &&s : m_States)
   {
       if(s.type == ST_ACCEPT)
           break;
       if(s.ID!=0)
       {
           cout << "state ID : " << s.ID << endl;
           cout << "attribute size:" << s.attr.size() << endl;
           cout << "index_attribute : " << s.index_attribute << endl;
       }
   }

   cout << "print trasitions ====================== " << endl;
   for(auto && t : m_Transitions)
   {
       cout << "from state : " << t.from << "--- to state: " << t.to << " condditions : ------------" << endl;
       for(auto && c : t.conditions)
           cout << "------------------------ 1st operandi idx: " << c.param[0] <<  "--- 2nd operand idx: " << c.param[1] << "--- operator: " << c.op << endl;
       cout << endl;
   }
   cout << "============================================== " << endl;
   cout << "============================================== " << endl;
}
