#pragma once
#include "../EventStream.h"

#include <memory>
#include <thread>

class PatternMatcher;

class BatchWorker
{
public:
	enum {
		EVENTS_PER_BATCH = 128,
	};

	BatchWorker();
	~BatchWorker();

	void setPatternMatcher(std::unique_ptr<PatternMatcher> _matcher);
	void start();
	void join();

private:

	void threadProc();
	uint32_t readEvents();

	std::unique_ptr<PatternMatcher>	m_Matcher;
	StreamEvent						m_Event[EVENTS_PER_BATCH];
	std::thread						m_Thread;

	

};

