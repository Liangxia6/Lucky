#pragma once

#include <functional>
#include <string>
#include <atomic>
#include <thread>
#include <semaphore.h>
#include <memory>
#include <unistd.h>

#include "getThreadID.h"

/**
 * 
*/
class Thread
{
public:

    using ThreadFunc = std::function<void()>;

    Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    //创建/启动线程
    void start();
    void join();

    //私有成员变量接口函数
    bool isStart();
    pid_t getTid() const;
    void setName();
    const std::string &getName() const;

    static int getCreateNum();

private:

    bool is_start_;
    bool is_join_;
    
    std::shared_ptr<std::thread> thread_;   //线程主体
    pid_t tid_;                             //线程id
    std::string name_;                      //线程名称
    ThreadFunc func_;                       //线程执行函数

    static std::atomic_int create_num_;     //线程数

};