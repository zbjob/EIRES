#include "../Query.h"
#include "../PatternMatcher.h"
#include "../EventStream.h"
#include "../_shared/PredicateMiner.h"
#include "../_shared/MonitorThread.h"
#include "../_shared/GlobalClock.h"

#include "../freegetopt/getopt.h"
#include "../NormalDistGenChangePattern.h"

#include "../Cache_ordered.h"
#include "../FetchWorker.h"

#include <vector>
#include <chrono>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <random>
#include <set>
#include <queue>
#include <cstdlib>
#include <thread>

using namespace std;
using namespace std::chrono;

time_point<high_resolution_clock> g_BeginClock;
uint64_t NumFullMatch = 0;
uint64_t NumHighLatency = 0;
uint64_t NumPartialMatch= 0;
uint64_t NumShedPartialMatch= 0;

int fetching_latency = 1;

long int RealTimeLatency = 0;
int G_numTimeslice=1;
int G_numCluster = 4;
default_random_engine m_generator;
uniform_int_distribution<int> m_distribution(1,100); 
uniform_int_distribution<int> random_input_shedding_distribution(0,1); 

bool eventB = false;
bool eventZ= false;
uint64_t lastEventBVersion = 0;
uint64_t lastEventBTS = 0;
uint64_t PMSheddingTimer = 0;
int PMSheddingDice = 1;

uint64_t ACCLantency = 0;

int PatternMatcher::Transition::fetchCnt = 0;
int PatternMatcher::Transition::prefetchCnt = 0;
int PatternMatcher::Transition::prefetchFrequency = 1000;
int PatternMatcher::Transition::fetchProbability= 0;
int PatternMatcher::Transition::relaxProbability= 0;
int PatternMatcher::Transition::noiseProbability= 0;
bool PatternMatcher::Transition::FLAG_greedy= false;
int PatternMatcher::Transition::fetch_latency = 1;
bool PatternMatcher::Transition::FLAG_delay_fetch = false;
bool PatternMatcher::Transition::FLAG_prefetch = false;
int FetchWorker::fetch_latency = 1;

inline void init_utime() 
{
	g_BeginClock = high_resolution_clock::now();
}

inline uint64_t current_utime() 
{ 
	return duration_cast<microseconds>(high_resolution_clock::now() - g_BeginClock).count();
}

class CepMatch
{
public:
	CepMatch(queue<NormalEvent>& Q) : m_Query(0), m_DefAttrId(0), m_DefAttrOffset(0), m_NextMinerUpdateTime(0), RawEventQueue(Q)
	{
        lastSheddingTime = 0;
        loadCnt = 0;
        time = 60000000;
        acctime = time;
        Rtime = time/50000;
        latency = 0;
        cntFullMatch = 1;
        m_RandomInputSheddingFlag = false;
        m_InputSeddingSwitcher = false;
        m_PMSheddingSwitcher = false;
	}

	~CepMatch()
	{

	}

	bool init(const char* _defFile, const char* _queryName, const char* _miningPrefix, bool _generateTimeoutEvents, bool _appendTimestamp)
	{
		StreamEvent::setupStdIo();

		if (!m_Definition.loadFile(_defFile))
		{
			fprintf(stderr, "failed to load definition file %s\n", _defFile);
			return false;
		}

		m_Query = !_queryName ? m_Definition.query((size_t)0) : m_Definition.query(_queryName);
		if (!m_Query)
		{
			fprintf(stderr, "query not found");
			return false;
		}

		if(_miningPrefix)
		{
			m_MiningPrefix = _miningPrefix;
			m_Miner.reset(new PredicateMiner(m_Definition, *m_Query));

			for (size_t i = 0; i < m_Query->events.size() - 1; ++i)
			{
				uint32_t eventType = m_Definition.findEventDecl(m_Query->events[i].type.c_str());
				m_Miner->initList(i * 2 + 0, eventType);
				m_Miner->initList(i * 2 + 1, eventType);
			}

			unsigned numCores = thread::hardware_concurrency();
			m_Miner->initWorkerThreads(std::min(numCores - 1, 16u));

			const uint64_t one_min = 60 * 1000 * 1000;
			m_NextMinerUpdateTime = current_utime() + one_min / 6;
		}

		QueryLoader::Callbacks cb;
		cb.insertEvent[PatternMatcher::ST_ACCEPT] = bind(&CepMatch::write_event, this, false, placeholders::_1, placeholders::_2);

		if(_generateTimeoutEvents)
			cb.timeoutEvent = bind(&CepMatch::write_event, this, true, placeholders::_1, placeholders::_2);

		if (!m_Definition.setupPatternMatcher(m_Query, m_Matcher, cb))
		{
			return false;
		}

		m_Query->generateCopyList(m_Query->returnAttr, m_OutEventAttrSrc);

		m_ResultEventType = m_Definition.findEventDecl(m_Query->returnName.c_str());
		m_ResultEventTypeHash = StreamEvent::hash(m_Query->returnName);
		m_ResultAttributeCount = (uint8_t)m_OutEventAttrSrc.size();
		m_GenerateTimeoutEvents = _generateTimeoutEvents;
		m_AppendTimestamp = _appendTimestamp;

		return true;
	}

	bool processEvent()
	{
		StreamEvent event;
        NormalEvent RawEvent;
        NormalEvent RawEvent1;
        if(!RawEventQueue.empty())
        {
            RawEvent = RawEventQueue.front();
            RawEventQueue.pop();

        }
        else{ 
            cout << "event stream queue is empty now!" << endl;
            return false;
        }

        event.attributes[0] = RawEvent.ArrivalQTime;
        event.attributes[1] = RawEvent.v1;
        event.attributes[2] = RawEvent.v2;
        event.attributes[3] = RawEvent.ID;

        event.typeIndex = m_Definition.findEventDecl(RawEvent.name.c_str());

		event.attributes[Query::DA_ZERO] = 0;
		event.attributes[Query::DA_MAX] = numeric_limits<attr_t>::max();
		event.attributes[Query::DA_CURRENT_TIME] = current_utime();
		event.attributes[Query::DA_OFFSET] = m_DefAttrOffset;
		event.attributes[Query::DA_ID] = m_DefAttrId;

		const EventDecl* decl = m_Definition.eventDecl(event.typeIndex);

		m_Matcher.event(event.typeIndex, (attr_t*)event.attributes);

		if (m_Miner)
		{
			m_Miner->flushMatch();
			m_Miner->addEvent(event.typeIndex, (const attr_t*)event.attributes);
			m_Miner->removeTimeouts(event.attributes[0]);
		}

		m_DefAttrId++;
		m_DefAttrOffset += event.offset;

		if (m_Miner && m_NextMinerUpdateTime <= event.attributes[Query::DA_CURRENT_TIME])
		{
			const uint64_t one_min = 60 * 1000 * 1000;
			m_NextMinerUpdateTime = event.attributes[Query::DA_CURRENT_TIME] + 10 * one_min;
			update_miner();
		}

		return true;
	}

    void PMSheddingOn() { m_PMSheddingSwitcher = true;}
    void PMSheddingOff() {m_PMSheddingSwitcher = false;}
    bool PMShedding() { return m_PMSheddingSwitcher;}

    void InputSheddingOn() {m_InputSeddingSwitcher = true;}
    void InputSheddingOff() {m_InputSeddingSwitcher= false;}
    bool InputShedding()  { return m_InputSeddingSwitcher;}
    
    uint64_t  RandomInputShedding(int _quota, uint64_t & _eventCnt) 
    {
        int ToShedCnt = _quota;
        
        while(ToShedCnt > 0 && !RawEventQueue.empty())
        {
            int dice_roll = m_distribution(m_generator);
            if(dice_roll <= 50)
            {
                RawEventQueue.pop();
                --ToShedCnt;
            }
            else
            {
               if(processEvent()) 
                   _eventCnt++;
            }
        }

        return _quota - ToShedCnt;
    }

    uint64_t  RandomInputShedding(double ratio, volatile uint64_t & _eventCnt) 
    {
        uint64_t ShedCnt = 0;
        
        while(!RawEventQueue.empty())
        {
            ++_eventCnt;
            if(RawEventQueue.front().name == "D") 
            {
                processEvent();
                continue; 
            }
            int dice_roll = m_distribution(m_generator);
            if(dice_roll <=  (ratio *100) )
            {
                RawEventQueue.pop();
                ++ShedCnt;
            }
            else
            {
               processEvent(); 
            }
        }

        return ShedCnt;
    }

   uint64_t  VLDB_03_InputShedding(int _quota, uint64_t & _eventCnt) 
   {
       int ToShedCnt = _quota;
       NormalEvent RawEvent;
       while(ToShedCnt > 0 && !RawEventQueue.empty())
       {
           RawEvent = RawEventQueue.front(); 

           if(RawEvent.name == "A") 
           {
               if(RawEvent.v1 <= 33 || RawEvent.v1 > 53)
               {
                   RawEventQueue.pop();
                   --ToShedCnt;
               }
               else
               {
                   processEvent();
                   _eventCnt++; 
               }

           }
           else if(RawEvent.name == "B")
           {
               if(RawEvent.v2 > 54 || RawEvent.v2 < 43)
               {
                   RawEventQueue.pop();
                   --ToShedCnt;
               }
               else
               {
                   processEvent();
                   _eventCnt++;
               }

           }
           else
           {
               processEvent();
               _eventCnt++;
           }
       }

       return _quota - ToShedCnt;
   }

   uint64_t  VLDB_03_InputShedding(int ALowB, int AUpB, int BLowB, int BUpB, int CLowB, int CUpB, volatile uint64_t & _eventCnt) 
   {
       uint64_t ShedCnt = 0;
       NormalEvent RawEvent;
       while(!RawEventQueue.empty())
       {
           RawEvent = RawEventQueue.front(); 

           if(RawEvent.name == "A") 
           {
               if(RawEvent.v1 < ALowB || RawEvent.v1 > AUpB)
               {
                   RawEventQueue.pop();
                   ++ShedCnt;
                   _eventCnt++;
               }
               else
               {
                   processEvent();
                   _eventCnt++; 
               }

           }
           else if(RawEvent.name == "B")
           {
               if(RawEvent.v1 < BLowB || RawEvent.v1 > BUpB)
               {
                   RawEventQueue.pop();
                   ++ShedCnt;
                   _eventCnt++;
               }
               else
               {
                   processEvent();
                   _eventCnt++;
               }

           }
           else if(RawEvent.name == "C")
           {

               if( RawEvent.v1 < CLowB || RawEvent.v1 > CUpB )
               {
                   RawEventQueue.pop();
                   ++ShedCnt;
                   _eventCnt++;
               }
               else
               {
                   processEvent();
                   _eventCnt++;
               }
           }
           else
           {
               processEvent();
               _eventCnt++;
           }
       }

       return ShedCnt;
   }

   uint64_t  VLDB_03_InputShedding(double _ratio, volatile uint64_t & _eventCnt) 
   {
       uint64_t ShedCnt = 0;
       int ratio = int(_ratio * 100);
       NormalEvent RawEvent;

       switch (ratio)
       {
           case 10:
               {
                   const int   AUpB = 9;
                   const int   BUpB = 9;
                   const int   DiceUpB = int( double(10/11) * 100);

                   while(!RawEventQueue.empty())
                   {
                       int dice_roll =  m_distribution(m_generator);
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C" && RawEvent.v1 == 2 && dice_roll <= DiceUpB)
                       {

                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;

           case 20:
               {
                   const int    AUpB = 8;
                   const int    BUpB = 8;
                   const int    DiceUpB = int( double(9/11) * 100);

                   while(!RawEventQueue.empty())
                   {
                       int dice_roll =  m_distribution(m_generator);
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C" && (RawEvent.v1 == 2 || (RawEvent.v1 ==3 && dice_roll <= DiceUpB)))
                       {

                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;
           case 30:
               {
                   const int     AUpB = 7;
                   const int     BUpB = 7;
                   const int     DiceUpB = int( double(8/11) * 100);

                   while(!RawEventQueue.empty())
                   {
                       int dice_roll =  m_distribution(m_generator);
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C" && ( (RawEvent.v1 >=2 && RawEvent.v1 <=3) || (RawEvent.v1 ==4 && dice_roll <= DiceUpB)))
                       {

                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;
           case 40:
               {
                   const int     AUpB = 6;
                   const int     BUpB = 6;
                   const int     DiceUpB = int( double(7/11) * 100);

                   while(!RawEventQueue.empty())
                   {
                       int dice_roll =  m_distribution(m_generator);
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C" && ( (RawEvent.v1 >=2 && RawEvent.v1 <=4) ||  (RawEvent.v1 ==10 && dice_roll <= DiceUpB)))
                       {

                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;
           case 50:
               {
                   const int     AUpB = 5;
                   const int     BUpB = 5;
                   const int     DiceUpB = int( double(6/11) * 100);

                   while(!RawEventQueue.empty())
                   {
                       int dice_roll =  m_distribution(m_generator);
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C" && ( (RawEvent.v1 >=2 && RawEvent.v1 <=4) || (RawEvent.v1 >= 9) ||  (RawEvent.v1 ==8 && dice_roll <= DiceUpB)))
                       {

                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;
           case 60:
               {
                   const int     AUpB = 4;
                   const int     BUpB = 4;
                   const int     DiceUpB = int( double(5/11) * 100);

                   while(!RawEventQueue.empty())
                   {
                       int dice_roll =  m_distribution(m_generator);
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C" && 
                               ( (RawEvent.v1 >=7 ) || (RawEvent.v1 ==2) ||  (RawEvent.v1 ==3 && dice_roll <= DiceUpB)))
                       {

                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;
           case 70:
               {
                   const int     AUpB = 3;
                   const int     BUpB = 3;
                   const int     DiceUpB = int( double(4/11) * 100);

                   while(!RawEventQueue.empty())
                   {
                       int dice_roll =  m_distribution(m_generator);
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C" && 
                               ( (RawEvent.v1 >=6 ) || (RawEvent.v1 ==2) ||  (RawEvent.v1 ==3 && dice_roll <= DiceUpB)))
                       {

                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;
           case 80:
               {
                   const int     AUpB = 2;
                   const int     BUpB = 2;
                   const int     DiceUpB = int( double(3/11) * 100);

                   while(!RawEventQueue.empty())
                   {
                       int dice_roll =  m_distribution(m_generator);
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C" && 
                               ( (RawEvent.v1 >=4 )  ||  (RawEvent.v1 ==2 && dice_roll <= DiceUpB)))
                       {

                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;
           case 90:
               {
                   const int     AUpB = 1;
                   const int     BUpB = 1;
                   const int     DiceUpB = int( double(2/11) * 100);

                   while(!RawEventQueue.empty())
                   {
                       int dice_roll =  m_distribution(m_generator);
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C" && 
                               ( (RawEvent.v1 >=3 )  ||  (RawEvent.v1 ==2 && dice_roll <= DiceUpB)))
                       {

                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;
           default:
               return 0;
               break;
       }
      
   }

   uint64_t  SmPMS_min_combo_additional_InputShedding(double _ratio, volatile uint64_t & _eventCnt) 
   {
       uint64_t ShedCnt = 0;
       int ratio = int(_ratio * 100);
       NormalEvent RawEvent;

       switch (ratio)
       {
           case 10:
           case 20:
           case 30:
           case 40:
           case 50:
               {
                   const int   AUpB = 9;
                   const int   BUpB = 9;

                   while(!RawEventQueue.empty())
                   {
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;

           case 60:
               {
                   const int     AUpB = 9;
                   const int     BUpB = 9;

                   while(!RawEventQueue.empty())
                   {
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C" &&  (RawEvent.v1 == 3 || RawEvent.v1 ==4 ))
                       {

                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;
           case 70:
               {
                   const int     AUpB = 9;
                   const int     BUpB = 9;

                   while(!RawEventQueue.empty())
                   {
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C" &&  RawEvent.v1 <=6 ) 
                       {

                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;
           case 80:
               {
                   const int     AUpB = 9;
                   const int     BUpB = 9;
                   const int     DiceUpB = int( double(3/11) * 100);

                   while(!RawEventQueue.empty())
                   {
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C" && ( RawEvent.v1 <=6   ||  RawEvent.v1 ==10))
                       {

                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;
           case 90:
               {

                   while(!RawEventQueue.empty())
                   {
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 !=1 && RawEvent.v1 != 9) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 !=1 && RawEvent.v1 != 9)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C" && RawEvent.v1 !=10)
                       {

                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;
           default:
               return 0;
               break;
       }
   }

   uint64_t SmPMS_additional_InputShedding(double _ratio, volatile uint64_t & _eventCnt)
   {
       uint64_t ShedCnt = 0;
       int upAB = *m_PM_Booking.rbegin();
       NormalEvent RawEvent;
       while(!RawEventQueue.empty())
       {
           RawEvent = RawEventQueue.front();
           if(RawEvent.name == "A" && RawEvent.v1 >= upAB)
           {
               RawEventQueue.pop();
               ++ShedCnt;
               _eventCnt++;
           }
           else if(RawEvent.name == "B" && RawEvent.v1 >= upAB)
           {
               RawEventQueue.pop();
               ++ShedCnt;
               _eventCnt++;
           }
           else if(RawEvent.name == "C" && m_type_C_Booking[RawEvent.v1] == false) 
           {
               RawEventQueue.pop();
               ++ShedCnt;
               _eventCnt++;
           }
           else
           {
               processEvent(); 
               _eventCnt++;
           }
       }

       return ShedCnt;

   }

   uint64_t  SmPMS_max_combo_additional_InputShedding(double _ratio, volatile uint64_t & _eventCnt) 
   {
       uint64_t ShedCnt = 0;
       int ratio = int(_ratio * 100);
       NormalEvent RawEvent;

       switch (ratio)
       {
           case 10:
           case 20:
           case 30:
           case 40:
           case 50:
               {
                   const int   AUpB = 9;
                   const int   BUpB = 9;

                   while(!RawEventQueue.empty())
                   {
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;

           case 60:
               {
                   const int     AUpB = 9;
                   const int     BUpB = 9;

                   while(!RawEventQueue.empty())
                   {
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C" &&  RawEvent.v1 == 6)
                       {

                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;
           case 70:
               {
                   const int     AUpB = 8;
                   const int     BUpB = 8;

                   while(!RawEventQueue.empty())
                   {
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C") 
                       {
                           
                           switch(RawEvent.v1)
                           {
                               case 2:
                               case 6:
                               case 10:
                                   RawEventQueue.pop();
                                   ++ShedCnt;
                                   _eventCnt++;
                                   break;
                               default:
                                   processEvent();
                                   _eventCnt++;
                                   break;
                           }
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;
           case 80:
               {
                   const int     AUpB = 7;
                   const int     BUpB = 7;

                   while(!RawEventQueue.empty())
                   {
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > AUpB) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > BUpB)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C")
                       {
                           switch(RawEvent.v1)
                           {
                               case 2:
                               case 3:
                               case 6:
                               case 9:
                               case 10:
                                   RawEventQueue.pop();
                                   ++ShedCnt;
                                   _eventCnt++;
                                   break;
                               default:
                                   processEvent();
                                   _eventCnt++;
                                   break;

                           }
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;
           case 90:
               {

                   while(!RawEventQueue.empty())
                   {
                       RawEvent = RawEventQueue.front(); 

                       if(RawEvent.name == "A" && RawEvent.v1 > 4) 
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "B" && RawEvent.v1 > 4)
                       {
                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else if(RawEvent.name == "C" && RawEvent.v1 >=6)
                       {

                               RawEventQueue.pop();
                               ++ShedCnt;
                               _eventCnt++;
                       }
                       else
                       {
                           processEvent();
                           _eventCnt++;
                       }
                   }
                   return ShedCnt;
               }
               break;
           default:
               return 0;
               break;
       }
   }
   
   uint64_t  selectivity_InputShedding(double ratio, volatile uint64_t & _eventCnt) 
   {
       uint64_t ShedCnt = 0;

       const int dice_UP = ratio*100;

       NormalEvent RawEvent;
       while(!RawEventQueue.empty())
       {
           RawEvent = RawEventQueue.front(); 

            int dice_roll2 =  m_distribution(m_generator);

               if(RawEvent.name == "A" && dice_roll2 <= dice_UP+2) 
               {
                   RawEventQueue.pop();
                   ++ShedCnt;
                   _eventCnt++;
               }
               else if(RawEvent.name == "B" && dice_roll2 <= dice_UP+2)
               {
                   RawEventQueue.pop();
                   ++ShedCnt;
                   _eventCnt++;
               }
               else if(RawEvent.name == "C" && dice_roll2 <= dice_UP-2)
               {
                   RawEventQueue.pop();
                   ++ShedCnt;
                   _eventCnt++;
               }
               else
               {
                   processEvent();
                   _eventCnt++;
               }
       }

       return ShedCnt;
   }

   uint64_t ICDT_14_InputShedding(int _quota, uint64_t & _eventCnt)
   {
       int ToShedCnt = _quota;
       NormalEvent RawEvent;
       while(ToShedCnt> 0 && !RawEventQueue.empty())
       {
           int dice_roll = m_distribution(m_generator);

           RawEvent = RawEventQueue.front(); 
           if(RawEvent.name == "A" && dice_roll <= 90)
           {
               RawEventQueue.pop();
               --ToShedCnt;
           }
           else if(RawEvent.name == "B" && dice_roll > 90 && dice_roll <= 95)
           {
               RawEventQueue.pop();
               --ToShedCnt;
           }
           else if(RawEvent.name == "C" && dice_roll > 95)
           {
               RawEventQueue.pop();
               --ToShedCnt;
           }
           else
           {
               processEvent();
               _eventCnt++;
           }
       }

       return _quota - ToShedCnt;
   }

   uint64_t cost_based_InputShedding(int _quota, uint64_t & _eventCnt)
   {
       int ToShedCnt = _quota;
       NormalEvent RawEvent;
       while(ToShedCnt> 0 && !RawEventQueue.empty())
       {
           RawEvent = RawEventQueue.front();
           int dice_roll = m_distribution(m_generator);
           if(RawEvent.name == "A" && ( (RawEvent.v1 <38 || RawEvent.v1 > 50) || ( ((RawEvent.v1 < 42 && RawEvent.v1 > 37) || RawEvent.v1==50) && dice_roll >= 50 ) ) )
           {
               RawEventQueue.pop();
               --ToShedCnt;
           }
           else if(RawEvent.name == "B" && ( RawEvent.v2 <= 33 || RawEvent.v2 > 53))
           {
               RawEventQueue.pop();
               --ToShedCnt;
           }
           else 
           {
               processEvent();
               _eventCnt++;
           }
       }

       return _quota - ToShedCnt;
       
   }

	void update_miner()
	{
		if (!m_Miner)
			return;

		string eqlFilename = m_MiningPrefix + ".eql";
		string scriptFilename = m_MiningPrefix + ".sh";

		ofstream script(scriptFilename, ofstream::out | ofstream::trunc);

		QueryLoader dst = m_Definition;
		for (size_t i = 0; i < m_Query->events.size() - 1; ++i)
		{
			m_Miner->printResult(i * 2, i * 2 + 1);

			auto options = m_Miner->generateResult(i * 2, i * 2 + 1);
			for (auto& it : options)
			{
				Query q = m_Miner->buildPredicateQuery(dst, *m_Query, (uint32_t)i, it);

				ostringstream namestr;
				namestr << q.name << "_mined_" << i << '_' << it.eventId;
				for (size_t i = 0; i < it.numAttr; ++i)
					namestr << '_' << it.attrIdx[i];

				q.name = namestr.str();

				script << "$CEP_CMD -c " << eqlFilename << " -q " << q.name << " < $CEP_IN > ${CEP_OUT}" << q.name << " &" << endl;
				dst.addQuery(move(q));
			}
		}

		dst.storeFile(eqlFilename.c_str());
	}

    bool readEventStreamFromFiles(string file, int eventCnt, bool _override)
    {
        ifstream ifs;
        ifs.open(file.c_str());
        if( !ifs.is_open())
        {
            cout << "can't open file " << file << endl;
            return false;
        }

        string line;

        int cnt = 0;

        while(getline(ifs,line))
        {
            if(cnt++ > eventCnt)
                break;
            vector<string> dataEvent;
            stringstream lineStream(line);
            string cell;

            while(getline(lineStream,cell,','))
                dataEvent.push_back(cell);

            NormalEvent RawEvent;
            RawEvent.name = dataEvent[0];
            RawEvent.ArrivalQTime = stoi(dataEvent[1]);
            RawEvent.ID = stoi(dataEvent[2]); 
            RawEvent.v1 = stoull(dataEvent[3]);
            RawEvent.v2 = stoi(dataEvent[4]);
            RawEvent.v2 = 1;

            RawEventQueue.push(RawEvent);
        }

        return true;

    }

    void dumpLatencyBooking(string file)
    {
        ofstream outFile;
        outFile.open(file.c_str());

        for(auto iter : m_Latency_booking)
            outFile << iter.first << "," << iter.second <<  endl;
    }

	void write_event(bool _timeout, uint32_t _state, const attr_t* _attributes)
	{
		StreamEvent r;
		r.typeIndex = m_ResultEventType;
		r.typeHash = m_ResultEventTypeHash;
		r.attributeCount = m_ResultAttributeCount;
		r.flags = 0;

		if (_timeout)
		{
			r.flags = StreamEvent::F_TIMEOUT;
			r.timeoutState = (uint8_t)(_state - 1);
		}

		if (m_AppendTimestamp)
		{
			r.flags |= StreamEvent::F_TIMESTAMP;
			r.attributes[r.attributeCount++] = current_utime();
		}

		uint64_t* outattr_it = r.attributes;
		for (auto it : m_OutEventAttrSrc)
        {
			*outattr_it++ = _attributes[it];
            //cout << *outattr_it << " " << flush;
        }
        
        cout << endl;

        uint64_t la = _attributes[Query::DA_FULL_MATCH_TIME] - _attributes[Query::DA_CURRENT_TIME]; 
        m_RealTimeLatency = la;
        m_Latency_booking[la]++;
        ACCLantency += la;

            if(la > LATENCY)
                ++NumHighLatency;

		if (m_Miner)
		{
			if (_timeout)
			{
				size_t idx = (_state - 1) * 2 + 1;
				uint32_t eventA = (uint32_t)r.attributes[_state];
				uint32_t eventB = (uint32_t)m_DefAttrId;

				m_Miner->addMatch(eventA, eventB, idx);
			}
			else
			{
				for (uint32_t s = 1; s < _state; ++s)
				{
					size_t idx = (s - 1) * 2;
					uint32_t eventA = (uint32_t)r.attributes[s];
					uint32_t eventB = (uint32_t)r.attributes[s + 1];

					m_Miner->addMatch(eventA, eventB + 1, idx);
				}
			}
		}
	}

	QueryLoader			m_Definition;
	const Query*		m_Query;

	unique_ptr<PredicateMiner> m_Miner;
	string				m_MiningPrefix;
	PatternMatcher		m_Matcher;
	vector<uint32_t>	m_OutEventAttrSrc;

    bool                m_RandomInputSheddingFlag;
    bool                m_InputSeddingSwitcher;
    bool                m_PMSheddingSwitcher;
    
    bool                m_Monitoring_Latency = false;

	uint16_t			m_ResultEventType;
	uint32_t			m_ResultEventTypeHash;
	uint8_t				m_ResultAttributeCount;
	bool				m_GenerateTimeoutEvents;
	bool				m_AppendTimestamp;

	uint64_t			m_DefAttrId;
	uint64_t			m_DefAttrOffset;

    uint64_t            m_SheddingCnt = 0;

	uint64_t			m_NextMinerUpdateTime;
    uint64_t            lastSheddingTime = 0;

    uint64_t            time;
    uint64_t            latency;
    uint64_t            acctime;
    uint64_t            Rtime;
    uint64_t            cntFullMatch;
    uint64_t            m_RealTimeLatency = 0;

    queue<NormalEvent>&  RawEventQueue;
    
    map<int,uint64_t>  m_Latency_booking;
    queue<int>  m_Latency_booking_Q;
    set<int> m_PM_Booking;
    bool m_type_C_Booking[21];
    int loadCnt;
};

int max(int a, int b) { return (a > b) ? a : b; }

void knapSackSolver(int W, int wt[], int val[], int name[], int n, set<int> &keepingPMSet)
{   
    int i, w;  
    int K[n + 1][W + 1]; 
    for(int i=0; i<=n; ++i)
        for(int j=W; j<=W; ++j)
            K[i][j] = 6000;

    for (i = 0; i <= n; i++) {
        for (w = 0; w <= W; w++) {
            if (i == 0 || w == 0)
                K[i][w] = 0;
            else if (wt[i - 1] <= w)
                K[i][w] = max(val[i - 1] +
                        K[i - 1][w - wt[i - 1]], K[i - 1][w]);
            else 
                K[i][w] = K[i - 1][w];
        }
    }

    int res = K[n][W];

    w = W;
    for (i = n; i > 0 && res > 0; i--) {

        if (res == K[i - 1][w])
            continue;        
        else { 

            keepingPMSet.insert(name[i-1]);

            res = res - val[i - 1]; 
            w = w - wt[i - 1]; 
        }
    } 
}                                     

int main(int _argc, char* _argv[])
{
	init_utime();

	const char* deffile = "default.eql";
	const char* queryName = 0;
	const char* monitorFile = 0;
	const char* miningPrefix = 0;
	bool captureTimeouts = false;
	bool appendTimestamp = false;
    bool InputShedding = false;
    bool PMShedding = false;

    bool MonitoringCons_Contr_Flag = false; 

    bool Flag_ClusteringPMShedding = false;
    bool Flag_RandomPMShedding = false;
    bool GeneratePMsFlag = false;

    bool Flag_selectivityPMShedding = false;

    bool Flag_inputShedVLDB_03 = false;
    bool Flag_inputShedICDT_14 = false;
    bool Flag_inputShedRandom = false;
    bool inputShed_cost_based = false;

    bool DropIrrelevantOnly = false;

    string HOMEDIR = getenv("HOME"); 

    string suffix = "none";
    string streamFile = HOMEDIR + "/CEP_load_shedding/src_PM_Distribution_test/NormalEventStreamGen/Uniform_StreamLog_prefetch_500K.csv";
    double sheddingRatio = 0;
    int PMABUpperBound = 10;
    int eventCnt = 100000;

    int TTL = 1;

    int num_fetchWoker;
    uint64_t cacheSize;

	int c;
	while ((c = _free_getopt(_argc, _argv, "F:c:q:p:m:n:r:D:T:f:C:Z:X:Y:L:u:tsIPRGMVOABaohzg")) != -1)
	{
		switch (c)
		{
            case 'F':
                streamFile = string(_free_optarg); 
                break;
            case 'c':
                deffile = _free_optarg;
                break;
            case 'q':
                queryName = _free_optarg;
                break;
            case 'p':
                monitorFile = _free_optarg;
                break;
            case 'm':
                miningPrefix = _free_optarg;
                break;
            case 'n':
                suffix = string(_free_optarg);
                break;
            case 'r':
                sheddingRatio = stod(string(_free_optarg));
                break;
            case 'D':
                eventCnt = PMABUpperBound = stoi(string(_free_optarg));
                break;
            case 'T':
                TTL = stoi(string(_free_optarg));
                break;
            case 'f':
                num_fetchWoker = stoi(string(_free_optarg));
                break;
            case 'C':
                cacheSize = stoull(string(_free_optarg));
                break;
            case 'Z':
                PatternMatcher::Transition::fetchProbability = stoi(string(_free_optarg)); 
                break;
            case 'X':
                PatternMatcher::Transition::noiseProbability = stoi(string(_free_optarg)); 
                break;
            case 'Y':
                PatternMatcher::Transition::relaxProbability = stoi(string(_free_optarg)); 
                break;
            case 'g':
                PatternMatcher::Transition::FLAG_greedy = true;
                break;
            case 'L':
                PatternMatcher::Transition::fetch_latency = stoi(string(_free_optarg));
                FetchWorker::fetch_latency = stoi(string(_free_optarg));
                break;
            case 'u':
                PatternMatcher::Transition::prefetchFrequency = stoi(string(_free_optarg));
                break;
            case 't':
                captureTimeouts = true;
                break;
            case 's':
                appendTimestamp = true;
                break;
            case 'I':
                InputShedding = true;
                break;
            case 'P':
                Flag_ClusteringPMShedding = true;
                break;
            case 'R':
                Flag_RandomPMShedding = true;
                break;
            case 'G':
                GeneratePMsFlag = true;
                break;
            case 'M':
                MonitoringCons_Contr_Flag = true;
                break;
            case 'V':
                Flag_selectivityPMShedding = true;
                break;
            case 'z':
                Flag_inputShedVLDB_03 = true;
                break;
            case 'h':
                Flag_inputShedICDT_14= true;
                break;
            case 'a':
                Flag_inputShedRandom = true;
                break;
            case 'o':
                inputShed_cost_based = true;
                break;
            case 'O':
                DropIrrelevantOnly = true;
                break;
            case 'A':
                PatternMatcher::Transition::FLAG_prefetch = true;
                break;
            case 'B':
                PatternMatcher::Transition::FLAG_delay_fetch = true;
                break;

            default:
                abort();
        }
    }

    int ABUpperBound = 10-(sheddingRatio * 10);

    int CLowerBound = sheddingRatio*10;

    queue<NormalEvent> EventQueue;

    FetchWorker* fetchWorker = new FetchWorker[num_fetchWoker];
    Cache cache(cacheSize, fetchWorker, num_fetchWoker);
    unordered_map<uint64_t, uint64_t> UtilityMap;

    cache.setUtilityMap(&UtilityMap);

	CepMatch prog(EventQueue);
	if (!prog.init(deffile, queryName, miningPrefix, captureTimeouts, appendTimestamp))
		return 1;

    prog.readEventStreamFromFiles(streamFile, eventCnt, false); 
    prog.m_Matcher.setTTL(TTL);
    prog.m_Matcher.setTimeSliceSpan(prog.m_Query->within);

    for(auto && s : prog.m_Matcher.m_States)
    {
        s.setCache(&cache, num_fetchWoker);
        s.setUtilityMap(&UtilityMap);
    }

    cache.start();
    for(int i=0; i<num_fetchWoker; ++i)
        fetchWorker[i].start();

    prog.m_Matcher.m_States[0].stateBufferCount = 0;
    prog.m_Matcher.m_States[0].setTimesliceClusterAttributeCount(1,1,1);
    prog.m_Matcher.m_States[0].count[0][0] = 1;

    prog.m_Matcher.m_States[1].addClusterAttrIdx(1);
    prog.m_Matcher.m_States[2].addClusterAttrIdx(1);
    prog.m_Matcher.m_States[2].addClusterAttrIdx(4);

    if(queryName == string("P5") or queryName == string("P6"))
    {
        cout << queryName << endl;
    }

    if(queryName == string("P5"))
    {

        if(PatternMatcher::Transition::FLAG_prefetch)
        {

            prog.m_Matcher.m_States[1].setExternalIndex(1);
            prog.m_Matcher.m_States[2].setExternalIndex(1);
        }

        prog.m_Matcher.m_States[3].setExternalIndex(1);
        prog.m_Matcher.m_States[3].setExternalComIndex(5);
        prog.m_Matcher.m_States[3].setExternalFlagIndex(2);

        if(PatternMatcher::Transition::FLAG_delay_fetch)
        {
            prog.m_Matcher.m_States[4].setExternalIndex(1);
            prog.m_Matcher.m_States[4].setExternalComIndex(5);
            prog.m_Matcher.m_States[4].setExternalFlagIndex(2);
        }
    }
    
    else if(queryName == string("P6"))
    {

        if(PatternMatcher::Transition::FLAG_prefetch)
        {

            prog.m_Matcher.m_States[1].setExternalIndex(1);
        }
        prog.m_Matcher.m_States[2].setExternalIndex(1);
        prog.m_Matcher.m_States[2].setExternalComIndex(4);
        prog.m_Matcher.m_States[2].setExternalFlagIndex(2);

        if(PatternMatcher::Transition::FLAG_delay_fetch)
        {
            prog.m_Matcher.m_States[3].setExternalIndex(1);
            prog.m_Matcher.m_States[3].setExternalComIndex(4);
            prog.m_Matcher.m_States[3].setExternalFlagIndex(2);
        }
    }

    else {

        if(PatternMatcher::Transition::FLAG_prefetch)
        {

            prog.m_Matcher.m_States[1].setExternalIndex(1);
            prog.m_Matcher.m_States[2].setExternalIndex(1);
            prog.m_Matcher.m_States[3].setExternalIndex(1);
        }

        prog.m_Matcher.m_States[4].setExternalIndex(1);
        prog.m_Matcher.m_States[4].setExternalComIndex(5);
        prog.m_Matcher.m_States[4].setExternalFlagIndex(2);

        if(PatternMatcher::Transition::FLAG_delay_fetch)
        {
            prog.m_Matcher.m_States[5].setExternalIndex(1);
            prog.m_Matcher.m_States[5].setExternalComIndex(5);
            prog.m_Matcher.m_States[5].setExternalFlagIndex(2);

            prog.m_Matcher.m_States[6].setExternalIndex(1);
            prog.m_Matcher.m_States[6].setExternalComIndex(5);
            prog.m_Matcher.m_States[6].setExternalFlagIndex(2);

            prog.m_Matcher.m_States[7].setExternalIndex(1);
            prog.m_Matcher.m_States[7].setExternalComIndex(5);
            prog.m_Matcher.m_States[7].setExternalFlagIndex(2);

            prog.m_Matcher.m_States[8].setExternalIndex(1);
            prog.m_Matcher.m_States[8].setExternalComIndex(5);
            prog.m_Matcher.m_States[8].setExternalFlagIndex(2);
        }
    }

    prog.m_Matcher.m_States[1].addKeyAttrIdx(1);
    prog.m_Matcher.m_States[2].addKeyAttrIdx(1);
    prog.m_Matcher.m_States[2].addKeyAttrIdx(4);

    prog.m_Matcher.addClusterTag(1,0,0,234,104607);
    prog.m_Matcher.addClusterTag(1,0,1,3.3333333333,4046.625);
    prog.m_Matcher.addClusterTag(1,0,2,432,157341.333333);
    prog.m_Matcher.addClusterTag(1,0,3,92.6666666667,48944.5);
    
    prog.m_Matcher.addClusterTag(2,0,0,7.685873606,413.494423792);
    prog.m_Matcher.addClusterTag(2,0,1,1.9649122807,10344.7017544);
    prog.m_Matcher.addClusterTag(2,0,2,0,17537.0285714);
    prog.m_Matcher.addClusterTag(2,0,3,1.7356321839,4548.56321839);

    prog.m_Matcher.sortClusterTag();

    /*
     * manually set predicates a.v1+b.v1 == d.v1
     */

    if(InputShedding)
    {
        cout << "Inpute shedding on " << endl;
        prog.InputSheddingOn();
    }
    else
        prog.InputSheddingOff();

    if(PMShedding)
    {
        cout << "PM Shedding on " << endl;
        prog.PMSheddingOn();
        PatternMatcher::setMonitoringLoadOn();
    }
    else
    {
        PatternMatcher::setMonitoringLoadOff();
        prog.PMSheddingOff();
    }

	volatile uint64_t eventCounter = 0;

    set<int> PMBook;
    bool C_keep_book[21];
    for(int i=0; i<21; ++i)
        C_keep_book[21] = false;

	MonitorThread monitor;
	monitor.addValue(&current_utime);        
	monitor.addValue(&eventCounter, true);   
    
	if (monitorFile)
		monitor.start(monitorFile);

    if(MonitoringCons_Contr_Flag)
        PatternMatcher::setMonitoringLoadOn();
    else
        PatternMatcher::setMonitoringLoadOff();

    /************** PM shedding *********************/

    if(Flag_ClusteringPMShedding)
    {
        prog.m_Matcher.clustering_classification_PM_shedding_semantic_setPMSheddingCombo(2);

        int val[19];  
        int wt[19];   
        int name[19]; 
        
        for(int i=0; i<19; ++i)
            name[i] = i+2;

        for(int i=0; i<19; ++i)
        {
            int pc = i+2;
            if(pc <= PMABUpperBound)
                val[i] = pc - 1;
            if(pc > PMABUpperBound)
                val[i] = 1;   

            if(pc <= 11)
                wt[i] = pc -1;
            if(pc > 11)
                wt[i] = 20-pc+1;
        }

        int n = sizeof(val) / sizeof(val[0]); 

        int RatioToKeep = 100 - sheddingRatio*100;

        knapSackSolver(RatioToKeep, wt, val, name, n, prog.m_PM_Booking);

        for(auto &a : prog.m_type_C_Booking)
            a = false;

        for(auto PMKey : prog.m_PM_Booking)
        {
            prog.m_Matcher.m_States[2].PMKeepingBook[PMKey] = true;
            prog.m_type_C_Booking[PMKey] = true;
        }

        cout << "Keeping ratio " << RatioToKeep << endl;

        for(int i=0; i<21; ++i)
            cout << prog.m_type_C_Booking[i] << "---" << flush;
       cout << endl;
    }

    if(DropIrrelevantOnly)
    {
        prog.m_Matcher.clustering_classification_PM_shedding_semantic_setPMSheddingCombo(2);
        
        for(int PMKey =2; PMKey<= PMABUpperBound; ++PMKey)
        {
            prog.m_PM_Booking.insert(PMKey);
            prog.m_Matcher.m_States[2].PMKeepingBook[PMKey] = true;
            prog.m_type_C_Booking[PMKey] = true;
        }
    }

    if(Flag_RandomPMShedding)
    {
        prog.m_Matcher.clustering_classification_PM_shedding_random(1,sheddingRatio);
        prog.m_Matcher.clustering_classification_PM_shedding_random(2,sheddingRatio);
    }

    if(Flag_selectivityPMShedding)
        prog.m_Matcher.clustering_classification_PM_shedding_selectivity(sheddingRatio);
    /************************************************/

    cout << "CEP engine working..." << endl;

    uint64_t sheddingCnt = 0;
    uint64_t inputSheddingCnt = 0;

    uint64_t eventCntFlag = eventCounter + 20000;

    time_point<high_resolution_clock> begin_time_point = high_resolution_clock::now();
     
	while(prog.processEvent())
    {
        ++eventCounter;
    }

    time_point<high_resolution_clock> end_time_point = high_resolution_clock::now();

    cout << "NumFullMatch " << NumFullMatch << endl;
    cout << "work done" << endl;
        
	prog.update_miner();

    string _file = string("latency_")+suffix+".csv";
    prog.dumpLatencyBooking(_file);

    string result_file = string("results_")+suffix+".txt";

    ofstream outResultFile;
    outResultFile.open(result_file.c_str());

    outResultFile << "perform, " << prog.loadCnt << "loadshedding" << endl;
    outResultFile << "eventCn, " << eventCounter << endl;
    outResultFile << "#FM, " << NumFullMatch << endl;
    outResultFile << "#PM, " << NumPartialMatch << endl;

    outResultFile << "#high-latecny, "  << NumHighLatency<< endl;
    if(NumFullMatch)
        outResultFile << "avg-latency, " << (double) (ACCLantency/NumFullMatch) << endl; 
    outResultFile << "exe-time-in-ms, " << duration_cast<milliseconds>(end_time_point - begin_time_point).count() << endl;
    outResultFile << "#cache-update, " << cache.KeyUpdateCnt << endl;
    outResultFile << "#cache-heap-update, " << cache.HeapUpdateCnt << endl;
    outResultFile << "#cache-hit, " << cache.cacheHit << endl;
    outResultFile << "#cache-miss, " << cache.cacheMiss << endl;
    outResultFile << "#block-fetch, " << cache.BlockFectchCnt << endl;

    outResultFile << "#Pcache, " << cache.FetchCached[1] << endl;
    outResultFile << "#Dcache, " << cache.FetchCached[2] << endl;
    outResultFile << "#Bcache, " << cache.FetchCached[3] << endl;

    outResultFile << "#Phit, " << cache.FetchHit[1] << endl;
    outResultFile << "#Dhit, " << cache.FetchHit[2] << endl;
    outResultFile << "#Bhit, " << cache.FetchHit[3] << endl;

    outResultFile << "#PNhit, " << cache.FetchNeverUse[1] << endl;
    outResultFile << "#DNhit, " << cache.FetchNeverUse[2] << endl;

    cout << "fetcher, cache worker done " << endl;

    exit(0);
    cache.stop();

    for(int i=0; i<num_fetchWoker; ++i) 
    {    
        fetchWorker[i].stop();
        fetchWorker[i].m_Thread.detach();
    }    
    cache.m_Thread.detach(); 

    delete [] fetchWorker;
	return 0;
}
