#include <time.h>

#include "TimeStamp.h"

TimeStamp::TimeStamp(): sysSecond_(0) {}

TimeStamp::TimeStamp(int64_t sysSecond): sysSecond_(sysSecond) {}

TimeStamp TimeStamp::NowTime() {
    return TimeStamp(time(NULL));
}

std::string TimeStamp::TimetoString() const {
    char buff[128] = {0};
        struct tm *time = localtime(&sysSecond_);
        snprintf(buff, 128, "%4d/%02d/%02d %02d:%02d:%02d",
            time->tm_year + 1900,
            time->tm_mon + 1,
            time->tm_mday,
            time->tm_hour,
            time->tm_min,
            time->tm_sec);
    return buff;
}

#include <iostream>
int main() {
    std::cout << TimeStamp::NowTime().TimetoString() << std::endl;
    return 0;
}