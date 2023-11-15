#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "EventLoop.h"
#include "Manager.h"

class EventLoop;
class Manager;

class ThreadPool{

public:

    using ThreadInitCallback = std::function<void(EventLoop *)>;

    ThreadPool(EventLoop *, const std::string &);
    ~ThreadPool();

    void setThread_num(int);

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    EventLoop *getNextLoop();
    std::vector<EventLoop *> getAllLoops();
    bool is_start() const;
    const std::string getName() const;

private:

    bool is_start_;

    EventLoop *baseLoop_;
    std::string name_;
    int threads_num_;
    int nextLoop_;
    std::vector<std::unique_ptr<Manager>> threads_;
    std::vector<EventLoop *> loops_;

};