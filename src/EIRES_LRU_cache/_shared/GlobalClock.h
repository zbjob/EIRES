#pragma once
#include <chrono>
#include <cstdint>
using namespace std::chrono;

extern time_point<high_resolution_clock> g_BeginClock;
extern uint64_t NumFullMatch;
extern uint64_t NumHighLatency;
extern uint64_t NumPartialMatch;
extern uint64_t NumShedPartialMatch;

extern uint64_t A[11];
extern uint64_t Ac[11];
extern uint64_t B[11];
extern uint64_t Bc[11];
extern uint64_t C[21];

extern int G_numTimeslice;
extern int G_numCluster;
//int const LATENCY = 2000;
int const LATENCY = 20000;
const int G_EventBOn = 1;
const int G_EventBOff = 3;
const double G_PMShedRatio = 0.2;
const int    G_InputShedCnt = 500;

extern int fetching_latency;
