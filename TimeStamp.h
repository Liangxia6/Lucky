#pragma once

#include <iostream>
#include <string>

class TimeStamp{
public:
    TimeStamp();
    TimeStamp(int64_t);
    static TimeStamp NowTime();
    std::string TimetoString() const;
    
private:
    int64_t sysSecond_;
};