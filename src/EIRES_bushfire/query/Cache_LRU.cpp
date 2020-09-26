#include "Cache_LRU.h"  
#include <iostream>

using namespace std;

LRUCache::LRUCache(uint64_t c, FetchWorker* worker, int numThread):m_capacity(c), fetchWorker(worker),  numWorker(numThread), m_Stop(false), KeyUpdateCnt(0), cacheHit(0), cacheMiss(0), BlockFectchCnt(0)
{
     for(int i=0; i<4; ++i)
     {
         FetchHit[i] = 0;
         FetchCached[i] = 0;
         FetchNeverUse[i] = 0;
         FetchUpdate[i] = 0;
     }
}

    
uint64_t LRUCache::lookUp(uint64_t key) {
    lock_guard<mutex> lock(s_lock);
    if(m_map.count(key) == 0)
        return 0;
    else
    {
        m_map[key].second.hit = true;
        ++FetchHit[m_map[key].second.fetchStrategy];

        return m_map[key].first;
    }
}

void LRUCache::update(uint64_t key, uint64_t value, int fetchStrategy) {
    lock_guard<mutex> lock(s_lock);
    if(m_map.count(key))
    {
        if(fetchStrategy)
        {
            m_map[key] = {value, cacheTrackingFlag(fetchStrategy)};
            ++FetchCached[fetchStrategy];
        }
        else
            fetchStrategy = m_map[key].second.fetchStrategy;

        
        uint64_t tmp = *m_mapPos[key];

        m_list.erase(m_mapPos[key]);
        m_list.push_front(tmp);
        m_mapPos[key] = m_list.begin();


    }
    else
    {
        if(m_list.size() < m_capacity)
        {
            m_list.push_front(key);
            m_mapPos[key] = m_list.begin();
            m_map[key] = {value, cacheTrackingFlag(fetchStrategy)};

            ++FetchCached[fetchStrategy];
        }
        else
        {
            uint64_t tmp = m_list.back();

            if(!m_map[tmp].second.hit)
                ++FetchNeverUse[m_map[tmp].second.fetchStrategy];
            m_map.erase(tmp);
            m_mapPos.erase(tmp);
            m_list.pop_back();

            m_list.push_front(key);
            m_map[key] = {value, cacheTrackingFlag(fetchStrategy)};
            m_mapPos[key] = m_list.begin();

            ++FetchCached[fetchStrategy];
        }
    }
}

LRUCache::~LRUCache()
{                
     m_Stop = true;
     m_Thread.join();
}          

void LRUCache::stop()
{                
    m_Stop = true;
}                

void LRUCache::join()
{                
    m_Stop = true;
    cout << "Cache joining" << endl;
    for(int i=0; i<numWorker; ++i)
        fetchWorker[i].join();
    if(m_Thread.joinable())
    {
        m_Thread.join();
        cout << "Cache_LRU joined" << endl;
    }
}                

void LRUCache::start()
{                
    m_Thread = thread([&]()
            {   
            while(!m_Stop)
            for(int i=0; i<numWorker; ++i)
            if(!fetchWorker[i].out_Queue.empty())
            { 

            fetchItem item;
            fetchWorker[i].wait_and_pop(item);
            //auto item = fetchWorker[i].out_Queue.front();  
            //fetchWorker[i].out_Queue.pop();

            update(item.key, item.payload, item.fetchStrategy);
            }   
            }); 
}                
