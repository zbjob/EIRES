#pragma once

#include <map>
#include <unordered_map>
#include <algorithm>
#include <utility>
#include <thread>
#include <atomic>
#include <mutex> 
#include "FetchWorker.h"

typedef uint64_t cacheKey_t; 
typedef uint64_t cachePld_t; 
typedef uint64_t cacheUti_t; 

struct cacheTrackingFlag
{
    int fetchStrategy; //0: Pfetch.  1: Dfetch
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
        unordered_map<cacheKey_t, cacheLine_t> cache;  // acutal content
        multimap<cacheUti_t, cacheKey_t> cacheHeap;    // Topk highest utility
        unordered_map<cacheKey_t, heapIdx_t> cacheHeapIndex;  // Tracking key-heapIdx mapping, for updating

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

        thread   m_Thread;
        atomic<bool> m_Stop;

        Cache(uint64_t c, FetchWorker* worker, int numThread);
        ~Cache();

        void stop();
        void join();
        void start();
        cachePld_t lookUp(cacheKey_t key);
        void update(cacheKey_t key, cachePld_t payload, cacheUti_t utility, int fetchStrategy);
        void setUtilityMap(unordered_map<uint64_t, uint64_t>* map);
};
