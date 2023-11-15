#pragma once

#include <vector>
#include <unordered_map>
#include <sys/epoll.h>

#include "TimeStamp.h"
#include "Channel.h"
#include "EventLoop.h"

class Channel;
class EventLoop;

class Epoller{

public:

    using ChannelList = std::vector<Channel *>;

    Epoller(EventLoop *);
    ~Epoller();

    TimeStamp epoll(int, ChannelList *);

    bool hasChannel(Channel *) const;
    static Epoller *newEpoller(EventLoop *);

    void UpdateinEpoller(Channel *);
    void RemoveinEpoller(Channel *);

private:

    static const int kEventListSize = 16;

    void fillActiveChannels(int, ChannelList *) const;
    void update(int, Channel *);

    using EventList = std::vector<epoll_event>;
    EventList events_;

    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

    int epollfd_;
    EventLoop *ownerLoop_;

};