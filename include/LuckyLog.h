#pragma once

#include <string>
#include <iostream>
#include <cstdarg>

#include "TimeStamp.h"

// 优化前
// LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFOM(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        LuckyLog &logger = LuckyLog::LogSingleton();      \
        logger.setLogLevel(INFOM);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.writeLog(buf);                             \
    } while (0)

#define LOG_ERROR(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        LuckyLog &logger = LuckyLog::LogSingleton();      \
        logger.setLogLevel(ERROR);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.writeLog(buf);                             \
    } while (0)

#define LOG_FATAL(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        LuckyLog &logger = LuckyLog::LogSingleton();      \
        logger.setLogLevel(FATAL);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.writeLog(buf);                             \
        exit(-1);                                         \
    } while (0)

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        LuckyLog &logger = LuckyLog::LogSingleton();      \
        logger.setLogLevel(DEBUG);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.writeLog(buf);                             \
    } while (0)
#else
#define LOG_DEBUG(logmsgFormat, ...)
#endif

// 日志级别
enum LogLevel
{
    INFOM, // 普通信息
    ERROR, // 错误信息
    FATAL, // 严重信息
    DEBUG, // 调试信息
};

/**
 * @brief 日志类
 *
 * 使用函数模板代替大量define, 使用数组配合枚举代替switch
 *
 */
class LuckyLog
{
public:
    // 获取日志唯一的实例对象 单例
    static LuckyLog &LogSingleton();
    // 设置日志级别
    void setLogLevel(int);
    // 写日志
    void writeLog(std::string);

private:
    static const char *headers[4];

    LuckyLog() = default;
    LuckyLog(const LuckyLog &) = delete;
    LuckyLog &operator=(const LuckyLog &) = delete;
    int loglevel_;
};

// // 优化后 (使用函数模板)
// template <typename... Args>
// void logHelper(const char *, LuckyLog *, Args...);

// template <LogLevel, typename... Args>
// void log(const char *, Args...);
// //辅助函数
// template <typename... Args>
// void logHelper(const char* format, LuckyLog *luckylog, Args... args)
// {
//     char buff[1024] = {0};
//     snprintf(buff, 1024, format, args...);
//     luckylog->writeLog(buff);
// }

// //模板函数,使用方法LOG_INFOM(massage);
// template <LogLevel level, typename... Args>
// void log(const char* format, Args... args)
// {
//     //创建单例
//     LuckyLog luckylog = LuckyLog::LogSingleton();
//     luckylog.setLogLevel(level);

//     logHelper(format, &luckylog, args...);

//     if (level == LogLevel::FATAL)
//     {
//         exit(-1);
//     }
// }
