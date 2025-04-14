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
#include "Filed.h"
#include "Epoller.h"
#include "ThreadID.h"

class Filed;
class Epoller;

/**
 * EventLoop有持续监听、持续获取监听结果、持续处理监听结果对应的事件的能力
 * 循环的调用Poller:poll方法获取实际发生事件的Filed集合，
 * 然后调用这些Filed里面保管的不同类型事件的处理函数（调用Filed::HandlerEvent方法）
 *
 * Filed和Poller其实相当于EventLoop的手下，
 * EventLoop整合封装了二者并向上提供了更方便的接口来使用。
 *
 * 每一个EventLoop都绑定了一个线程（一对一绑定）
 */

class EventLoop
{
public:
    using Callback = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    // 让回调函数在此线程直接执行
    void runinLoop(Callback);
    // 让回调函数加入pendingFunc中等待,在下一个EventLoop中再执行
    void queueinLoop(Callback);

    // 主R唤醒子R的epoller,避免阻塞在epoll_wait (向wakeuoFd写入读事件)
    void wakeup();
    // 判断EventLoop初始化时绑定的线程id是否和当前正在运行的线程id是否一致
    bool isLoopThread() const;
    // 判断filed是否在当前epoller
    bool hasFiled(Filed *);

    // EPoller管理的fd有事件发生时的时间（也就是epoll_wait返回的时间）
    TimeStamp epollReturnTime() const;

    // 再epoll中更新filed感兴趣事件,底层函数为epoll_ctl()
    void update(Filed *);
    // 删除filed感兴趣的fd
    void remove(Filed *);

private:
    // wakeup的读事件回调
    void handelwakeupRead();
    void doPendFunc();

    std::atomic_bool looping_; // 是否在事件循环中
    std::atomic_bool quit_;    // 是否退出事件循环

    std::mutex mutex_;     // 保护pendFuncs_
    const pid_t threadId_; // 当前loop的线程
    TimeStamp epollReturnTime_;

    std::unique_ptr<Epoller> epoller_;
    int wakeupFd_;                       // 唤醒epoll的事件
    std::unique_ptr<Filed> wakeupFiled_; // 连接主Reactor与子Reactor,主通知子有新的fd

    using FiledList = std::vector<Filed *>;
    FiledList activeFileds_;

    std::atomic_bool isCallPendFunc_; // 是否正在调用待执行的函数
    std::vector<Callback> pendFuncs_; // 存储loop跨线程需要执行的所有回调操作
};