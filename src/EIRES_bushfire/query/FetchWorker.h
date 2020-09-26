#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <set>
using namespace std;

struct cachePld_t
{
    double temperature; 
    double humidity;
    cachePld_t():temperature(0),humidity(0){}
    cachePld_t(double t, double h):temperature(t),humidity(h){}
};

struct fetchItem
{
    uint64_t key;
    cachePld_t payload;
    int      fetchStrategy;
    fetchItem(uint64_t k, double t, double h, int f):key(k),payload(t,h),fetchStrategy(f){}
};

class FetchWorker
{
public:
    FetchWorker();
    ~FetchWorker();

    static int fetch_latency;

    void join();
    void fetch(uint64_t _key, int fetchStrategy);
    void start();
    void stop();
    fetchItem  wait_and_pop(); 



    std::set<uint64_t> keys;
    std::set<uint64_t> Outkeys;

    std::thread     m_Thread;
    std::queue<fetchItem> out_Queue;
    std::queue<std::pair<uint64_t, int>>  in_Queue;
    std::atomic<bool> m_Stop;
    mutable mutex m_lock;
    mutable mutex m_lock_out;
    condition_variable data_cond;
    condition_variable data_cond_out;
};
