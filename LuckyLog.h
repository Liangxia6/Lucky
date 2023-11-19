#pragma once

#include <string>
#include <iostream>
#include <cstdarg>

#include "TimeStamp.h"

//日志级别
enum LogLevel
{
    INFOM,  //普通信息
    ERROR,  //错误信息
    FATAL,  //严重信息
    DEBUG,  //调试信息
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

    const char* headers[4] = {
    "[INFOM]",
    "[ERROR]",
    "[FATAL]",
    "[DEBUG]"
    };

    LuckyLog() = default;
    int loglevel_;

};

// 优化后 (使用函数模板)
template <typename... Args>
void logHelper(const char *, LuckyLog *, Args...);

template <LogLevel, typename... Args>
void log(const char *, Args...);
// //辅助函数
// template <typename... Args>
// void logHelper(const char* format, LuckyLog *luckylog, Args... args) 
// {
//     char buff[1024] = {0};
//     snprintf(buff, 1024, format, args...);
//     luckylog->writeLog(buff);
// }

// //模板函数,使用方法log<INFOM>(massage);
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

// 优化前
// LOG_INFO("%s %d", arg1, arg2)
// #define LOG_INFO(logmsgFormat, ...)                       \
//     do                                                    \
//     {                                                     \
//         Logger &logger = Logger::instance();              \
//         logger.setLogLevel(INFO);                         \
//         char buf[1024] = {0};                             \
//         snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
//         logger.log(buf);                                  \
//     } while (0)

// #define log<ERROR>(logmsgFormat, ...)                      \
//     do                                                    \
//     {                                                     \
//         Logger &logger = Logger::instance();              \
//         logger.setLogLevel(ERROR);                        \
//         char buf[1024] = {0};                             \
//         snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
//         logger.log(buf);                                  \
//     } while (0)

// #define (logmsgFormat, ...)                      \
//     do                                                    \
//     {                                                     \
//         Logger &logger = Logger::instance();              \
//         logger.setLogLevel(FATAL);                        \
//         char buf[1024] = {0};                             \
//         snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
//         logger.log(buf);                                  \
//         exit(-1);                                         \
//     } while (0)

// #ifdef MUDEBUG
// #define log<DEBUG>(logmsgFormat, ...)                      \
//     do                                                    \
//     {                                                     \
//         Logger &logger = Logger::instance();              \
//         logger.setLogLevel(DEBUG);                        \
//         char buf[1024] = {0};                             \
//         snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
//         logger.log(buf);                                  \
//     } while (0)
// #else
// #define log<DEBUG>(logmsgFormat, ...)
// #endif