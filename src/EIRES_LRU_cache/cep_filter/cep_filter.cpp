#include "../EventStream.h"
#include <iostream>
#include <map>
#include <algorithm>

using namespace std;

class EventStats
{
public:
	EventStats() : m_NumEvents(0), m_NumAttributes(0)
	{}

	void add(const StreamEvent& _event)
	{
		m_NumEvents++;
		m_NumAttributes += _event.attributeCount;
	}

	friend std::ostream& operator << (std::ostream&, const EventStats&);

private:
	size_t	m_NumEvents;
	size_t	m_NumAttributes;
};

std::ostream& operator << (std::ostream& _stream, const EventStats& _stats) 
{
	return _stream << _stats.m_NumEvents << " events, " << _stats.m_NumAttributes << " attributes";
}

int main(int _argc, char* _argv[])
{
	StreamEvent::setupStdIo();

	const uint16_t typeFilter = StreamEvent::hash("GFail");

	map<uint16_t, EventStats>	stats;

	vector<size_t> eventFilter;
	for (int i = 1; i < _argc; ++i)
		eventFilter.push_back(atoi(_argv[i]));
	sort(eventFilter.begin(), eventFilter.end());

	size_t eventId = 0;
	StreamEvent event;
	for(;event.read();eventId++)
	{
		// filter event id by argument list
		if (!eventFilter.empty())
		{
			auto it = lower_bound(eventFilter.begin(), eventFilter.end(), eventId);
			if (it == eventFilter.end() || *it != eventId)
				continue;
		}

		// filter by timestamp
		if (event.attributes[0] == 0)
			continue;

		// filter google cluster task timeout events
//		if (event.typeHash != typeFilter)
//			continue;

		event.write();

		stats[event.typeIndex].add(event);
	}

	for (auto& it : stats)
		cerr << "event " << it.first << ": " << it.second << endl;

	return 0;
}
