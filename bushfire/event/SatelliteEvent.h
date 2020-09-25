#pragma once

#include <string>
// #include "PatternMatcher.h"
#include <iostream>
#include <sstream>


enum Threshold
{
    LOW,
    NORMAL,
    HIGH
};

enum LandCover
{
    FOREST,
    GRASSLAND,
    CROPSLAND
};

enum ChannelID{
    C00,
    C01,
    C02,
    C06,
    C07_MINUS_C14,
    C14,
    C16,
    C07
};

class SatelliteEvent
{

public:
    SatelliteEvent() {}

    // attr_t eventID;
    // std::string name;

    // std::string day;
    attr_t time;

    attr_t channelID;
    attr_t level;
    // attr_t landcover;
    // double temperature;
    std::string boundary;

    double temperature = 0;
    double humidity = 0;
    double pressure = 0;

    void setLevel(string level_)
    {
        if (level_ == "high")
            level = Threshold::HIGH;
        else if (level_ == "normal")
            level = Threshold::NORMAL;
        else if (level_ == "low")
            level = Threshold::LOW;
        else
            level = Threshold::LOW;
    }

    // void setLandcover(string landcover_)
    // {
    //     if(landcover_ == "forest")
    //         landcover = LandCover::FOREST;
    //     else if (landcover_ == "grassland")
    //         landcover = LandCover::GRASSLAND;
    //     else if (landcover_ == "cropsland")
    //         landcover = LandCover::CROPSLAND;
    //     else 
    //         landcover = LandCover::FOREST;
    // }

    // void setName(string name_){
    //     name  = name_;
    // }

    void setTime(string time_){
        // time = stoll(time_);
        std::istringstream iss(time_);
        iss >> time;
    }

    void setChannelID(string cid_){
        if(cid_ == "C01")
            channelID = ChannelID::C01;
        else if(cid_ == "C02")
            channelID = ChannelID::C02;
        else if(cid_ == "C06")
            channelID = ChannelID::C06;
        else if(cid_ == "C07_minus_C14")
            channelID = ChannelID::C07_MINUS_C14;
        else if(cid_ == "C14")
            channelID = ChannelID::C14;
        else if(cid_ == "C07")
            channelID = ChannelID::C07;
        else 
            channelID = ChannelID::C16;
    }

    void setTemperature(std::string temp_){
        // temperature = stod(temp_);
        std::istringstream iss(temp_);
        iss >> temperature;

    }

    void setHumidity(std::string hum_){
        std::istringstream iss(hum_);
        iss >> humidity;
    }

    void setPressure(std::string pres_){
        // pressure = stod(pres_);
        std::istringstream iss(pres_);
        iss >> pressure;

    }
    
    void setBoundary(std::string bound_){
        boundary = bound_;
    }

    void printEvent(){
        // cout << "=>>>Event name: " << name << endl;
        cout << "time: " << time << endl;
        cout << "channelID: " << channelID << endl;
        cout << "level: " << level << endl;
        // cout << "landcover: " << landcover << endl; 
        cout << "boundary: " << boundary << endl; 

    }
};
