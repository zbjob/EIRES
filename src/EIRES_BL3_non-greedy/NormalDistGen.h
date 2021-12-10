#pragma once
#include <iostream>
#include <string>
#include <random>
#include <fstream>
#include <functional>
#include <thread>
#include <queue>
#include <vector>
#include <utility>
#include "RingBuffer.h"

#include "PatternMatcher.h"
#include "Query.h"

class NormalEvent
{
    public:
        NormalEvent(std::string N): name(N){}
        NormalEvent(){}
        ~NormalEvent(){}
       
        //attr_t attributes[EventDecl::MAX_ATTRIBUTES];
        attr_t  v1;
        attr_t  v2;
        attr_t  ArrivalQTime;
        attr_t  ID;

        std::string name;
};

// This version is not thread safe, because STL queue is not thread safe.
// However, for only one producing thread and only one consuming thread, it almost works fine.
// For multiple producing/consuming threads, a concurrent queue is necessary.

class NormalDistGen
{
public:
    NormalDistGen(RingBuffer<NormalEvent>& Q, int size, uint64_t events):m_Buffer(Q), m_BufferSize(size), eventCnt(events),m_StopThread(false) {};
    ~NormalDistGen(){};

    void run(double e11, double d11, double e12, double d12, double e21, double d21, double e22, double d22, double e31, double d31, double e32, double d32);
    bool stop();
    void addDistribution(double e, double d);
    bool isStop() {return m_StopThread;}


private:
    std::thread         m_GenThread;
    volatile bool       m_StopThread;
    //std::queue<NormalEvent>&  m_Buffer;
    RingBuffer<NormalEvent>&  m_Buffer;
    int                 m_BufferSize; // abstract length of the queue, independet with physical event qeueue
    uint64_t            eventCnt;
    std::vector<std::pair<double, double>>       NormalDistributions;

    
};
