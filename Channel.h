#pragma once

#include <functional>
#include <memory>

#include "TimeStamp.h"
#include "EventLoop.h"

class EventLoop;

class Channel{

public:

    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(TimeStamp)>;

    Channel(EventLoop *, int);
    ~Channel();

    void HandleEvent(TimeStamp);

    void enableReading();
    void disableReading();
    void enableWriting();
    void disableWriting();
    void disableAll();  

    bool isNoneEvent() const;
    bool isWriting() const;
    bool isReading() const;

    EventLoop *ownerLoop();

    int getFd() const;
    int getEvents() const;
    void setRevents(int);
    int getIndex() const;
    void setIndex(int);

    void tie(const std::shared_ptr<void> &);

    void setReadCallback(ReadEventCallback);
    void setWriteCallback(EventCallback);
    void setCloseCallback(EventCallback);
    void setErrorCallback(EventCallback); 

    void UpdateinChannel();
    void RemoveinChannel();

private:

    void HandleEventGuard(TimeStamp);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;
    const int fd_;
    int events_;
    int revents_;
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

};