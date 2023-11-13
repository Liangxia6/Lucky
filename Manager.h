#pragma once

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

#include "EventLoop.h"
#include "Thread.h"

class Manager{

public:

    using ThreadInitCallback = std::function<void(EventLoop *)>;

    Manager(const ThreadInitCallback & = ThreadInitCallback(),
            const std::string &name = std::string());
    ~Manager();

    EventLoop *startLoop();

private:

    void threadFunc();

    bool is_exit_;

    EventLoop *loop_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;

};