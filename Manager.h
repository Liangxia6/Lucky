#pragma once

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

#include "EventLoop.h"
#include "Thread.h"

class EventLoop;

/**
 * @brief 管理类
 * 
 * 将Thread与EventLoop联系起来
*/
class Manager
{
public:

    using ThreadInitCallback = std::function<void(EventLoop *)>;

    Manager(const ThreadInitCallback & = ThreadInitCallback(),
            const std::string &name = std::string());
    ~Manager();

    //开启新线程
    EventLoop *startLoop();

private:

    //线程执行函数
    void threadFunc();

    bool is_exit_;

    EventLoop *loop_;               //线程(子线程)绑定的EventLoop
    Thread thread_;                 //EventLoop所属的线程
    std::mutex mutex_;              //互斥锁,这里是配合条件变量使用
    std::condition_variable cond_;  //条件变量, 主线程等待子线程创建EventLoop对象完成
    
    ThreadInitCallback callback_;   //线程初始化完成后要执行的函数

};