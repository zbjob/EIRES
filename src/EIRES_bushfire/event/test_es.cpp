#include "EventStream.h"
#include <iostream>
#include <queue>
#include "SatelliteEvent.h"
#include <fstream> // std::ifstream

using namespace std;

queue<SatelliteEvent> RawEventQueue;

bool readEventStreamFromFiles(string file, bool header = true)
{
    ifstream ifs;
    ifs.open(file.c_str());
    if (!ifs.is_open())
    {
        cout << "can't open file " << file << endl;
        return false;
    }

    string line;
    if(header)
        getline(ifs,line);

    while (getline(ifs, line))
    {
        vector<string> dataEvent;
        stringstream lineStream(line);
        string cell;
        string polygon_;

        while (getline(lineStream, cell, ','))
        {
            // dataEvent.push_back(cell);
            if(cell.find("POLYGON") != string::npos){
                // cell = cell.erase(8,1);
                // polygon_ += cell + ",";
                polygon_ = cell + ",";
                // polygon_ += cell.substr(0,cell.rfind(" ")) + "," + cell.substr((size_t)cell.rfind(" "),(size_t)cell.length() - (size_t)cell.rfind(" "));
                // cout << polygon_ << endl;
                while(getline(lineStream,cell,',')){
                    polygon_ += cell.erase(0,1);
                    if(cell.find("))") != string::npos)
                        break;
                    else 
                        polygon_ += ",";
                }
                // cout << "Huy" << endl;
                polygon_ = polygon_.erase(0,1);
                polygon_ = polygon_.erase(polygon_.length()-1,1);
                // cout << polygon_ << endl;
                // exit(0);


                dataEvent.push_back(polygon_);
            } else {
                dataEvent.push_back(cell);
            }
        }

        if (dataEvent[4] == "NULL" || dataEvent[8] == "NULL")
            continue;

        SatelliteEvent RawEvent;

        RawEvent.setName(dataEvent[0]);
        RawEvent.setTime(dataEvent[1]);
        RawEvent.setChannelID(dataEvent[2]);
        RawEvent.setLevel(dataEvent[3]);
        RawEvent.setLandcover(dataEvent[4]);
        // cout << "event 5 : " << dataEvent[5] << endl; 
        RawEvent.setBoundary(dataEvent[5]);
        RawEvent.setTemperature(stod(dataEvent[6]));

        RawEventQueue.push(RawEvent);
    }

    return true;
}
int main()
{
    // attr_e e1 = attr_e((uint64_t)15);
    // uint64_t r = e1.getValue();
    // cout << "ok reuslt : " << r << endl;
    string streamFile = "../datasets/Stream_SatelliteEvents_Polygon.csv";
    readEventStreamFromFiles(streamFile);
    SatelliteEvent RawEvent;
    int error = 0;
    while (!RawEventQueue.empty())
    {
        if (!RawEventQueue.empty())
        {
            RawEvent = RawEventQueue.front();
            RawEventQueue.pop();
            // RawEvent.printEvent();
            // cout << "name: " << RawEvent.name << " time: " << RawEvent.time << " channelId: " << RawEvent.channelID << endl;
        }
        else
        {
            cout << "event stream queue is empty now!" << endl;
            return false;
        }

        StreamEvent event;
        event.attributes[0] = attr_e(RawEvent.time);
        event.attributes[1] = attr_e(RawEvent.channelID);
        event.attributes[2] = attr_e(RawEvent.level);
        event.attributes[3] = attr_e(RawEvent.landcover);
        event.attributes[4] = attr_e(RawEvent.boundary);

        // cout << "Stream event " << endl;
        // for(int i=0;i<4;i++){
        //     cout << "attr " << i << " : "  << endl;
        // }
    
        
        // uint64_t t = 86400000000;
        // uint64_t ts= 300000000;
        // const uint64_t one_min = 60*1000*1000;


		// event.attributes[Query::DA_ZERO] = 0;
		// event.attributes[Query::DA_MAX] = numeric_limits<attr_t>::max();
		// event.attributes[Query::DA_CURRENT_TIME] = current_utime();
		// event.attributes[Query::DA_OFFSET] = m_DefAttrOffset;
		// event.attributes[Query::DA_ID] = m_DefAttrId;
    }
}