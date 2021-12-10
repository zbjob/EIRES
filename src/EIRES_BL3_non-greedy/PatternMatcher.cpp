#include "_shared/GlobalClock.h"
#include "PatternMatcher.h"
#include <assert.h>
#include <emmintrin.h>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <string>
#include "Query.h"
#include <time.h>
#include <random>
#include <limits>

using namespace std;
using namespace std::chrono;
bool PatternMatcher::loadMonitoringFlag = true;

default_random_engine _m_generator;
uniform_int_distribution<int> _m_distribution(1,100);
//uniform_int_distribution<int> fetch_latency_distribution(fetching_latency, fetching_latency*10);
//uniform_int_distribution<int> fetch_latency_distribution(5, 10);
uniform_int_distribution<int> fetch_latency_distribution(1, 10);





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

void PatternMatcher::print(){
    cout << "print states: =====================" << endl;
    for(auto &&s : m_States)
    {
        if(s.type == ST_ACCEPT)
            break;
        if(s.ID!=0 )
        {
            cout << "state ID : " << s.ID << endl;
            cout << "KeyAttrIdx siez:" << s.KeyAttrIdx.size() << endl;
            cout << "index_attribute : " << s.index_attribute << endl;
            cout << "attribute Cnt: " << s.buffers[0][0].size() << endl;

            int timesliceIt = 0;
            for(auto && T : s.buffers)
            {
                int clusterIt = 0; 
                for(auto && C : T)
                    cout << "------time slice : " << timesliceIt++ << " cluster : " << clusterIt++ << " #PMs : " << C.front().size() << endl; 
            }

            cout << endl;
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

void PatternMatcher::printContributions(){

    cout << "printContributions " << endl;
    for(auto &&s : m_States)
    {
        if(s.type == ST_ACCEPT)
            break;
        if(s.ID!=0 )
        {

            for(auto it : s.contributions)
                cout << "State: " << s.ID << it.first << "---" << it.second << endl;
        }
    }
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


void PatternMatcher::addState(uint32_t _idx, int _numTimeSlice, int _numCluser, uint32_t _numAttributes, PatternMatcher::StateType _type, uint32_t _kleenePlusAttrIdx)
{
	if (_idx >= m_States.size())
		m_States.resize(_idx + 1);

	State& s = m_States[_idx];
    s.ID = _idx;
	//s.setAttributeCount(_numAttributes);
    s.setTimesliceClusterAttributeCount(_numTimeSlice, _numCluser, _numAttributes);
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

void PatternMatcher::Transition::testCondition(const Condition& _condition, const State& _state, size_t _runOffset, const attr_t* _attr, std::function<void(int, int, uint32_t)> _callback)
{
    //if(_state.ID == 2)
    //    cout << "testCondition on state 2" << endl;
//        cout << "testCondition on state  " <<  _state.ID << endl;
//        
	const attr_t eventParam = _attr[_condition.param[1]] + _condition.constant;
    int timesliceIt = 0;
    for(auto&& timeslice: _state.buffers)
    {
        int clusterIt = 0;

        for(auto&& cluster: timeslice)
        {


            if(_condition.param[2] == 0) // two attr, one operator case.
            {
                // special case for index use
                const auto* index = _state.indexMap(timesliceIt,clusterIt,_condition.param[0]);
                if (index && _condition.op == OP_EQUAL)
                {
                    auto range = index->equal_range(eventParam);
                    for (auto&& it = range.first; it != range.second; ++it)
                    {
                        uint32_t runIdx = (uint32_t)(it->second - _state.firstMatchId[timesliceIt][clusterIt]);
                        if (runIdx < _state.count[timesliceIt][clusterIt] && runIdx >= _runOffset && _state.runValid(timesliceIt,clusterIt,runIdx))
                        {
                            _callback(timesliceIt,clusterIt,runIdx);
                        }
                    }
                }
                else
                {
                    //const auto& list = _state.attr[_condition.param[0]];
                    const auto& list = cluster[_condition.param[0]]; //interested attribute colume
                    //cout << "[testconditon] flag8.1 " << endl;
                    //cout << "_runOffset : " << _runOffset << " timesliceIt: " << timesliceIt << " clusterIt: " << clusterIt << endl;
                    //cout <<"state ID : " << _state.ID << "buffer size: " << _state.buffers[timesliceIt][clusterIt].front().size() << " relevent count : " << _state.count[timesliceIt][clusterIt] << " first matchID : " << _state.firstMatchId[timesliceIt][clusterIt] << endl;
                    //cout << _state.count[timesliceIt][clusterIt] << endl;

                    for (uint32_t idx = (uint32_t)_runOffset; idx < _state.count[timesliceIt][clusterIt]; ++idx)
                    {
                        if (checkCondition(list[idx], eventParam, _condition.op) && _state.runValid(timesliceIt,clusterIt,idx))
                        {
                            _callback(timesliceIt,clusterIt,idx);
                        }
                    }
                }
            }
            else //three attr, two operator case
            {
                cout << "[checkCondition] in the three attr, two operator case" << endl; 
                assert (_condition.op == PatternMatcher::OP_ADD);
                const attr_t eventParam3 = _attr[_condition.param[2]] + _condition.constant;
                cout << " eventParam3 : " << eventParam3 << endl;
                const auto& list = cluster[_condition.param[0]]; //interested attribute colume
                const auto& list2 = cluster[_condition.param[1]]; //interested attribute colume

                for (uint32_t idx = (uint32_t)_runOffset; idx < _state.count[timesliceIt][clusterIt]; ++idx)
                {
                    if (checkCondition(list[idx]+list2[idx], eventParam3, _condition.op2) && _state.runValid(timesliceIt,clusterIt,idx))
                    {
                        _callback(timesliceIt,clusterIt,idx);
                    }
                }


            }


            clusterIt++;
        }
        timesliceIt++;
    }
}

//void PatternMatcher::testCondition(const Condition& _condition, const State& _state, const attr_t* _attr, uint32_t _matchOffset, std::vector<uint32_t>& _matchOut)
//{
//	const attr_t eventParam = _attr[_condition.param[1]] + _condition.constant;
//	const auto& attribute = _state.attr[_condition.param[0]];
//
//	for (uint32_t cur = _matchOffset; cur < _matchOut.size();)
//	{
//		const attr_t eventParam = _attr[_condition.param[1]] + _condition.constant;
//		if (checkCondition(attribute[_matchOut[cur]], eventParam, _condition.op))
//		{
//			cur++;
//		}
//		else
//		{
//			_matchOut[cur] = _matchOut.back();
//			_matchOut.pop_back();
//		}
//	}
//}

uint64_t PatternMatcher::clustering_classification_PM_shedding(int _stateID, int _timeSliceID, int _clusterID, double _quota)
{
    uint64_t sheddingCnt = m_States[_stateID].buffers[_timeSliceID][_clusterID].front().size();

    for(auto && C : m_States[_stateID].buffers[_timeSliceID][_clusterID])
    {
        C.clear();
    }
    m_States[_stateID].count[_timeSliceID][_clusterID] = 0;
    m_States[_stateID].firstMatchId[_timeSliceID][_clusterID] = 0;

    return sheddingCnt;
}

void PatternMatcher::clustering_classification_PM_shedding_4_typeC(int _state, int lowerBound, int upperBound)
{
    m_States[_state].TypeClowerBound = lowerBound;
    m_States[_state].TypeCupperBound = upperBound;
}

void PatternMatcher::clustering_classification_PM_shedding_selectivity(double ratio)
{
    int  r = ratio*100;

    if(r < 10 && r>3)
    {
        m_States[1].SelectivityPMDiceUB = r-3;
        m_States[2].SelectivityPMDiceUB = r+1;
    }
    else if(r >= 10)
    {
        m_States[1].SelectivityPMDiceUB = r-5;
        m_States[2].SelectivityPMDiceUB = r+1;
    }

   // switch(r)
   // {
   //     case 10:
   //     case 20:
   //     case 30:
   //     case 40:
   //     case 50:
   //     case 60:
   //     case 70:
   //         m_States[1].SelectivityPMDiceUB = r-5;
   //         m_States[2].SelectivityPMDiceUB = r+1;
   //         break;
   //     case 80:
   //         m_States[1].SelectivityPMDiceUB = 75;
   //         m_States[2].SelectivityPMDiceUB = 81;
   //         break;
   //     case 90:
   //         m_States[1].SelectivityPMDiceUB = 85;
   //         m_States[2].SelectivityPMDiceUB = 91;
   //         break;
   //     default:
   //         break;
   // }

}

void PatternMatcher::clustering_classification_PM_shedding_random(int _state, double ratio)
{
    m_States[_state].PMDiceUB = ratio*100;
}

void PatternMatcher::clustering_classification_PM_shedding_semantic(double _ratio)
{
    m_States[2].PMSmDropRatio = int(_ratio*100);
    m_States[1].PMSmDropRatio = int(_ratio*100);
}

void PatternMatcher::clustering_classification_PM_shedding_semantic_setPMSheddingCombo(int _combo)
{
    m_States[1].PMSheddingCombo = _combo;
    m_States[2].PMSheddingCombo = _combo;
}
    
uint64_t PatternMatcher::clustering_classification_PM_random_shedding(uint64_t _quota)
{
    cout << "[clustering_classification_PM_random_shedding] " << endl;
    int clusterCnt = 0;
    for(auto && S : m_States)
    {
        if(S.ID == 0)
            continue;
        if(S.type == ST_ACCEPT)
            break;
        for(auto && T : S.buffers)
            //for(auto && C:)
            clusterCnt += T.size();
    }

    cout << "[clustering_classification_PM_random_shedding] clusterCnt" << clusterCnt << endl;

    uint64_t perQuota = _quota/clusterCnt; 

    cout << "[clustering_classification_PM_random_shedding] perQuota" << perQuota << endl;
    //for(auto && S: m_States)
    //{
    //    if(S.ID = 0)
    //        continue; 
    //    if(S.type == ST_ACCEPT)
    //        break;
    //    for(auto && T : S.buffers)
    //    {
    //        int sheddingCnt = 0;
    //        for(auto && C : T)
    //        {
    //            ; // This is not finished 
    //            
    //        }
    //    }
    //}
    //
    
    int64_t sheddingGapCnt = 0;
    uint64_t sheddingCnt = 0;

    for(int stateIt = 1; stateIt < m_States.size(); ++stateIt)
    {
        //cout << "[clustering_classification_PM_random_shedding] for stateIt " << stateIt << endl;
        if(m_States[stateIt].type == ST_ACCEPT)
            break;

        for(int timesliceIt = 0; timesliceIt < m_States[stateIt].buffers.size(); ++timesliceIt)
            for(int clusterIt = 0; clusterIt < m_States[stateIt].buffers[timesliceIt].size(); ++clusterIt)
            {

                //cout << "[clustering_classification_PM_random_shedding] for timesliceIt, clusterIt, timeslice.size() : " << timesliceIt << "," << clusterIt <<"," << m_States[stateIt].buffers[timesliceIt].size()<< endl;
                uint64_t cnt  =  clustering_classification_PM_random_shedding(stateIt, timesliceIt, clusterIt, perQuota+sheddingGapCnt);
                sheddingGapCnt = perQuota - cnt; 
                sheddingCnt += cnt;
                //cout << "[clustering_classification_PM_random_shedding] for sheddingGapCnt, sheddingCnt: " << sheddingGapCnt<< "," << sheddingCnt<< endl;
            }
    }

    //if(sheddingGapCnt >= 0)
    //    return _quota - sheddingGapCnt;
    //else
    //    return _quota;

    return sheddingCnt;
}

uint64_t PatternMatcher::clustering_classification_PM_random_shedding(int _stateID, int _timeSliceID, int _clusterID, uint64_t _quota)
{
    uint64_t sheddingCnt = 0;

    srand(time(NULL));
    
    //cout << "perform random load shedding" << endl;
    //for(auto && it : m_States[_stateID].buffers[_timeSliceID][_clusterID])
    uint64_t sheddingEnd =  m_States[_stateID].buffers[_timeSliceID][_clusterID].front().size();
    
    

            
    for(int cnt=0; cnt < sheddingEnd; ++cnt)
    {
        //cout <<"[R shedding 1] " << endl;
        uint64_t ran = rand() % (sheddingEnd -1);

        if(m_States[_stateID].buffers[_timeSliceID][_clusterID].front()[ran] > 0)
        {
            m_States[_stateID].buffers[_timeSliceID][_clusterID].front()[ran] = 0;
            sheddingCnt++;
        }

        if(sheddingCnt > _quota)
            break;
    }

    //if(sheddingCnt < _quota )
    //    return _quota - sheddingCnt;
    //else 
    //    return 0;
    //
    return sheddingCnt;
}




void PatternMatcher::computeScores4LoadShedding()
{
    double a = 1;
    double b = 0.5;
    double c = 3;
    for(auto && it : this->m_States)
    {
        if(it.type == ST_ACCEPT)
            break;
        if(it.ID == 0)
            continue;
        it.scoreTable.clear();
        it.scoreTable.reserve(1024);
        for(auto && iterConsumption : it.consumptions)
        {
            auto iter = it.contributions.find(iterConsumption.first);
            if(iter != it.contributions.end())
            {
                it.scoreTable.push_back(make_pair(iterConsumption.first, 2048*iter->second - iterConsumption.second));
                //it.scoreTable.push_back(make_pair(iterConsumption.first, 2048*iter->second ));
                //it.scoreTable.push_back(make_pair(iterConsumption.first, 0 - 0.1*iterConsumption.second));
            }
            else
            {
                it.scoreTable.push_back(make_pair(iterConsumption.first, 0 - iterConsumption.second));
            }

            sort(it.scoreTable.begin(), it.scoreTable.end(), [](const pair<attr_t,double> & lhs, const pair<attr_t, double> &rhs) {
                    return lhs.second < rhs.second;
                    });
        }
    }
}

void PatternMatcher::computeScores4LoadShedding_VLDB16()
{
    for(auto && it : this->m_States)
    {
        if(it.type == ST_ACCEPT)
            break;
        if(it.ID == 0)
            continue;
        it.scoreTable_VLDB16.clear();
        it.scoreTable_VLDB16.reserve(1024);
            for(auto && iter : it.contributions)
            {
                    it.scoreTable_VLDB16.push_back(make_pair(iter.first, iter.second));
            }

            sort(it.scoreTable_VLDB16.begin(), it.scoreTable_VLDB16.end(), [](const pair<attr_t,uint64_t> & lhs, const pair<attr_t, uint64_t> &rhs) {
                    return lhs.second < rhs.second;
                    });
    }
}

uint64_t PatternMatcher::loadShedding_VLDB16(uint64_t quota, int _state1, int _state2)
{
//    uint64_t sheddingCnt = m_States[_state1].size()*quota + m_States[_state2].size()*quota;
    long long sheddingCnt = quota; 
    long long sheddingTargetCnt = sheddingCnt;
    

    computeScores4LoadShedding_VLDB16();

    auto iter1 = m_States[_state1].scoreTable_VLDB16.begin();
    auto iter2 = m_States[_state2].scoreTable_VLDB16.begin();
    
    for(; iter1 != m_States[_state1].scoreTable_VLDB16.end() &&  iter2 != m_States[_state2].scoreTable_VLDB16.end() && sheddingCnt > 0; ++iter1, ++iter2)
    {
        uint64_t sheddingKey = iter1->first;

        if(iter1->second > iter2->second)
        {
            //cout << "[loadShedding_VLDB16] if flag 1" << endl;
            sheddingKey = iter2->first;  

            for(int timesliceIt=0; timesliceIt < m_States[_state2].buffers.size(); ++timesliceIt)
                for(int clusterIt =0; clusterIt < m_States[_state2].buffers.front().size(); ++clusterIt)
                {
                    auto range = m_States[_state2].KeyAttributeIndex[timesliceIt][clusterIt].equal_range(sheddingKey);
                    for (auto it = range.first; it != range.second; ++it)
                    {
                        uint64_t _firstMatchID = m_States[_state2].firstMatchId[timesliceIt][clusterIt];
                        uint64_t _size =  m_States[_state2].buffers[timesliceIt][clusterIt].front().size();

                        if (it->second > _firstMatchID && it->second - _firstMatchID <  _size) 
                        {
                            uint64_t _idx = it->second - _firstMatchID;
                            if(m_States[_state2].buffers[timesliceIt][clusterIt].front()[_idx]) 
                            {
                                m_States[_state2].buffers[timesliceIt][clusterIt].front()[_idx] = 0; 
                                --sheddingCnt;
                            }
                        }
                    }
                }
            //cout << "[loadShedding_VLDB16] if flag 2" << endl;
        }

        else if(iter1->second < iter2->second)
        {
            //cout << "[loadShedding_VLDB16] else if flag 1" << endl;
            sheddingKey = iter1->first;  

            for(int timesliceIt=0; timesliceIt < m_States[_state1].buffers.size(); ++timesliceIt)
                for(int clusterIt =0; clusterIt < m_States[_state1].buffers.front().size(); ++clusterIt)
                {
                    auto range = m_States[_state1].KeyAttributeIndex[timesliceIt][clusterIt].equal_range(sheddingKey);
                    for (auto it = range.first; it != range.second; ++it)
                    {
                        uint64_t _firstMatchID = m_States[_state1].firstMatchId[timesliceIt][clusterIt];
                        uint64_t _size = m_States[_state1].buffers[timesliceIt][clusterIt].front().size();

                        if (it->second >= _firstMatchID && it->second - _firstMatchID < _size)
                        {
                            uint64_t _idx = it->second - _firstMatchID;
                            if(m_States[_state1].buffers[timesliceIt][clusterIt].front()[_idx]) 
                            {
                                m_States[_state1].buffers[timesliceIt][clusterIt].front()[_idx] = 0; 
                                --sheddingCnt;
                            }
                        }
                    }
                }
            //cout << "[loadShedding_VLDB16] else if flag 2" << endl;
        }
   
    }

    int _sheddingState = _state1;
    auto iter = iter1;

    if(iter1 == m_States[_state1].scoreTable_VLDB16.end())
    {
        iter = iter2;
        _sheddingState = _state2;
    }

    for(; iter != m_States[_sheddingState].scoreTable_VLDB16.end() && sheddingCnt > 0; ++iter) 
    {
        uint64_t sheddingKey = iter->first;
        for(int timesliceIt=0; timesliceIt < m_States[_sheddingState].buffers.size(); ++timesliceIt)
            for(int clusterIt =0; clusterIt < m_States[_sheddingState].buffers.front().size(); ++clusterIt)
            {
                auto range = m_States[_sheddingState].KeyAttributeIndex[timesliceIt][clusterIt].equal_range(sheddingKey);
                for (auto it = range.first; it != range.second; ++it)
                {
                    uint64_t _firstMatchID = m_States[_sheddingState].firstMatchId[timesliceIt][clusterIt];
                    uint64_t _size = m_States[_sheddingState].buffers[timesliceIt][clusterIt].front().size();

                    if (it->second >= _firstMatchID && it->second - _firstMatchID < _size)
                    {
                        uint64_t _idx = it->second - _firstMatchID;
                        if(m_States[_sheddingState].buffers[timesliceIt][clusterIt].front()[_idx]) 
                        {
                            m_States[_sheddingState].buffers[timesliceIt][clusterIt].front()[_idx] = 0; 
                            --sheddingCnt;
                        }
                    }
                }
            }
    }
    

    //cout << "[loadShedding_VLDB16] succ leave" << endl;
    return sheddingTargetCnt - sheddingCnt; 

    //cout << "[loadShedding_VLDB16] succ leave" << endl;
}



//uint32_t PatternMatcher::PMloadShedding(uint32_t stateID)
//{
//    uint32_t LoadCnt = m_States[stateID].attr.front().size() * G_PMShedRatio;
//    uint32_t cnt = LoadCnt;
//    while(LoadCnt)
//        m_States[stateID].attr.front()[LoadCnt--] = 0;
//    return cnt;
//}

//void PatternMatcher::loadShedding()
//{
//    cout << "perform load shedding" << endl;
//    for(auto && it : this->m_States)
//    {
//        if(it.type == ST_ACCEPT)
//            break;
//        if(it.ID == 0)
//            continue;
//            
//        int loadThreshold = it.attr.front().size() * 0.3  ;
//        //int loadThreshold = 0;
//        //if(it.ID == 1)
//        //    loadThreshold = it.attr.front().size() * 0.3;
//        int cnt = 0;
//
//        //try to reduce map index 
//        //vector<pair<multimap<attr_t, uint64_t>::iterator,multimap<attr_t, uint64_t>::iterator>> vec;
//        
//
//        //cout << "state size " << it.attr.front().size() << "timeout events so far ... " << it.firstMatchId << " indexmap size " << it.index.size() << endl;
//        for(auto && iterScore : it.scoreTable)
//        {
//            
//            if(cnt >= loadThreshold)
//                break;
//            //time_point<high_resolution_clock> t0 = high_resolution_clock::now();
//            //auto range = it.index.equal_range(iterScore.first);
//            pair<multimap<attr_t, uint64_t>::iterator,multimap<attr_t, uint64_t>::iterator> range = it.KeyAttributeIndex.equal_range(iterScore.first);
//            //if(range.first != range.second)
//            //vec.push_back(range);
//            //int ir = 0;
//            for(multimap<attr_t, uint64_t>::iterator i = range.first; i != range.second; ++i)
//            //for(auto i = range.first; i != range.second; ++i)
//            {
//                //if(i != it.index.end())
//                 //it.index.erase(i);
//                //if(i->second == 0)
//                //    continue;
//                uint32_t id = i->second - it.firstMatchId;
//                if(id < it.attr.front().size() && it.attr.front()[id] != 0)
//                //if(id < it.attr.front().size())
//                {
//                    //if(it.attr.front()[id] !=0)
//                    it.attr.front()[id] = 0; // set timestamp of a run to zero
//                    //i->second = 0; // This is expensive during runtime
//                    ++cnt;
//                    //it.index.erase(i);
//                    //++ir;
//                    //else
//                    //    --cnt;
//                }
//
//                //cout << "erase item in index" << endl;
//                
//            }
//
//            //it.KeyAttributeIndex.erase(range.first, range.second);
//            
//            //time_point<high_resolution_clock> t1 = high_resolution_clock::now();
//            //cout << "shed one key in score table time " << duration_cast<microseconds>(t1 - t0).count() << " #items with the same key " << ir << endl;
//        }
//
//        //cout << "vec size " << vec.size() << endl;
//        //for(auto && itRange: vec)
//        //{
//        //    //it.index.erase(itRange.first, itRange.second);
//        //      cout << "===" << itRange.first->first  << " , " << itRange.first->second  << "=====" << itRange.second->first << " , " << itRange.second->second << endl;
//        //    it.index.erase(itRange.first, itRange.second);
//        //      }
//    }
//}

//void PatternMatcher::randomLoadShedding()
//{
//    //if(Rflag == true)
//    //{
//    //srand(time(NULL));
//    srand((uint64_t)duration_cast<microseconds>(high_resolution_clock::now() - g_BeginClock).count()<<2);
//    //Rflag = false;
//    //}
//    //srand(time())
//    
//    cout << "perform random load shedding" << endl;
//    for(auto && it : this->m_States)
//    {
//        if(it.type == ST_ACCEPT)
//            break;
//        if(it.ID == 0)
//            continue;
//            
//        int loadThreshold = it.attr.front().size() / 5;
//        int cnt = 0;
//
//        for(int cnt=0; cnt < loadThreshold; ++cnt)
//        {
//                int size = it.attr.front().size();
//                int ran = rand() % size + 1;
//                
//                //if(it.attr.front()[ran] == 0)
//                //    --cnt;
//                //else
//                    it.attr.front()[ran] = 0;
//        }
//        
//        
//    }
//    
//}
//
uint32_t PatternMatcher::event(uint32_t _typeId, const attr_t* _attributes)
{
	uint32_t matchCount = 0;


    //cout << "[event]" <<  _attributes[0] -  m_Timeout << endl;
	if(_attributes[0] >= m_Timeout)
		for (State& state : m_States)
        {
            state.removeTimeouts(_attributes[0] - m_Timeout);
           // cout << "[event()] called state :" << state.ID  << " removeTimeouts: " << _attributes[0] - m_Timeout << endl; 
        }

    //cout << "time window" << m_Timeout << endl;
//    cout << "[PatternMatcher::event] flag 1" << endl;

    //if(_typeId == 1)
    //    m_States[1].removeTimeouts(_attributes[0] -  2*TTL);

	for (Transition& t : m_Transitions)
	{
		if (t.eventType != _typeId)
			continue;


 //   cout << "[PatternMatcher::event] flag 1.2" << endl;

		matchCount += t.checkEvent(m_States[t.from], m_States[t.to], 0, _attributes);
  //  cout << "[PatternMatcher::event] flag 1.3" << endl;
	}
   // cout << "[PatternMatcher::event] flag 2" << endl;

	for (State& state : m_States)
		state.endTransaction();
    //cout << "[PatternMatcher::event] flag 3" << endl;

	return matchCount;
}

//void PatternMatcher::run(uint32_t _stateId, uint32_t _runIdx, attr_t* _attrOut) const
//{
//	assert(_stateId < m_States.size());
//	assert(_runIdx < m_States[_stateId].count);
//
//	for (const auto& it : m_States[_stateId].attr)
//	{
//		*_attrOut++ = it[_runIdx];
//	}
//}

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


void PatternMatcher::addClusterTag(int stateID, int tsID, int clusterID, long double contribution, long double consumption)
{
    m_ClusterTags.push_back(ClusterTag(stateID, tsID, clusterID, contribution, consumption));
}

void PatternMatcher::resetRuns()
{
	for (uint32_t i = 0; i < m_States.size(); ++i)
	{
		clearState(i);
	}
}

PatternMatcher::State::State() :
	type(ST_NORMAL), kleenePlusAttrIdx(0),  index_attribute(0), /*firstMatchId(0), KeyAttrIdx(0),*/ accNum(0), transNum(0),stateBufferCount(0)
{
    for(int i=0; i<21; ++i)
        PMKeepingBook[i] = false;
}

PatternMatcher::State::~State()
{
}

void PatternMatcher::State::setCount(uint32_t _count)
{
    for(auto&& itTime:buffers)
        for(auto&& itCluster:itTime)
            for(auto&& itAttr: itCluster)
                itAttr.resize(_count);

    
//	assert(_count <= count || attr.empty());
//
//	count = _count;
//
//	for (auto& it : attr)
//		it.resize(_count);

}

void PatternMatcher::State::setAttributeCount(uint32_t _count)
{
	//assert(count == 0);
	//attr.resize(_count);
    for(auto&& itTime:buffers)
        for(auto&& itCluster:itTime)
            for(auto&& itAttr: itCluster)
                itAttr.resize(_count);
    
}

uint64_t PatternMatcher::approximate_BKP_PMshedding(double LOF)
{
    cout << "[PatternMatcher::approximate_BKP_PMshedding] called" << endl;
    ++m_CntCall_AppKNP_solver;

    uint64_t sheddingCnt = 0;
    uint64_t totalConsumption = 0;
    uint64_t sheddingConsumption = 0;

    bool sheddingFlag = true;


    //for(int i=1; i<m_States.size(); ++i)
    //{
    //    if(m_States[i].type == ST_ACCEPT)
    //        break;
    //    for(auto&& itTime: m_States[i].buffers)
    //        for(auto&& itCluster:itTime)
    //            numPMs += it.ClusterTag.front().size();
    //}

    //sheddingPMNum = numPMs * LOF;

    if(!m_ClusterTags_sorted)
        sortClusterTag();
    

    for(auto && tag: m_ClusterTags)
    {
        tag.size = m_States[tag.stateID].buffers[tag.timeSliceID][tag.clusterID].front().size();
        tag.TotalConsumption = tag.consumption*tag.size;
        totalConsumption += tag.TotalConsumption;
    }

    sheddingConsumption = totalConsumption * LOF ;


    for(auto && tag: m_ClusterTags)
    {
        if(!sheddingFlag)
            break;
        if(sheddingConsumption > tag.TotalConsumption)
        {
            cout << "[PatternMatcher::approximate_BKP_PMshedding] flag 1" << endl;
            sheddingCnt += clustering_classification_PM_shedding(tag.stateID, tag.timeSliceID, tag.clusterID);
            sheddingConsumption -= tag.TotalConsumption;
        }
        else
        {

            sheddingFlag = false;
            uint64_t sheddingQuota = sheddingConsumption / tag.consumption + 1;
            if(sheddingQuota > m_States[tag.stateID].buffers[tag.timeSliceID][tag.clusterID].front().size())
                sheddingQuota = m_States[tag.stateID].buffers[tag.timeSliceID][tag.clusterID].front().size(); 
            
            
            for(auto && attrDeque : m_States[tag.stateID].buffers[tag.timeSliceID][tag.clusterID])
                attrDeque.erase(attrDeque.begin(), attrDeque.begin()+sheddingQuota);

            sheddingCnt += sheddingQuota; 
        }

    }

    return sheddingCnt;

}

void PatternMatcher::sortClusterTag()
{
    std::sort(m_ClusterTags.begin(), m_ClusterTags.end(), [](const ClusterTag & lhs, const ClusterTag & rhs){
            return lhs.ccRatio < rhs.ccRatio;
            });
}


void PatternMatcher::State::setTimesliceClusterAttributeCount(int t, int c, int _count)
{
	assert(stateBufferCount== 0);
    numTimeSlice = t;
    buffers.resize(t);
    for(int i=0; i<t;++i)
    {
        buffers[i].resize(c);
        for(int j=0; j<c; ++j)
            buffers[i][j].resize(_count);
    }

    count.resize(t);
    for(int i=0; i<t; ++i)
        count[i].resize(c);

    index.resize(t);
        for(int i=0; i<t; ++i)
            index[i].resize(c);

    KeyAttributeIndex.resize(t);
        for(int i=0; i<t; ++i)
            KeyAttributeIndex[i].resize(c);


    firstMatchId.resize(t);
    for(int i=0; i<t; ++i)
        firstMatchId[i].resize(c);


	//attr.resize(_count);
}

void PatternMatcher::State::setIndexAttribute(uint32_t _idx)
{
    
	//assert(index.empty());
	index_attribute = _idx;
    //setKeyAttrIdx(_idx);
}




void PatternMatcher::State::insert(const attr_t * _attributes)
{
    //auto start = std::chrono::high_resolution_clock::now();
    //This is the place to call python libraries
	if(callback_insert)
		callback_insert(_attributes);
    
    if(type==ST_ACCEPT)
    {
        ++NumFullMatch;
        
        //const attr_t*  a = _attributes;
        //cout << "[insert] " << flush;
        //while(*a)
        //    cout << *(a++) << "--" << flush; 
        //cout << endl;
        //Ac[_attributes[1]]++;
        //Bc[_attributes[4]]++;
        //C[_attributes[1]+_attributes[4]]++;

        //add looking up cacahe, peftching,  in a blocking way.


      //  _attributes[Query::DA_FULL_MATCH_TIME] = (uint64_t)duration_cast<microseconds>(high_resolution_clock::now() - g_BeginClock).count();

        return;
    }




    //cout << "[insert] timeSliceSpan " << timeSliceSpan << endl;
    //cout << "[insert] timePointIdx" << timePointIdx << endl;
    //int _timeslice = (_attributes[timePointIdx]-_attributes[0])/timeSliceSpan;
    int _timeslice = 0;
    //int _t = (_attributes[timePointIdx]-_attributes[0]);
    //cout << "_timeslice " << _timeslice << endl;
   // if(ID == 2 && _t < 1001)
   // {
   //     if( (_attributes[1]+_attributes[4] == 9 ||  _attributes[1]+_attributes[4] == 10) ) 
   //         return;
   // } 


    //cout << "[insert] _timeslice : " << _timeslice << endl;
    
    //string parametersTuple4Cluster = to_string(ID-1)+","+to_string(_timeslice);

   // for(auto iter:ClusterAttrIdxs)
   // {
   //     parametersTuple4Cluster += string(",")+to_string(_attributes[iter]);
   //     //cout << iter << " , " << parametersTuple4Cluster << endl; 
   // }

   
    //cout << "[insert] state ID : "<< ID << "type : " << type << endl;
    //now call the python libraries for clustering

    //int clusterID = PythonCallerInstance.callPythonDSTree(parametersTuple4Cluster);
//    auto start1 = std::chrono::high_resolution_clock::now();
//
    //int clusterID = _m_distribution(_m_generator);
    int clusterID = 0; 

    //int clusterID = 0;
 //   auto elapsed1 = std::chrono::high_resolution_clock::now() - start1;
  //  long long microsecondsR = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed1).count();
    
   // switch (ID)
   // {
   //     case 1:
   //         A[_attributes[1]]++;
   //         break;
   //     case 2:
   //         A[_attributes[1]]++;
   //         B[_attributes[4]]++;
   //         break;
   //     default:
   //         break;
   // }

    if(TypeClowerBound != 0)
    {
        if(ID == 2)
        {
            if(_attributes[1]+_attributes[4] >= TypeClowerBound && _attributes[1]+_attributes[4] <= TypeCupperBound )
                clusterID = 0;
            else
            {
                //--NumPartialMatch;
                return;
            }
        }
        else if(ID == 1)
        {
            if(_attributes[1] >= TypeClowerBound && _attributes[1] <= TypeCupperBound)
                clusterID = 0;
            else
            {
                //--NumPartialMatch;
                return;
            }
        }
    }

//    if(ID ==2 && PMSheddingCombo ==2)
//    {
//        int PMAB = _attributes[1] + _attributes[4];
//        if(PMKeepingBook[PMAB] == false)
//        {
//            ++NumShedPartialMatch;
//            return;
//        }
//    }
    //else if(ID==1)
    //{
    //    if(_attributes[1] == 10)
    //        return;
    //}

    //if(ID == 1 && TypeClowerBound != 0)
    //{
    //    if(_attributes[1] >= TypeClowerBound && _attributes[1] <= TypeCupperBound)
    //        clusterID = 0;
    //    else
    //        return;
    //}

  //************** random PM shedding******************
    int PMDice_roll  = _m_distribution(_m_generator);
    if(PMDice_roll < PMDiceUB)
    {
        //--NumPartialMatch;
        ++NumShedPartialMatch;
        return;
    }
    // ******************************************
    
    //**************selectivity PM shedding***********
    if(PMDice_roll < SelectivityPMDiceUB)
    {
        //--NumPartialMatch;
        ++NumShedPartialMatch;
        return;
    }


    
	// insert attributes
	const attr_t* src_it = _attributes;
	for (auto& it : buffers[_timeslice][clusterID])
		it.push_back(*src_it++);

   // auto start2 = std::chrono::high_resolution_clock::now();
	if (index_attribute)
	{
		// update index
		const uint64_t matchId = firstMatchId[_timeslice][clusterID] + buffers[_timeslice][clusterID][0].size() - 1; // last matching point
    //cout <<"[insert] flag0.1" << endl;
    //cout << "index size : " << index.size() << endl; 
		index[_timeslice][clusterID].insert(make_pair(_attributes[index_attribute], matchId));
   // cout <<"[insert] flag0.2" << endl;
	}
    //auto elapsed2 = std::chrono::high_resolution_clock::now() - start2;
    //long long microseconds2 = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed2).count();
    //cout <<"[insert] flag0" << endl;

    //update C+/C- index for shedding
    //if(!KeyAttrIdx.empty())
    //{
    // //   cout <<"[insert] flag 0.1" << endl;
    //    const uint64_t matchId = firstMatchId[_timeslice][clusterID] + buffers[_timeslice][clusterID][0].size() - 1; // last matching point
    //  //  cout <<"[insert] flag 0.2" << endl;
    //    if(KeyAttrIdx.size()==1)
    //    {
    //        //cout <<"[insert] flag 0.3" << endl;
    //        //cout <<"KeyAttrIdx[0]--" << KeyAttrIdx[0] << endl;
    //        //cout << _attributes[KeyAttrIdx[0]] << endl;
    //        //cout << "KeyAttributeIndex timeSlice size--" << KeyAttributeIndex.size() <<  "_timeslice " << _timeslice << " #cluster ---" << KeyAttributeIndex[_timeslice].size() << endl;
    //        //cout << " insert clusterID " << clusterID << endl;
    //            KeyAttributeIndex[_timeslice][clusterID].insert(make_pair(_attributes[KeyAttrIdx[0]], matchId));
    //    }

    //    else if(KeyAttrIdx.size() == 2)
    //    {
    //        //cout <<"[insert] flag 0.4" << endl;
    //        KeyAttributeIndex[_timeslice][clusterID].insert(make_pair( pack(_attributes[KeyAttrIdx[0]], _attributes[KeyAttrIdx[1]]) , matchId));
    //    }
    //}

	//if (attr.empty())
	if (!buffers[_timeslice][clusterID].empty())
		count[_timeslice][clusterID]++;
    //cout <<"[insert] flag 0.5" << endl;


    //for contribution learning
   // ++accNum;
    //auto iter = valAccCounter.find(_attributes[KeyAttrIdx]);
    //if(iter == valAccCounter.end())
    //    valAccCounter.insert(std::make_pair(_attributes[KeyAttrIdx],1));
    //else
    //    iter->second++;
    //cout << KeyAttrIdx << "---" << _attributes[KeyAttrIdx] << endl;
    //auto elapsed = std::chrono::high_resolution_clock::now() - start;
    //long long microseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
    // cout << "single call time insert: in us " << microseconds << endl;
    // cout << "single call time randomGern: in us " << microsecondsR << endl;
    // cout << "update index: in ns " << microseconds2 << endl;

    ++NumPartialMatch;

}

void PatternMatcher::State::endTransaction()
{
	static_assert(std::numeric_limits<attr_t>::min() < 0, "invalid attr_t type");
    //cout <<"[PatternMatcher::State::endTransaction] flag1" << endl;

   //todo 
	//if (!attr.empty())
    //cout <<buffers.size() << endl;
	if (!buffers.empty())
	{
    //cout <<"[PatternMatcher::State::endTransaction] flag1.2" << endl;
		//count = (uint32_t)attr.front().size();

		for(auto idx : remove_list)
			buffers[idx.timesliceID][idx.clusterID].front()[idx.bufferID] = std::numeric_limits<attr_t>::min();
    //cout <<"[PatternMatcher::State::endTransaction] flag2" << endl;
	}
	else
	{
		//count -= (uint32_t)remove_list.size();
    //cout <<"[PatternMatcher::State::endTransaction] flag2.2" << endl;
	}
    //cout <<"[PatternMatcher::State::endTransaction] flag3" << endl;

	remove_list.clear();
}

template<typename T> auto begin(const std::pair<T, T>& _obj) { return _obj.first; }
template<typename T> auto end(const std::pair<T, T>& _obj) { return _obj.second; }

void PatternMatcher::State::removeTimeouts(attr_t _value) // The assumption is that event streams into CEP Engine in time order, the time order means the time order within a single <timeslice , cluster>
{


    //cout << "[removeTimeouts] flag1 in stateID: " << ID << endl;

    if(buffers.empty())
        return ; 
    int timesliceIt = 0;
	//	non attribute storing state (ACCEPT, REJECT)
    //cout << "[removeTimeouts] flag2" << endl;
    for( auto&& timeslice:buffers){
        //cout << "[removeTimeouts] flag3" << endl;
        int clusterIt = 0;
        for (auto&& attr: timeslice){
	if (attr.empty())
		return; // non attribute storing state (ACCEPT, REJECT)

	        while (!attr.front().empty() && attr.front().front() < _value) // the first deque stores timestamps
	        {
                //cout << " [removeTimeouts] in the while loop" << endl;
                // if time out, find all the relevant attribute in the index. Index stores all the indexes of events with the same attribute value.
                // e.g. a1b2,a1b4, if a1 timeout, a1b2,a1b4 must be removed
                //if(attr.front().front() != 0)
                //{
	        	    if (index_attribute)
	        	    {
	        	    	// update index
	        	    	auto range = index[timesliceIt][clusterIt].equal_range(attr[index_attribute].front());
	        	    	for (auto it = range.first; it != range.second; ++it)
	        	    	{
	        	    		//if (it->second == firstMatchId)
	        	    		if (it->second == firstMatchId[timesliceIt][clusterIt])
	        	    		{
	        	    			index[timesliceIt][clusterIt].erase(it);
	        	    			break;
	        	    		}
	        	    	}
	        	    }
                //}
                //
                //
                //remove index for C+/C- learning
	        	    if (!KeyAttrIdx.empty())
	        	    {
                        assert(KeyAttrIdx.size() <= 2);
                        
	        	    	// update KeyAttributeIndex 

                        if(KeyAttrIdx.size() == 1)
                        {
                            auto range = KeyAttributeIndex[timesliceIt][clusterIt].equal_range(attr[KeyAttrIdx[0]].front());
                            for (auto it = range.first; it != range.second; ++it)
                            {
                                //if (it->second == firstMatchId)
                                if (it->second == firstMatchId[timesliceIt][clusterIt])
                                {
                                    KeyAttributeIndex[timesliceIt][clusterIt].erase(it);
                                    break;
                                }
                            }
                        }
                        else if(KeyAttrIdx.size() == 2)
                        {
                            auto range = KeyAttributeIndex[timesliceIt][clusterIt].equal_range( pack(attr[KeyAttrIdx[0]].front(), attr[KeyAttrIdx[1]].front()) );
                            for (auto it = range.first; it != range.second; ++it)
                            {
                                //if (it->second == firstMatchId)
                                if (it->second == firstMatchId[timesliceIt][clusterIt])
                                {
                                    KeyAttributeIndex[timesliceIt][clusterIt].erase(it);
                                    break;
                                }
                            }
                        }
	        	    }


                //for loadshedding: leanring consumptions
                //cout << "in removeTimeouts, MonitoringLoad = " << PatternMatcher::MonitoringLoad() << endl;
                if(PatternMatcher::MonitoringLoad() == true)
                {
                    //cout << "consumption MonitoringLoad is on" << endl;
                    for(auto&& it : *states)
                    {
                        //std::cout << "in learning consumptions " << endl;
                        //std::cout << "it.ID = " << it.ID << "| this->ID =" << this->ID << endl;
                       if(it.ID > this->ID || it.KeyAttrIdx.empty())
                       {   
                           //std::cout << "touching continue point" << endl;
                           continue;
                    
                       }
                       //auto iterCon = it.consumptions.find( this->attr[it.KeyAttrIdx].front() );
                       ////cout << "flag 1" << endl;
                       //if(iterCon == it.consumptions.end())
                       //{
                       //    //cout << "flag 2" << endl;
                       //    it.consumptions.insert(std::make_pair(this->attr[it.KeyAttrIdx].front(),1));
                       //    //cout << "flag 3" << endl;

                       //}
                       //else
                       //{
                       //    //cout << "flag 4" << endl;
                       //    iterCon->second++;
                       //}
                       
                       if(KeyAttrIdx.size() == 1)
                           it.consumptions[attr[it.KeyAttrIdx[0]].front()] += 1;
                       else if(KeyAttrIdx.size() == 2)
                           it.consumptions[ pack(attr[it.KeyAttrIdx[0]].front(), attr[it.KeyAttrIdx[1]].front() ) ] += 1;

                       //cout << " added one item to comsuptions" << endl;
                    }
                }
                //learning consumption finish
                
                //cout << "flag 5" << endl;


                //cout << "partial match : " << endl;
                //
                if(m_externalIdx)
                    (*m_KeyUtility)[attr[m_externalIdx].front()]--;
	        	for (auto&& it : attr)
                {
                 //   cout << it.front() << " , ";
	        		it.pop_front();
                }
	        	firstMatchId[timesliceIt][clusterIt]++;
	        	count[timesliceIt][clusterIt]--;


	        }
            ++clusterIt;
        }
        ++timesliceIt;
    }

	//count = (uint32_t)attr.front().size();
	timeout = _value; // the possible earlist timestamp. The lower bound of a living time window.
}

void PatternMatcher::State::attributes(int _timeslice, int _cluster, uint32_t _idx, attr_t * _out) const
{
	for (const auto& it : buffers[_timeslice][_cluster])
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

void PatternMatcher::Transition::setCustomExecuteHandler(std::function<void(State&, State&, int, int, uint32_t, const attr_t*)> _handler)
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

//    cout << "[PatternMatcher::Transition::checkEvent] flag1 " << endl;

	return (this->*checkForMatch)(_from, _to, _runOffset, _attr);
    //uint32_t counter = (this->*checkForMatch) (_from, _to, _runOffset, _attr);
    
}

void PatternMatcher::Transition::updateContribution(State& _from, State& _to, uint32_t idx, attr_t valFrom, attr_t valTo, attr_t* _attributes)
{
    //cout << "[updateContribution] in flag 0 " << endl;

    if (_from.ID == 0)
        return;
    
    _from.transNum++;

    if(_to.type == ST_ACCEPT)
    {
    
        //cout << "state[" << _from.ID << "] KeyAttrIdx =" << _from.KeyAttrIdx << "--- state[" << _to.ID << "] KeyAttrIdx=" << _to.KeyAttrIdx << endl;
        //cout << valFrom << "---" << valTo << endl;
        //cout << "from index_attribute " << _from.index_attribute << "--- to index_attribute" << _to.index_attribute << endl;
        //cout << valFrom << "---" << valTo << endl;
        //must be called after state insert method for both _from and _to states
        if(!this->states)
        {
            cout << "states == " << this->states << endl;
            return;
        }
        //cout << "flag2 in updateContribution" << endl;
        for(auto&& it : *states )
        {
            //cout << "contribution learning ";
            if(it.type == ST_ACCEPT)
                break;
            if(!it.KeyAttrIdx.empty() )
            {
                //auto iterCon = it.contributions.find(_attributes[it.KeyAttrIdx]);
                //    if(iterCon == it.contributions.end())
                //    {
                //        it.contributions.insert(std::make_pair(_attributes[it.KeyAttrIdx],1));
                //    }
                //    else
                //        iterCon->second++;
                if(it.KeyAttrIdx.size() == 1)
                    it.contributions[_attributes[it.KeyAttrIdx[0]]] += 1;
                else if(it.KeyAttrIdx.size() == 2)
                    it.contributions[ pack(_attributes[it.KeyAttrIdx[0]], _attributes[it.KeyAttrIdx[1]]) ] += 1;

                //cout << "[updateContribution] C+ state " <<  it.ID << "---size() : " << it.contributions.size() << endl;
            }
            //cout << endl;

        }

        //cout << "[updateContribution] succ leave" << endl;


        //acceptCounter_t trans = 0;
        //double learningScore = 0;
        //

        //std::pair<attr_t, attr_t> matchPair = std::make_pair(valFrom, valTo);
        ////
        ////auto iterAcc = _from.valAccCounter.find(valFrom);
        ////if(iterAcc == _from.valAccCounter.end())
        //    //cout << "_from.valAccCounter is empty" << endl;
        //auto iterTran = _from.tranCounter.find(matchPair);

        ////if(_from.tranCounter.find(matchPair) == _from.tranCounter.end())
        //if(iterTran == _from.tranCounter.end())
        //{
        //    _from.tranCounter.insert(std::make_pair(matchPair,1));
        //    trans = 1; 
        //    //cout << "in test tranCounter.end()" << endl;
        //}
        //else
        //    trans = ++(iterTran->second);
        ////cout << "after ++iterTran->second" << endl;
        //    //learningScore = (_from.valAccCounter.find(valFrom)->second/_from.accNum) * (trans/_from.transNum);
        //if(iterAcc != _from.valAccCounter.end())
        //    learningScore = iterAcc->second * trans; 
        //cout << "after iterAcc->second" << endl;


        // cout << "leaving updateContribution" << endl;


        //_to.accNum++;

        //--following should be maitained in insert method...
        //if(_to.valAccCounter.find(valTo) == _to.valAccCounter.end())
        //    _to.valAccCounter.insert(std::make_pair(valTo,1));
        //else
        //    _to.valAccCounter.find(valTo)->second++;
    }
}

void PatternMatcher::setStates2Transitions()
{
   // cout <<"in setStates2Transitions" << endl;
    
    for(auto && it: m_Transitions)
    {
        it.states = &m_States;
    }

  //  cout << "leave setStates2Transitions" << endl;
}

void PatternMatcher::setStates2States()
{
    for(auto && it : m_States)
    {
        it.states = &m_States;
    }
}

void PatternMatcher::setTimeSliceSpan(uint64_t _timewindow)
{
    for(auto&& s : m_States)
        s.setTimeSliceSpan(_timewindow);
}

void PatternMatcher::Transition::executeTransition(State & _from, State & _to, int timeslice, int cluster, uint32_t _idx, const attr_t* _attributes)
{
	attr_t attributes[MAX_ATTRIBUTES];

	// copy attributes from previous events
	for (uint32_t a = 0; a < _from.buffers[timeslice][cluster].size(); ++a)
	{
		attributes[a] = _from.buffers[timeslice][cluster][a][_idx];
	}

	// copy new incoming event attributes
	for (auto it : actions)
    {
        attributes[it.dst] = _attributes[(int32_t)it.src];
    }
    //consume this PM from _from state, setting its timestamp to 0.
    if(_to.ID > 1) 
        _from.buffers[timeslice][cluster][0][_idx] = 0; 

    //for latency computing for load shedding
    //attributes[Query::DA_CURRENT_TIME] = _attributes[Query::DA_CURRENT_TIME];
    // check cache lookup here. 
    // look up cache by externIdx, if in
    //              check condition. if statified, insert, else return.  Set externalFlag to false
    // if cache not in Cache, insert.

    if(_to.m_externalIdx && _to.type == ST_ACCEPT) 
    {
        uint64_t key = attributes[_to.m_externalIdx];
        int Dice_roll  = _m_distribution(_m_generator);

        if(_to.m_externalComIdx && _to.type == ST_ACCEPT) 
        {
            uint64_t eventParam = attributes[_to.m_externalComIdx];
            uint64_t payload = _to.m_Cache->lookUp(key); 

            if(false && payload) //cache hit
            {
                ++(_to.m_Cache->cacheHit);
                if(payload > 5000) //check cache
                    return;

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

                if(payload > 5000)
                    return;
            }
        }
        (*_to.m_KeyUtility)[key]++ ;
    }

    if(_to.type == ST_ACCEPT)
    {
        attributes[Query::DA_FULL_MATCH_TIME] = (uint64_t)duration_cast<microseconds>(high_resolution_clock::now() - g_BeginClock).count();  
        attributes[Query::DA_CURRENT_TIME] = _attributes[Query::DA_CURRENT_TIME];
    }
    _to.insert(attributes);
}

void PatternMatcher::Transition::executeReject(State & _from, State & _to, int timesliceId, int clusterId, uint32_t _idx, const attr_t * _attributes)
{
	_from.remove(timesliceId, clusterId, _idx);
}

void PatternMatcher::Transition::executeCustom(State & _from, State & _to, int timesliceId, int clusterId, uint32_t _idx, const attr_t * _attributes)
{
	m_CustomExecuteHandler(_from, _to, timesliceId, clusterId, _idx, _attributes);
}

uint32_t PatternMatcher::Transition::checkNoCondition(State & _from, State & _to, size_t _runOffset, const attr_t * _eventAttr)
{
    //for all the PM in the state buffer.


	uint32_t counter = 0;

    int timesliceId = 0; 
    for(auto timesliceIt:_from.buffers)
    {
        int clusterId = 0;
        for(auto clusterIt:timesliceIt)
        {
	        for (uint32_t i = (uint32_t)_runOffset; i < _from.count[timesliceId][clusterId]; ++i)
	        {
	        	if (_from.runValid(timesliceId,clusterId,i))
	        	{
	        		(this->*executeMatch)(_from, _to, timesliceId, clusterId, i, _eventAttr);
	        		counter++;
	        	}
	        }
            ++clusterId;

        }
        ++timesliceId;
        
    }
	//uint32_t counter = 0;
	//for (uint32_t i = (uint32_t)_runOffset; i < _from.count; ++i)
	//{
	//	if (_from.runValid(i))
	//	{
	//		(this->*executeMatch)(_from, _to, i, _eventAttr);
	//		counter++;
	//	}
	//}
	return counter;
}

uint32_t PatternMatcher::Transition::checkSingleCondition(State & _from, State & _to, size_t _runOffset, const attr_t * _eventAttr)
{
    //cout << "[checkSingleCondition]" << endl;
	uint32_t counter = 0;
    //int timesliceId= 0;
    //for(auto&& timesliceIt : _from.buffers)
    //{
    //    int clusterId = 0;
    //    for(auto&& clusterIt : timesliceIt)
    //    {
	//        const Condition& c = conditions.front();
	//        testCondition(c, _from, _runOffset, _eventAttr, [&](uint32_t _idx) {
	//        	(this->*executeMatch)(_from, _to, timesliceId, clusterId, _idx, _eventAttr);
	//        	counter++;
	//        });

    //    }
    //}
    
    //cout <<"[checkSingleCondition] flag1 from, to states" << _from.ID << "," << _to.ID<<  endl;

    
	const Condition& c = conditions.front();
	testCondition(c, _from, _runOffset, _eventAttr, [&](int _timeslice, int _cluster, uint32_t _idx) {
		(this->*executeMatch)(_from, _to, _timeslice, _cluster, _idx, _eventAttr);
		counter++;
	});

	return counter;
}

uint32_t PatternMatcher::Transition::checkMultipleCondotions(State & _from, State & _to, size_t _runOffset, const attr_t * _eventAttr)
{
    
//   cout << "[checkMultipleCondotions] flag1 _from, _to states : " << _from.ID << "," << _to.ID << " Condition size: " << conditions.size() <<  endl;

   //for(auto a : conditions)
   //    cout << a.param[0] << ", " << a.param[1] << " , " << a.op << " , " << a.constant << endl;
	uint32_t counter = 0;

	//const Condition& c = conditions.front();
	//testCondition(c, _from, _runOffset, _eventAttr, [&](uint32_t _idx) {

	//	bool valid = true;

	//	for (size_t i = 1; i < conditions.size(); ++i)
	//	{
	//		const Condition& c = conditions[i];
	//		const attr_t runParam = _from.attr[c.param[0]][_idx];
	//		const attr_t eventParam = _eventAttr[c.param[1]] + c.constant;
	//		if (!checkCondition(runParam, eventParam, c.op))
	//		{
	//			valid = false;
	//			break;
	//		}
	//	}

	//	if(valid)
	//	{
	//		(this->*executeMatch)(_from, _to, _idx, _eventAttr);
	//		counter++;
	//	}

	//});

	const Condition& c = conditions.front();
	testCondition(c, _from, _runOffset, _eventAttr, [&](int _timeslice, int _cluster, uint32_t _idx) {

		bool valid = true;
 //       cout << "[checkMultipleCondotions] calling testCondition " << endl;

        //the first condition "conditions.front()" has been tested in the testCondition itself, this labmda funciton actually checks the condition from the second condition in the "conditions"
		for (size_t i = 1; i < conditions.size(); ++i)
		{
            
			const Condition& c = conditions[i];
            
  //          cout << "[checkMultipleCondotions] flag1.1" << endl;
            const attr_t runParam = _from.buffers[_timeslice][_cluster][c.param[0]][_idx];

   //         cout << "[checkMultipleCondotions] flag1.2" << endl;
            const attr_t eventParam = _eventAttr[c.param[1]] + c.constant;
            if(c.param[2] != 0)
            {
                assert(c.op == PatternMatcher::OP_ADD);
                
                const attr_t runParam1 = _from.buffers[_timeslice][_cluster][c.param[1]][_idx] + runParam;
                const attr_t eventParam1 = _eventAttr[c.param[2]] + c.constant;

			    if(!checkCondition(runParam1, eventParam1, c.op2))
			    {
			    	valid = false;
			    	break;
			    }

            //cout << "[checkMultipleCondotions] flag1.2 from state " << _from.ID << "---"<<runParam << "---" << runParam1 - runParam << "---" << eventParam1 << endl;
            }
            else
            {
			    if(!checkCondition(runParam, eventParam, c.op))
			    {
			    	valid = false;
			    	break;
			    }
            }
		}

        //cout << "[checkMultipleCondotions] Valid : " << valid << endl;
		if(valid)
		{
			(this->*executeMatch)(_from, _to, _timeslice, _cluster, _idx, _eventAttr);
			counter++;
		}

	});
	return counter;

    //cout << "[checkMultipleCondotions] flag2 " << endl;
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
    //to do ...
    //
    //for every timeslice and every cluster in the from state buffer
    //but not specify timeslice and cluster here], it is called in the testcondition methonds. 
    //the runs shoud contain timeslice cluster id and kleene attribute value.
	(this->*checkForMatchOrg)(_from, _to, _runOffset, _eventAttr);

	// sort resulting runs by timestamp
	//std::sort(runs.begin(), runs.end(), [](const runs_t::value_type& _a, const runs_t::value_type& _b) -> bool {
	//	return _a.second < _b.second;
	//});

	uint32_t counter = 0;
	const function<void(runs_t::const_iterator, runs_t::const_iterator)> func = [&](runs_t::const_iterator _top, runs_t::const_iterator _topend)
	{
		for (auto& a : _from.aggregation)
		{
			//attr_t runAttr = _from.attr[a.srcAttr][_top->first];
			attr_t runAttr = _from.buffers[_top->_timeslice][_top->_cluster][a.srcAttr][_top->_idx];
			(attr_t&)_eventAttr[a.dstAttr] = a.function->push(runAttr);
		}

		bool predicateOk = true;
		for (const auto & c : postconditions)
			predicateOk &= checkCondition(_eventAttr[c.param[0]], _eventAttr[c.param[1]] + c.constant, c.op);

		if (predicateOk)
		{
            // should execute the transition and pass the first element of the kleene part
            // runs[pmid].timeslice, cluster, id
			//(this->*executeMatchOrg)(_from, _to, runs.front()._idx, _eventAttr);
			(this->*executeMatchOrg)(_from, _to, _top->_timeslice, _top->_cluster, _top->_idx, _eventAttr);
			counter++;
		}

		//for (runs_t::const_iterator it = _top + 1; it != runs.end(); ++it)
		for (runs_t::const_iterator it = _top + 1; it <= _topend; ++it)
			func(it, _topend);

		for (auto& a : _from.aggregation)
			//a.function->pop(_from.attr[a.srcAttr][_top->first]);
			a.function->pop( _from.buffers[_top->_timeslice][_top->_cluster][a.srcAttr][_top->_idx]);

	};

	if (!runs.empty())
	{

        if(run_range.back().second != runs.size()-1)
            run_range.push_back(make_pair(run_range.back().second+1, runs.size()-1));

		if (runs.size() > 20)
			fprintf(stderr, "warning: aggregation over %u runs...", (unsigned)runs.size());

		for (auto& a : _from.aggregation)
			a.function->clear();
		//for (runs_t::const_iterator it = runs.begin(); it != runs.end(); ++it)
		//	func(it);
        for(auto &&it : run_range)
        {
            
            if(it.second-it.first > 20)
                fprintf(stderr,"warning: real aggregation is over %u runs..", (unsigned)(it.second-it.first));

            func(runs.begin()+it.first, runs.begin()+it.second);

            if(it.second-it.first > 20)
                fprintf(stderr,"done\n");

        }

        

		if (runs.size() > 20)
			fprintf(stderr, "done\n");

		runs.clear();
        run_range.clear();
	}
	return counter;
}

void PatternMatcher::Transition::executeKleene(State & _from, State & _to, int _timeslice, int _cluster, uint32_t _idx, const attr_t * _attributes)
{

    //to do ...
	//runs.push_back(make_pair(_idx, _from.attr[_from.kleenePlusAttrIdx][_idx]));
    if(_from.Kleene_lastTimeStamp != _from.buffers[_timeslice][_cluster][_from.Kleene_LastStateTimeStampIdx][_idx]) // partial matches for a single section of kleene closure part 
    {
        run_range.push_back(make_pair(run_lastPos, runs.size()-1));
        run_lastPos = runs.size();
        _from.Kleene_lastTimeStamp = _from.buffers[_timeslice][_cluster][_from.Kleene_LastStateTimeStampIdx][_idx];
    
    }

	runs.push_back(runs_element(_timeslice, _cluster, _idx, _from.buffers[_timeslice][_cluster][_from.kleenePlusAttrIdx][_idx]));
}
