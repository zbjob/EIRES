#include <string>

class Weather {
    public:
        string weatherFile = "../bin/weather/Discovered_Events_V3New_Weather.csv";
        double fetchHumidity();
        double fetchTemperature();
        double featchPressure();

}