#pragma once
#include <thread>
#include <iostream>

using namespace std;
class EventTrenTimer
{
public:
    EventTrenTimer(bool &f, uint64_t d1, uint64_t d2, uint64_t &version): m_Trigger(f), m_DurationOn(d1), m_DurationOff(d2), m_Version(version) {}

    bool Start()
    {
       m_StopThread = false; 
       m_Thread = thread([=]() {
            while(!m_StopThread)
            {
                ++m_Version;
                m_Trigger = true;            
                //++m_Version
                cout << "generate event B" << endl;
                this_thread::sleep_for(chrono::seconds(m_DurationOn));

                m_Trigger = false;
                cout << "stop generating event B" << endl;
                this_thread::sleep_for(chrono::seconds(m_DurationOff));
            }
        });
       
    }

    ~EventTrenTimer()
    {
        if(!m_StopThread){
            m_StopThread = true;
            m_Thread.join();
        }
    }


private:
    bool &       m_Trigger;
    uint64_t    m_DurationOn;
    uint64_t    m_DurationOff;
    bool        m_StopThread;
    uint64_t&   m_Version;
    thread      m_Thread;    
};
