#include "../Query.h"
#include "../PatternMatcher.h"
#include "../EventStream.h"
#include "../_shared/PredicateMiner.h"
#include "../_shared/MonitorThread.h"

#include <vector>
#include <chrono>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <deque>
#include <assert.h>

using namespace std;

struct GFailEvent
{
	size_t		id;
	uint64_t	timestamp;
	uint64_t	task_id;
};

void main()
{
	StreamEvent::setupStdIo();

	deque<GFailEvent> events;
	multimap<attr_t, GFailEvent*> index;

	const uint64_t timeout = 60000000;
	size_t eventId = 0;
	StreamEvent event;

	const uint16_t typeFilter = StreamEvent::hash("GFail");
	while (event.read())
	{
		if (event.typeHash != typeFilter)
		{
			eventId++;
			continue;
		}

		while (!events.empty() && events.front().timestamp < (event.attributes[0] - timeout))
		{
			const GFailEvent& e1 = events.front();

			auto task_range = index.equal_range(e1.task_id);
			auto it2 = task_range.first; it2++;
			for(; it2 != task_range.second; ++it2)
			{
				const GFailEvent& e2 = *it2->second;

				auto it3 = it2; it3++;
				for(; it3 != task_range.second; ++it3)
				{
					const GFailEvent& e3 = *it3->second;

					StreamEvent out_event = {};
					out_event.typeIndex = 9;
					out_event.typeHash = StreamEvent::hash("GFail3");
					out_event.attributeCount = 4;
					out_event.attributes[0] = e1.timestamp;
					out_event.attributes[1] = e1.timestamp;
					out_event.attributes[2] = e2.timestamp;
					out_event.attributes[3] = e3.timestamp;
					out_event.write();
				}
			}

			index.erase(task_range.first);
			events.pop_front();
		}
		events.push_back({ eventId++, event.attributes[0], event.attributes[2] });
		index.insert(make_pair(events.back().task_id, &events.back()));
	}
}
