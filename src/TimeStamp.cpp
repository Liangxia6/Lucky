#include "TimeStamp.h"

TimeStamp::TimeStamp() : sysSecond_(0)
{
}

TimeStamp::TimeStamp(int64_t sysSecond) : sysSecond_(sysSecond)
{
}

TimeStamp TimeStamp::getNowTime()
{
    // time_t time(time_t* arg);
    return TimeStamp(time(NULL));
}

std::string TimeStamp::toString() const
{
    char buff[128] = {0};
    // 将时间表示从time_t(时间戳int64)类型转换为本地时间的tm结构
    // struct tm {
    //      int tm_sec;   // 秒数，范围从 0 到 59
    //      int tm_min;   // 分钟数，范围从 0 到 59
    //      int tm_hour;  // 小时数，范围从 0 到 23
    //      int tm_mday;  // 一个月中的日期，范围从 1 到 31
    //      int tm_mon;   // 月份，范围从 0 到 11
    //      int tm_year;  // 年份，从 1900 开始
    //      int tm_wday;  // 星期几，范围从 0 到 6（0 表示周日）
    //      int tm_yday;  // 一年中的天数，范围从 0 到 365
    //      int tm_isdst; // 夏令时标识符，正数表示夏令时，0 表示不是夏令时，负数表示夏令时信息不可用
    //  };
    struct tm *time = localtime(&sysSecond_);
    // 将格式化的字符串写入缓冲区
    // int snprintf(char* buffer, size_t bufferSize, const char* format, ...);
    snprintf(buff, 128, "%3d/%02d/%02d %02d:%02d",
             time->tm_year - 100,
             time->tm_mon + 1,
             time->tm_mday,
             time->tm_hour,
             time->tm_min);

    return buff;
}
