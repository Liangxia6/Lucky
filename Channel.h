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
 * 封装了fd,引入回调函数
 * Channel的生命周期和Connection一样长，因为Channel是Connection的成员，
*/
class Channel
{

public:

    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(TimeStamp)>;

    Channel(EventLoop *, int);
    ~Channel();

    //fd得到Epoller通知后的回调
    void handleEvent(TimeStamp);

    //像所属的epoll中注册,删除fd感兴趣的事件,底层是epoll_ctl()
    void enableReading();
    void disableReading();
    void enableWriting();
    void disableWriting();
    void disableAll();  

    //返回fd当前事件的状态
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

    //将tie与Connection的shard_ptr绑定,防止Connection的意外析构
    void tie(const std::shared_ptr<void> &);

    //设置各种回调
    void setReadCallback(ReadEventCallback);
    void setWriteCallback(EventCallback);
    void setCloseCallback(EventCallback);
    void setErrorCallback(EventCallback); 

    //底层调用epoll_ctl()
    void update();
    //停止epoll监听fd感兴趣的事件
    void remove();

private:

    void handleEventGuard(TimeStamp);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;           // 当前Channel属于的EventLoop
    const int fd_;              // fd, Poller监听对象
    int events_;                // 注册fd感兴趣的事件
    int revents_;               // poller返回的具体发生的事件
    int index_;                 // 在EPoller上注册的状态（状态有kNew,kAdded, kDeleted）

    std::weak_ptr<void> tie_;   //监测Connection的弱指针,防止Connection的意外销毁
    bool tied_;                 //存储是否调用过Channel::tie

    //保存回调函数,来自Connection类
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

};