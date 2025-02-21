#include "LuckyLog.h"

const char *LuckyLog::headers[4] = {
    "[INFOM]",
    "[ERROR]",
    "[FATAL]",
    "[DEBUG]"};

// 获取日志唯一的实例对象 单例
LuckyLog &LuckyLog::LogSingleton()
{
    static LuckyLog luckylog;
    return luckylog;
}

// 设置日志级别
void LuckyLog::setLogLevel(int loglevel)
{
    loglevel_ = loglevel;
}

// 写日志的底层函数:cout
void LuckyLog::writeLog(std::string msg)
{
    std::string head = headers[loglevel_];
    // 打印时间和msg
    std::cout << head + TimeStamp::getNowTime().toString() << ": " << msg << std::endl;
}

// // 辅助函数
// template <typename... Args>
// void logHelper(const char *format, LuckyLog *luckylog, Args... args)
// {
//     char buff[1024] = {0};
//     snprintf(buff, 1024, format, args...);
//     luckylog->writeLog(buff);
// }

// // 模板函数,使用方法LOG_INFOM(massage);
// template <LogLevel level, typename... Args>
// void log(const char *format, Args... args)
// {
//     // 创建单例
//     LuckyLog luckylog = LuckyLog::LogSingleton();
//     luckylog.setLogLevel(level);

//     logHelper(format, &luckylog, args...);

//     if (level == LogLevel::FATAL)
//     {
//         exit(-1);
//     }
// }

// int main()
// {
//     LOG_INFOM("hhh");
//     LOG_ERROR("八嘎");
// }
