#pragma once
#include <vector>
#include <functional>
#include <thread>
#include <queue>
#include <string>

class MonitorThread
{
public:
	MonitorThread();
	~MonitorThread();

	void addValue(std::function<uint64_t(void)> _callback);
	bool start(const char* _filename);
	void stop();

    bool start_monitoring_latency( std::string  _filename, std::queue<int> & latency_booking);

	template<typename T>
	void addValue(const T* _ptr, bool _relative)
	{
		addValue([=] {
			static uint64_t last = 0;
			uint64_t current = (uint64_t)*_ptr;
			if (_relative)
			{
				uint64_t diff = current - last;
				last = current;
				current = diff;
			}
			return current;
		});
	}

private:
	std::vector<std::function<uint64_t(void)> >	m_Values;
	std::thread									m_PollThread;
	volatile bool								m_StopThread;
};
