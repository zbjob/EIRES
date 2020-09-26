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
    const attr_t eventParam = _attr[_condition.param[1]] + _condition.constant;
    int timesliceIt = 0;
    for(auto&& timeslice: _state.buffers)
    {
        int clusterIt = 0;

        for(auto&& cluster: timeslice)
        {

            if(_condition.param[2] == 0) 
            {
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
                    const auto& list = cluster[_condition.param[0]]; 

                    for (uint32_t idx = (uint32_t)_runOffset; idx < _state.count[timesliceIt][clusterIt]; ++idx)
                    {
                        if (checkCondition(list[idx], eventParam, _condition.op) && _state.runValid(timesliceIt,clusterIt,idx))
                        {
                            _callback(timesliceIt,clusterIt,idx);
                        }
                    }
                }
            }
            else 
            {
                cout << "[checkCondition] in the three attr, two operator case" << endl; 
                assert (_condition.op == PatternMatcher::OP_ADD);
                const attr_t eventParam3 = _attr[_condition.param[2]] + _condition.constant;
                cout << " eventParam3 : " << eventParam3 << endl;
                const auto& list = cluster[_condition.param[0]]; 
                const auto& list2 = cluster[_condition.param[1]]; 

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
            clusterCnt += T.size();
    }

    cout << "[clustering_classification_PM_random_shedding] clusterCnt" << clusterCnt << endl;

    uint64_t perQuota = _quota/clusterCnt; 

    cout << "[clustering_classification_PM_random_shedding] perQuota" << perQuota << endl;

    int64_t sheddingGapCnt = 0;
    uint64_t sheddingCnt = 0;

    for(int stateIt = 1; stateIt < m_States.size(); ++stateIt)
    {
        if(m_States[stateIt].type == ST_ACCEPT)
            break;

        for(int timesliceIt = 0; timesliceIt < m_States[stateIt].buffers.size(); ++timesliceIt)
            for(int clusterIt = 0; clusterIt < m_States[stateIt].buffers[timesliceIt].size(); ++clusterIt)
            {

                uint64_t cnt  =  clustering_classification_PM_random_shedding(stateIt, timesliceIt, clusterIt, perQuota+sheddingGapCnt);
                sheddingGapCnt = perQuota - cnt; 
                sheddingCnt += cnt;
            }
    }

    return sheddingCnt;
}

uint64_t PatternMatcher::clustering_classification_PM_random_shedding(int _stateID, int _timeSliceID, int _clusterID, uint64_t _quota)
{
    uint64_t sheddingCnt = 0;

    srand(time(NULL));

    uint64_t sheddingEnd =  m_States[_stateID].buffers[_timeSliceID][_clusterID].front().size();

    for(int cnt=0; cnt < sheddingEnd; ++cnt)
    {
        uint64_t ran = rand() % (sheddingEnd -1);

        if(m_States[_stateID].buffers[_timeSliceID][_clusterID].front()[ran] > 0)
        {
            m_States[_stateID].buffers[_timeSliceID][_clusterID].front()[ran] = 0;
            sheddingCnt++;
        }

        if(sheddingCnt > _quota)
            break;
    }

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
        }

        else if(iter1->second < iter2->second)
        {
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

    return sheddingTargetCnt - sheddingCnt; 

}

uint32_t PatternMatcher::event(uint32_t _typeId, const attr_t* _attributes)
{
    uint32_t matchCount = 0;

    if(_attributes[0] >= m_Timeout)
        for (State& state : m_States)
        {
            state.removeTimeouts(_attributes[0] - m_Timeout);
        }

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

}

void PatternMatcher::State::setAttributeCount(uint32_t _count)
{
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

}

void PatternMatcher::State::setIndexAttribute(uint32_t _idx)
{

    index_attribute = _idx;
}

void PatternMatcher::State::insert(const attr_t * _attributes)
{
    if(callback_insert)
        callback_insert(_attributes);

    if(type==ST_ACCEPT)
    {
        ++NumFullMatch;

        return;
    }

    int _timeslice = 0;

    int clusterID = 0; 

    if(TypeClowerBound != 0)
    {
        if(ID == 2)
        {
            if(_attributes[1]+_attributes[4] >= TypeClowerBound && _attributes[1]+_attributes[4] <= TypeCupperBound )
                clusterID = 0;
            else
            {
                return;
            }
        }
        else if(ID == 1)
        {
            if(_attributes[1] >= TypeClowerBound && _attributes[1] <= TypeCupperBound)
                clusterID = 0;
            else
            {
                return;
            }
        }
    }

    int PMDice_roll  = _m_distribution(_m_generator);
    if(PMDice_roll < PMDiceUB)
    {
        ++NumShedPartialMatch;
        return;
    }

    if(PMDice_roll < SelectivityPMDiceUB)
    {
        ++NumShedPartialMatch;
        return;
    }

    const attr_t* src_it = _attributes;
    for (auto& it : buffers[_timeslice][clusterID])
        it.push_back(*src_it++);

    if (index_attribute)
    {
        const uint64_t matchId = firstMatchId[_timeslice][clusterID] + buffers[_timeslice][clusterID][0].size() - 1; 
        index[_timeslice][clusterID].insert(make_pair(_attributes[index_attribute], matchId));
    }

    if (!buffers[_timeslice][clusterID].empty())
        count[_timeslice][clusterID]++;

    ++NumPartialMatch;

}

void PatternMatcher::State::endTransaction()
{
    static_assert(std::numeric_limits<attr_t>::min() < 0, "invalid attr_t type");

    if (!buffers.empty())
    {

        for(auto idx : remove_list)
            buffers[idx.timesliceID][idx.clusterID].front()[idx.bufferID] = std::numeric_limits<attr_t>::min();
    }
    else
    {
    }

    remove_list.clear();
}

template<typename T> auto begin(const std::pair<T, T>& _obj) { return _obj.first; }
template<typename T> auto end(const std::pair<T, T>& _obj) { return _obj.second; }

void PatternMatcher::State::removeTimeouts(attr_t _value) 
{

    if(buffers.empty())
        return ; 
    int timesliceIt = 0;
    for( auto&& timeslice:buffers){
        int clusterIt = 0;
        for (auto&& attr: timeslice){
            if (attr.empty())
                return; 

            while (!attr.front().empty() && attr.front().front() < _value) 
            {
                if (index_attribute)
                {
                    auto range = index[timesliceIt][clusterIt].equal_range(attr[index_attribute].front());
                    for (auto it = range.first; it != range.second; ++it)
                    {
                        if (it->second == firstMatchId[timesliceIt][clusterIt])
                        {
                            index[timesliceIt][clusterIt].erase(it);
                            break;
                        }
                    }
                }
                if (!KeyAttrIdx.empty())
                {
                    assert(KeyAttrIdx.size() <= 2);

                    if(KeyAttrIdx.size() == 1)
                    {
                        auto range = KeyAttributeIndex[timesliceIt][clusterIt].equal_range(attr[KeyAttrIdx[0]].front());
                        for (auto it = range.first; it != range.second; ++it)
                        {
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
                            if (it->second == firstMatchId[timesliceIt][clusterIt])
                            {
                                KeyAttributeIndex[timesliceIt][clusterIt].erase(it);
                                break;
                            }
                        }
                    }
                }

                if(PatternMatcher::MonitoringLoad() == true)
                {
                    for(auto&& it : *states)
                    {
                        if(it.ID > this->ID || it.KeyAttrIdx.empty())
                        {   
                            continue;

                        }

                        if(KeyAttrIdx.size() == 1)
                            it.consumptions[attr[it.KeyAttrIdx[0]].front()] += 1;
                        else if(KeyAttrIdx.size() == 2)
                            it.consumptions[ pack(attr[it.KeyAttrIdx[0]].front(), attr[it.KeyAttrIdx[1]].front() ) ] += 1;

                    }
                }

                if(m_externalIdx)
                    (*m_KeyUtility)[attr[m_externalIdx].front()]--;
                for (auto&& it : attr)
                {
                    it.pop_front();
                }
                firstMatchId[timesliceIt][clusterIt]++;
                count[timesliceIt][clusterIt]--;

            }
            ++clusterIt;
        }
        ++timesliceIt;
    }

    timeout = _value; 
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

    return (this->*checkForMatch)(_from, _to, _runOffset, _attr);

}

void PatternMatcher::Transition::updateContribution(State& _from, State& _to, uint32_t idx, attr_t valFrom, attr_t valTo, attr_t* _attributes)
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
        for(auto&& it : *states )
        {
            if(it.type == ST_ACCEPT)
                break;
            if(!it.KeyAttrIdx.empty() )
            {
                if(it.KeyAttrIdx.size() == 1)
                    it.contributions[_attributes[it.KeyAttrIdx[0]]] += 1;
                else if(it.KeyAttrIdx.size() == 2)
                    it.contributions[ pack(_attributes[it.KeyAttrIdx[0]], _attributes[it.KeyAttrIdx[1]]) ] += 1;

            }

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

void PatternMatcher::setTimeSliceSpan(uint64_t _timewindow)
{
    for(auto&& s : m_States)
        s.setTimeSliceSpan(_timewindow);
}

void PatternMatcher::Transition::executeTransition(State & _from, State & _to, int timeslice, int cluster, uint32_t _idx, const attr_t* _attributes)
{
    attr_t attributes[MAX_ATTRIBUTES];

    for (uint32_t a = 0; a < _from.buffers[timeslice][cluster].size(); ++a)
    {
        attributes[a] = _from.buffers[timeslice][cluster][a][_idx];
    }

    for (auto it : actions)
    {
        attributes[it.dst] = _attributes[(int32_t)it.src];
    }

    if(_to.ID > 1 && !FLAG_greedy)
        _from.buffers[timeslice][cluster][0][_idx] = 0;

    if(_to.m_externalIdx) 
    {
        uint64_t key = attributes[_to.m_externalIdx];
        int Dice_roll  = _m_distribution(_m_generator);

        if(FLAG_prefetch && !_to.m_externalComIdx && !_to.m_externalFlagIdx && Dice_roll <= fetchProbability) 
        {
            uint64_t _key = key;
            if(Dice_roll <= noiseProbability)
                _key = 1;

            if(prefetchCnt++ % prefetchFrequency == 0)
            {
                _to.m_Cache->fetchWorker[fetchCnt++ % _to.m_num_FetchWorker].fetch(_key,1); 
                fetchCnt = fetchCnt % _to.m_num_FetchWorker;
            }
            prefetchCnt = prefetchCnt%prefetchFrequency;
        }

        if(!FLAG_delay_fetch && _to.m_externalComIdx && attributes[_to.m_externalFlagIdx]) 
        {
            uint64_t eventParam = attributes[_to.m_externalComIdx];
            uint64_t payload = _to.m_Cache->lookUp(key); 

            if(payload) 
            {
                ++(_to.m_Cache->cacheHit);
                if(FLAG_greedy && payload != eventParam) 
                    return;
                else if(!FLAG_greedy && payload > 5000)
                    return; 


                attributes[_to.m_externalFlagIdx] = 0;

                _to.m_Cache->update(key, payload, 0);
            }
            else
            {
                ++(_to.m_Cache->cacheMiss);

                int latency = fetch_latency_distribution(_m_generator) * fetch_latency; 
                this_thread::sleep_for(chrono::microseconds(latency));

                payload = key;
                _to.m_Cache->update(key, payload, 3);
                ++(_to.m_Cache->BlockFectchCnt);

                if(FLAG_greedy && payload != eventParam) 
                    return;
                else if(!FLAG_greedy && payload > 5000)
                    return; 
            }
        }
        if(FLAG_delay_fetch && _to.m_externalComIdx && attributes[_to.m_externalFlagIdx]) 
        {
            uint64_t eventParam = attributes[_to.m_externalComIdx];
            uint64_t payload = _to.m_Cache->lookUp(key); 

            if(payload) 
            {
                ++(_to.m_Cache->cacheHit);
                if(FLAG_greedy && payload != eventParam) 
                    return;
                else if(!FLAG_greedy && payload > 5000)
                    return; 

                attributes[_to.m_externalFlagIdx] = 0;

                _to.m_Cache->update(key, payload, 0);

            }
            else
            {
                ++(_to.m_Cache->cacheMiss);

                if(_to.type == ST_ACCEPT || Dice_roll <= relaxProbability)
                {
                    int latency = fetch_latency_distribution(_m_generator) * fetch_latency; 
                    this_thread::sleep_for(chrono::microseconds(latency));

                    payload = key;
                    _to.m_Cache->update(key, payload,  3);
                    ++(_to.m_Cache->BlockFectchCnt);

                    if(FLAG_greedy && payload != eventParam) 
                        return;
                    else if(!FLAG_greedy && payload > 5000)
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
    return counter;
}

uint32_t PatternMatcher::Transition::checkSingleCondition(State & _from, State & _to, size_t _runOffset, const attr_t * _eventAttr)
{
    uint32_t counter = 0;

    const Condition& c = conditions.front();
    testCondition(c, _from, _runOffset, _eventAttr, [&](int _timeslice, int _cluster, uint32_t _idx) {
            (this->*executeMatch)(_from, _to, _timeslice, _cluster, _idx, _eventAttr);
            counter++;
            });

    return counter;
}

uint32_t PatternMatcher::Transition::checkMultipleCondotions(State & _from, State & _to, size_t _runOffset, const attr_t * _eventAttr)
{

    uint32_t counter = 0;

    const Condition& c = conditions.front();
    testCondition(c, _from, _runOffset, _eventAttr, [&](int _timeslice, int _cluster, uint32_t _idx) {

            bool valid = true;

            for (size_t i = 1; i < conditions.size(); ++i)
            {

            const Condition& c = conditions[i];

            const attr_t runParam = _from.buffers[_timeslice][_cluster][c.param[0]][_idx];

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

            if(valid)
            {
                (this->*executeMatch)(_from, _to, _timeslice, _cluster, _idx, _eventAttr);
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

    uint32_t counter = 0;
    const function<void(runs_t::const_iterator, runs_t::const_iterator)> func = [&](runs_t::const_iterator _top, runs_t::const_iterator _topend)
    {
        for (auto& a : _from.aggregation)
        {
            attr_t runAttr = _from.buffers[_top->_timeslice][_top->_cluster][a.srcAttr][_top->_idx];
            (attr_t&)_eventAttr[a.dstAttr] = a.function->push(runAttr);
        }

        bool predicateOk = true;
        for (const auto & c : postconditions)
            predicateOk &= checkCondition(_eventAttr[c.param[0]], _eventAttr[c.param[1]] + c.constant, c.op);

        if (predicateOk)
        {
            (this->*executeMatchOrg)(_from, _to, _top->_timeslice, _top->_cluster, _top->_idx, _eventAttr);
            counter++;
        }

        for (runs_t::const_iterator it = _top + 1; it <= _topend; ++it)
            func(it, _topend);

        for (auto& a : _from.aggregation)
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

    if(_from.Kleene_lastTimeStamp != _from.buffers[_timeslice][_cluster][_from.Kleene_LastStateTimeStampIdx][_idx]) 
    {
        run_range.push_back(make_pair(run_lastPos, runs.size()-1));
        run_lastPos = runs.size();
        _from.Kleene_lastTimeStamp = _from.buffers[_timeslice][_cluster][_from.Kleene_LastStateTimeStampIdx][_idx];

    }

    runs.push_back(runs_element(_timeslice, _cluster, _idx, _from.buffers[_timeslice][_cluster][_from.kleenePlusAttrIdx][_idx]));
}
