#include "Cache_ordered.h"
#include <iostream>
#include <assert.h>
using namespace std;


Cache::Cache(uint64_t c, FetchWorker* worker, int numThread):cacheCapacity(c), fetchWorker(worker),  numWorker(numThread), m_Stop(false), HeapUpdateCnt(0), KeyUpdateCnt(0), cacheHit(0), cacheMiss(0), BlockFectchCnt(0)
{
    for(int i=0; i<4; ++i)
    {
        FetchHit[i] = 0;
        FetchCached[i] = 0;
        FetchNeverUse[i] = 0;
        FetchUpdate[i] = 0;
    }
}

Cache::~Cache()
{
     m_Stop = true;
     m_Thread.join();
}

void Cache::stop()
{
    m_Stop = true;
}

void Cache::join()
{
    m_Stop = true;
    for(int i=0; i<numWorker; ++i)
        fetchWorker[i].join();
    if(m_Thread.joinable())
    {
        m_Thread.join();
    } 
}

void Cache::setUtilityMap(unordered_map<uint64_t, uint64_t>* map)
{
    utilityMap = map;
}

void Cache::setFutureUtilityMap(unordered_map<uint64_t, uint64_t>* map)
{
    futureUtilityMap = map;
}

void Cache::setUuFuWeight(double w) 
{
    UU_FUweight = w;
}

void Cache::start()
{
    m_Thread = thread([&]()
            {
                while(!m_Stop)
                {
                    for(int i=0; i<numWorker; ++i)
                    if(!fetchWorker[i].m_Stop && !fetchWorker[i].out_Queue.empty())
                    {
                    auto item = fetchWorker[i].out_Queue.front();  
                    fetchWorker[i].out_Queue.pop();

                    uint64_t util = UU_FUweight*(*utilityMap)[item.key] + (1-UU_FUweight)*(*futureUtilityMap)[item.key];
                    update(item.key, item.payload, util, item.fetchStrategy);
                    }
                }
            }
            );
}



cachePld_t Cache::lookUp(cacheKey_t key)
{
    lock_guard<mutex> lock(s_lock);
    if( cache.count(key) )
    {
        cacheLine_t & item = cache[key];

        item.second.hit = true;
        if(item.second.fetchStrategy<4)
            ++FetchHit[item.second.fetchStrategy];

        return cache[key].first;
    }
    else
        return 0;
}



void Cache::update(cacheKey_t key, cachePld_t payload, cacheUti_t utility, int fetchStrategy)
{
    if(m_Stop)
        return; 
    //assert(fetchStrategy < 4); 
    if(fetchStrategy >= 4) 
        return;

    lock_guard<mutex> lock(s_lock);
    
    if( cache.count(key))
    {
        //This item has already been cached. 
        if(fetchStrategy)
            cache[key].second = cacheTrackingFlag(fetchStrategy);
        else
            fetchStrategy = cache[key].second.fetchStrategy;

        ++KeyUpdateCnt;
        ++FetchCached[fetchStrategy];

        auto heap_iter_old = cacheHeapIndex[key];
        cacheHeapIndex[key] = cacheHeap.insert({utility, key});
        cacheHeap.erase(heap_iter_old);
    }
    else 
    {
        if(cache.size() <  cacheCapacity)
        {
            //fill up cache capacity- intial phase.
            cache[key] = {payload, cacheTrackingFlag(fetchStrategy)};
            cacheHeapIndex[key] = cacheHeap.insert({utility, key});
            ++FetchCached[fetchStrategy];
        }
        else
        {
            //Cache is full. 
            if(utility > cacheHeap.begin()->first) 
            {
                ++HeapUpdateCnt;
                cacheTrackingFlag &  item = cache[key].second;
                if(!item.hit)
                    ++FetchNeverUse[item.fetchStrategy];
                
                cache[key] = {payload, cacheTrackingFlag(fetchStrategy)};
                cache.erase(cacheHeap.begin()->second);

                ++FetchCached[fetchStrategy];

                cacheHeapIndex[key] = cacheHeap.insert({utility, key});
                cacheHeapIndex.erase(cacheHeap.begin()->second);
                cacheHeap.erase(cacheHeap.begin());
            }
        }
    }
}
