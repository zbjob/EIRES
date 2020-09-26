#include "../EventStream.h"
#include "../freegetopt/getopt.h"
#include <iostream>
#include <map>
#include <vector>
#include <random>

using namespace std;

int main(int _argc, char* _argv[])
{
	unsigned maxDelay = 10000000; // 10s
	unsigned delayRatio = 100; // 1%
	unsigned seed = 0xAB0816;

	int c;
	while ((c = getopt(_argc, _argv, "d:r:s:")) != -1)
	{
		switch (c)
		{
		case 'd':
			maxDelay = atol(optarg);
			break;
		case 'r':
			delayRatio = atol(optarg);
			break;
		case 's':
			seed = atol(optarg);
			break;
		default:
			abort();
		}
	}

	mt19937 mt(seed);
	const unsigned delayPropability = mt.max() / delayRatio;

	StreamEvent::setupStdIo();

	uint64_t eventCounter = 0;
	uint64_t delayCounter = 0;
	uint64_t delayCounterTime = 0;

	map<uint64_t, vector<StreamEvent> > delayQueue;

	StreamEvent event;
	while (event.read())
	{
		eventCounter++;

		if (mt() < delayPropability)
		{
			uint64_t delay = (uint64_t)mt() * (uint64_t)maxDelay / mt.max();
			delayCounterTime += delay;
			delayCounter++;
			delayQueue[event.attributes[0] + delay].push_back(event);
			continue;
		}

		while (!delayQueue.empty() && delayQueue.begin()->first < event.attributes[0])
		{
			for (auto& it : delayQueue.begin()->second)
				it.write();
			delayQueue.erase(delayQueue.begin());
		}

		event.write();
	}

	cerr << delayCounter << " of " << eventCounter << " events delayed by " << (delayCounterTime / delayCounter) << "us on average" << endl;
	return 0;
}
