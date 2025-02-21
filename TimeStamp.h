#pragma once

#include <iostream>
#include <string>
#include <time.h>
#include <cstdio>

/**
 * @brief 时间戳类
 *
 *  服务于日志类
 */
class TimeStamp
{
public:
    // 默认初始化构造为0
    TimeStamp();
    TimeStamp(int64_t);

    // 获取当前时间戳
    static TimeStamp getNowTime();
    // 返回字符串类型时间戳
    std::string toString() const;

private:
    // 64位int格式的系统时间
    int64_t sysSecond_;
};