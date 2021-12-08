#include "MonitorThread.h"
#include <iostream>
#include <fstream>

MonitorThread::MonitorThread()
{
	m_StopThread = true;
}

MonitorThread::~MonitorThread()
{
	stop();
}

void MonitorThread::addValue(std::function<uint64_t(void)> _callback)
{
	m_Values.push_back(_callback);
}

bool MonitorThread::start(const char * _filename)
{
	using namespace std;

	m_StopThread = false;
	m_PollThread = thread([=]() {
		ofstream file;
		file.open(_filename);

		while (!m_StopThread)
		{
			for (size_t i = 0; i < m_Values.size(); ++i)
			{
				file << m_Values[i]();

				if (i != m_Values.size()-1)
					file << ',';
			}
			file << endl;
			//this_thread::sleep_for(chrono::milliseconds());
			this_thread::sleep_for(chrono::seconds(1llu));
		}
		file.close();
	});
	return true;
}

bool MonitorThread::start_monitoring_latency( std::string _filename, std::queue<int> & latency_booking)
{
	using namespace std;

	m_StopThread = false;
	m_PollThread = thread([&]() {
		ofstream file;
		file.open(_filename.c_str());
        file << "monitoring latency " << endl;
        cout << "monitoring latency " << endl;

		while (!m_StopThread)
		{
            while(!latency_booking.empty())
            {
                file << latency_booking.front() << endl;
        //        cout << latency_booking.front() << endl;
                latency_booking.pop();
            }
			//this_thread::sleep_for(chrono::milliseconds());
			this_thread::sleep_for(chrono::seconds(1llu));
		}
		file.close();
	});
	return true;
}


void MonitorThread::stop()
{
	if (!m_StopThread)
	{
		m_StopThread = true;
		m_PollThread.join();
	}
}
