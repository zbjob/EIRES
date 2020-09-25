#pragma once

#include <map>
#include <unordered_map>
#include <algorithm>
#include <utility>
#include <thread>
#include <atomic>
#include <mutex> 
#include <string>
#include "FetchWorker.h"

using namespace std;

typedef uint64_t cacheKey_t; 
typedef uint64_t cacheUti_t; 

struct cacheTrackingFlag
{
    int fetchStrategy; 
    bool hit;
    cacheTrackingFlag(int f):fetchStrategy(f), hit(false) {}
    cacheTrackingFlag():fetchStrategy(0), hit(false){}
};

typedef std::pair<cachePld_t, cacheTrackingFlag> cacheLine_t; 
typedef std::multimap<cacheUti_t, cacheKey_t>::iterator heapIdx_t;


using namespace std;

class Cache
{
    public:
        unordered_map<cacheKey_t, cacheLine_t> cache;  
        multimap<cacheUti_t, cacheKey_t> cacheHeap;    
        unordered_map<cacheKey_t, heapIdx_t> cacheHeapIndex;  

        FetchWorker* fetchWorker;
        unordered_map<uint64_t, uint64_t>* utilityMap;

        mutex s_lock;

        int numWorker;

        uint64_t cacheCapacity;
        uint64_t HeapUpdateCnt;
        uint64_t KeyUpdateCnt;
        uint64_t BlockFectchCnt;

        uint64_t FetchHit[4];
        uint64_t FetchCached[4];
        uint64_t FetchNeverUse[4];
        uint64_t FetchUpdate[4];
        
        uint64_t cacheHit;
        uint64_t cacheMiss;

        set<uint64_t> keys;
        
        thread   m_Thread;
        atomic<bool> m_Stop;

        Cache(uint64_t c, FetchWorker* worker, int numThread);
        ~Cache();

        void stop();
        void join();
        void start();
        bool lookUp(cacheKey_t key, cachePld_t & payload);
        void update(cacheKey_t key, cachePld_t payload, cacheUti_t utility, int fetchStrategy);
        void setUtilityMap(unordered_map<uint64_t, uint64_t>* map);
};
