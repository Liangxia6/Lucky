#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "EventLoop.h"
#include "Manager.h"

class EventLoop;
class Manager;

/**
 * 
*/
class ThreadPool
{
public:

    using ThreadInitCallback = std::function<void(EventLoop *)>;

    ThreadPool(EventLoop *, const std::string &);
    ~ThreadPool();

    void setThread_num(int);

    //启动线程池
    void start(const ThreadInitCallback & = ThreadInitCallback());

    //主Reactor将新接受的连接分发给子Reactor时，通过轮询的方式获取应该分发给哪个子Reactor（EventLoop）
    //下版本改用负载均衡算法
    EventLoop *getNextLoop();
    std::vector<EventLoop *> getAllLoops();

    bool isStart() const;
    const std::string getName() const;

private:

    bool is_start_;

    EventLoop *baseLoop_;   //主线程对应的主EventLoop
    std::string name_;
    int threads_num_;
    int nextLoop_;          //轮询的下标
    std::vector<std::unique_ptr<Manager>> threads_;
    std::vector<EventLoop *> loops_;

};