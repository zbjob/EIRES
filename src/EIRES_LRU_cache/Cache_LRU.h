#pragma once

#include <list>
#include <map>     
#include <unordered_map>     
#include <algorithm>     
#include <utility>     
#include <thread>     
#include <atomic>     
#include <mutex>     
#include "FetchWorker.h"  

struct cacheTrackingFlag                 
{                                        
    int fetchStrategy; //1: Pfetch.  2: Dfetch
    bool hit;                            
    cacheTrackingFlag(int f):fetchStrategy(f), hit(false) {}
    cacheTrackingFlag():fetchStrategy(0), hit(false){}
};                                       

typedef std::pair<uint64_t, cacheTrackingFlag> cacheLine_t;

using namespace std;

class LRUCache {
public:
    LRUCache(uint64_t capacity, FetchWorker* worker, int numThread); 
    ~LRUCache();

    uint64_t lookUp(uint64_t key); 
    void update(uint64_t key, uint64_t value, int fetchStrategy); 
    void stop();
    void join();
    void start();
    

    uint64_t m_capacity = 0;
    uint64_t KeyUpdateCnt;
    uint64_t BlockFectchCnt;
    uint64_t cacheHit;
    uint64_t cacheMiss;

    uint64_t FetchHit[4];
    uint64_t FetchCached[4];
    uint64_t FetchNeverUse[4];
    uint64_t FetchUpdate[4];


    thread   m_Thread;
    atomic<bool> m_Stop;
    FetchWorker* fetchWorker;
    int numWorker;

    mutex s_lock;

    list<uint64_t>   m_list;
    unordered_map<uint64_t, cacheLine_t> m_map;
    unordered_map<uint64_t, list<uint64_t>::iterator> m_mapPos;
};
