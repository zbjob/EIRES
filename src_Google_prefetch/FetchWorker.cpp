#include "FetchWorker.h"
#include "_shared/GlobalClock.h"
#include <random> 
#include <chrono> 
#include <iostream>
#include <assert.h>

default_random_engine generator;
uniform_int_distribution<int> distribution(1,10);

FetchWorker::FetchWorker():m_Stop(false)
{
}

FetchWorker::~FetchWorker()
{
    if(m_Stop)
       join();
}

void FetchWorker::stop()
{
    m_Stop = true;
}

void FetchWorker::join()
{
    m_Stop = true;
    if(m_Thread.joinable())
    {
        m_Thread.join();
    }
}

void FetchWorker::fetch(uint64_t key, int fetchStrategy)
{
    lock_guard<mutex> lock(m_lock);
    in_Queue.push({key,fetchStrategy});
    data_cond.notify_one();
}

void FetchWorker::start()
{
    m_Thread = thread([&]() 
            {
            while(!m_Stop)
            {
                uint64_t key; 
                int fetchStrategy;

                unique_lock<std::mutex> lk(m_lock);
                data_cond.wait(lk,[this]{return !in_Queue.empty();});
                auto item = in_Queue.front();
                key = item.first;
                fetchStrategy = item.second;
                in_Queue.pop();
                lk.unlock();

                int latency = distribution(generator) * fetch_latency;
                this_thread::sleep_for(chrono::microseconds(latency));

                if(fetchStrategy <4)
                out_Queue.push(fetchItem(key,key,fetchStrategy));
            }
        });
}
