#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

using namespace std;

struct fetchItem
{
    uint64_t key;
    uint64_t payload;
    int      fetchStrategy;
    fetchItem(uint64_t k, uint64_t p, int f):key(k), payload(p),fetchStrategy(f){}
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



    thread     m_Thread;
    queue<fetchItem> out_Queue;
    queue<pair<uint64_t, int>>  in_Queue;
    atomic<bool> m_Stop;
    mutex m_lock;
    condition_variable data_cond;
};
