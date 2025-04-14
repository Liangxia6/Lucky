#pragma once

#include <functional>
#include <memory>
#include <sys/epoll.h>

#include "TimeStamp.h"
#include "EventLoop.h"
#include "LuckyLog.h"

class EventLoop;

/**
 * @brief 通道类
 *
 * Filed类则封装了一个 [fd] 和这个 [fd感兴趣事件]
 * 以及事件监听器监听到 [该fd实际发生的事件]。
 * 同时Filed类还提供了设置该fd的感兴趣事件，
 * 以及将该fd及其感兴趣事件注册到事件监听器或从事件监听器上移除，
 * 以及保存了该fd的每种事件对应的处理函数。
 *
 * Filed并不负责fd的生命周期（P281, 8.1.1节），fd的生命周期是交给Socket类来管理的。
 *
 * 一个Filed负责一个fd
 * 封装了fd,引入回调函数
 * Filed的生命周期和Connection一样长，因为Filed是Connection的成员，
 */
class Filed
{

public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(TimeStamp)>;

    Filed(EventLoop *, int fd);
    ~Filed();

    // fd得到Epoller通知后的回调
    void handleEvent(TimeStamp);

    // 向所属的epoll中注册,删除fd感兴趣的事件,底层是epoll_ctl()
    void enableReading();
    void disableReading();
    void enableWriting();
    void disableWriting();
    void disableAll();

    // 返回fd当前事件的状态
    bool isNoneEvent() const;
    bool isWriting() const;
    bool isReading() const;

    int getFd() const;
    int getEvents() const;
    void setRevents(int);
    EventLoop *getLoop();
    /**
     * 返回fd在EPoller中的状态
     * for EPoller
     * const int kNew = -1;     // fd还未被Epoller监视
     * const int kAdded = 1;    // fd正被Epoller监视中
     * const int kDeleted = 2;  // fd被从Epoller中移除
     */
    int getIndex() const;
    void setIndex(int);

    // 将tie与Connection的shard_ptr绑定,防止Connection的意外析构
    void tie(const std::shared_ptr<void> &);

    // 设置各种回调
    void setReadCallback(ReadEventCallback);
    void setWriteCallback(EventCallback);
    void setCloseCallback(EventCallback);
    void setErrorCallback(EventCallback);

    // 把fd感兴趣的事件注册到epoller，本质就是调用epoll_ctl
    void update();
    // 从EPoller中移除自己，让EPoller停止关注自己感兴趣的事件
    void remove();

private:
    // void handleEventGuard(TimeStamp);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 当前Filed属于的EventLoop
    const int fd_;    // fd, Poller监听对象
    int events_;      // 注册fd感兴趣的事件
    // 事件监听器实际监听到该fd发生的事件类型集合，
    // 当事件监听器监听到一个fd发生了什么事件，通过Filed::set_revents()函数来设置revents值。
    int revents_; // poller返回的具体发生的事件
    int index_;   // 在EPoller上注册的状态（状态有kNew,kAdded, kDeleted）

    std::weak_ptr<void> tie_; // 监测Connection的弱指针,防止Connection的意外销毁，却还在执行回调操作
    bool tied_;               // 存储是否调用过Filed::tie

    // 表示这个Filed为这个文件描述符保存的各事件类型发生时的处理函数。来自Connection类
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};