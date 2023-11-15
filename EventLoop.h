#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "TimeStamp.h"
#include "Channel.h"
#include "Epoller.h"
#include "getThreadID.h"

class Channel;
class Epoller;

class EventLoop {

public:
    using Callback = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    void RuninLoop(Callback);
    void QueueinLoop(Callback);

    void wakeup();

    bool isLoopThread() const;
    bool hasChannel(Channel *);

    TimeStamp epollReturnTime() const;

    void UpdateinEventLoop(Channel *);
    void RemoveinEventLoop(Channel *);

private:

    void HandelRead();
    void doPendFun();

    std::atomic_bool looping_; 
    std::atomic_bool quit_;

    std::mutex mutex_;    
    const pid_t threadId_; 
    int wakeupFd_;
    TimeStamp epollReturnTime_;

    std::unique_ptr<Epoller> epoller_;
    std::unique_ptr<Channel> wakeupChannel_;
    
    using ChannelList = std::vector<Channel *>;
    ChannelList activeChannels_; 

    std::atomic_bool callPendFun_; 
    std::vector<Callback> pendFuns_;

};