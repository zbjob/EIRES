#pragma once
#include <chrono>
using namespace std::chrono;

extern time_point<high_resolution_clock> g_BeginClock;
extern long int NumFullMatch;
extern long int NumHighLatency;
extern long int NumPM;
extern long int NumSheddingPM;
int const LATENCY = 150;
extern int fetching_latency;
