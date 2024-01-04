#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "LuckyLog.h"
#include "TimeStamp.h"
#include "Channel.h"
#include "Epoller.h"
#include "getThreadID.h"

class Channel;
class Epoller;

/**
 * 
*/
class EventLoop 
{
public:

    using Callback = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    //让回调函数在此线程直接执行
    void runinLoop(Callback);
    //让回调函数加入pendingFunc中等待,在下一个EventLoop中再执行
    void queueinLoop(Callback);

    //唤醒epoller,避免阻塞再epoll_wait (向wakeuoFd写入读事件)
    void wakeup();
    //判断EventLoop初始化时绑定的线程id是否和当前正在运行的线程id是否一致
    bool isLoopThread() const;
    //判断channel是否在当前epoller
    bool hasChannel(Channel *);

    //EPoller管理的fd有事件发生时的时间（也就是epoll_wait返回的时间）
    TimeStamp epollReturnTime() const;

    //再epoll中更新channel感兴趣事件,底层函数为epoll_ctl()
    void update(Channel *);
    //删除channel感兴趣的fd
    void remove(Channel *);

private:

    //wakeupChannel的读事件回调
    void handelwakeupRead();
    void doPendFunc();

    std::atomic_bool looping_;  //是否在事件循环中
    std::atomic_bool quit_;     //是否退出事件循环

    std::mutex mutex_;          //保护pendFuncs_
    const pid_t threadId_;      //当前loop的线程
    TimeStamp epollReturnTime_; 

    std::unique_ptr<Epoller> epoller_;
    int wakeupFd_;              //唤醒epoll的事件
    std::unique_ptr<Channel> wakeupChannel_;
    
    using ChannelList = std::vector<Channel *>;
    ChannelList activeChannels_; 

    std::atomic_bool isCallPendFunc_;     //是否正在调用待执行的函数
    std::vector<Callback> pendFuncs_;   //存储loop跨线程需要执行的所有回调操作

};