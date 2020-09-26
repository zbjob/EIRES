#include "BatchWorker.h"

#include <mutex>

BatchWorker::BatchWorker()
{

}

BatchWorker::~BatchWorker()
{
	join();
}

void BatchWorker::setPatternMatcher(std::unique_ptr<PatternMatcher> _matcher)
{
	m_Matcher = std::move(_matcher);
}

void BatchWorker::start()
{
	m_Thread = std::thread(std::bind(&BatchWorker::threadProc, this));
}

void BatchWorker::join()
{
	m_Thread.join();
}

void BatchWorker::threadProc()
{
	uint32_t numEvents;
	do
	{
		numEvents = readEvents();

		for (uint32_t i = 0; i < numEvents; ++i)
		{
			m_Matcher->event(m_Event[i].typeIndex, (attr_t*)m_Event[i].attributes);
		}

		m_Matcher->resetRuns();
	} while (numEvents);
}

uint32_t BatchWorker::readEvents()
{
	static std::mutex s_lock;
	std::lock_guard<std::mutex> lock(s_lock);

	uint32_t numEvents = 0;
	while (numEvents < EVENTS_PER_BATCH)
	{
		if (!m_Event[numEvents].read())
			break;

		numEvents++;
	}
	return numEvents;
}
