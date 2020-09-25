#include "Weather.h"

bool readWeatherFromFiles(string file, bool header = true)
{
    ifstream ifs;
    ifs.open(file.c_str());
    if (!ifs.is_open())
    {
        cout << "can't open file " << file << endl;
        return false;
    }

    string line;
    if (header)
        getline(ifs, line);
    while (std::getline(ifs, line))
    {
        vector<string> dataEvent;
        stringstream lineStream(line);
        string cell;
        string polygon_;

        while (getline(lineStream, cell, ','))
        {
                dataEvent.push_back(cell);
        }

        RawEventQueue.push(RawEvent);
    }

    return true;
}

double Weather::fetchTemperature(int32_t time)

{
}