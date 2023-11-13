#include <iostream>
#include <cstdarg>

#include "LuckyLog.h"
#include "TimeStamp.h"

// 获取日志唯一的实例对象 单例
LuckyLog &LuckyLog::LogInstance(){
    static LuckyLog luckylog;
    return luckylog;
}

// 设置日志级别
void LuckyLog::setLogLevel(int loglevel){
    loglevel_ = loglevel;
}


// 写日志
void LuckyLog::LogWritter(std::string msg){
    std::string head("");
    switch (loglevel_)
    {
    case INFOM:
        head = "[INFOM]";
        break;
    case ERROR:
        head = "[ERROR]";
        break;
    case FATAL:
        head = "[FATAL]";
        break;
    case DEBUG:
        head = "[DEBUG]";
        break;
    default:
        break;
    }

     // 打印时间和msg
    std::cout << head + TimeStamp::NowTime().TimetoString()
            << " : " << msg << std::endl;
}

void logMessage(LogLevel level, const char* format, ...) {
    LuckyLog luckylog = LuckyLog::LogInstance();
    luckylog.setLogLevel(level);
    
    va_list args;
    va_start(args, format);
    
    char buff[1024] = {0};
    vsnprintf(buff, 1024, format, args);
    
    va_end(args);
    
    luckylog.LogWritter(buff);
    
    if (level == FATAL) {
        exit(-1);
    }
}
