#pragma once
#include <string>
#include "PatternMatcher.h"


class NormalEvent
{
    public:
        NormalEvent(std::string N): name(N){}
        NormalEvent(){}
        ~NormalEvent(){}
       
        //attr_t attributes[EventDecl::MAX_ATTRIBUTES];
        attr_t  v1;
        attr_t  v2;
        attr_t  ArrivalQTime;
        attr_t  ID;

        std::string name;
};
