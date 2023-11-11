#pragma once

#include <string>

void logMessage(LogLevel level, const char* format, ...);

#define LOG_INFOM(format, ...) logMessage(INFOM, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) logMessage(ERROR, format, ##__VA_ARGS__)
#define LOG_FATAL(format, ...) logMessage(FATAL, format, ##__VA_ARGS__)

#ifdef MUDEBUG
#define LOG_DEBUG(format, ...) logMessage(DEBUG, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...)
#endif

enum LogLevel{
    INFOM,
    ERROR,
    FATAL,
    DEBUG,
};

template <LogLevel loglevel>
LogLevel LOG_log(){

}

class LuckyLog
{
public:
    // 获取日志唯一的实例对象 单例
    static LuckyLog &LogInstance();
    // 设置日志级别
    void setLogLevel(int);
    // 写日志
    void LogWritter(std::string);

private:
    LuckyLog() = default;
    int loglevel_;
};