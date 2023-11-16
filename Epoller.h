#pragma once

#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "LuckyLog.h"
#include "TimeStamp.h"
#include "Channel.h"
#include "EventLoop.h"

class Channel;
class EventLoop;

class Epoller
{
public:

    using ChannelList = std::vector<Channel *>;

    Epoller(EventLoop *);
    ~Epoller();

    //底层函数epoll_wait,填充activeChannel列表
    TimeStamp epoll(int, ChannelList *);

    //判断channel是否已经注册到epoll
    bool hasChannel(Channel *) const;
    // //构造器
    // static Epoller *newEpoller(EventLoop *);

    //调用私有的updateHel中的perepoll_ctl()
    void update(Channel *);
    //连接销毁时,移除epoll中的channel
    void remove(Channel *);

private:

    //默认监听数量,可扩容,二倍扩容法
    static const int kEventListSize = 16;

    void fillActiveChannels(int, ChannelList *) const;
    void updateHelper(int, Channel *);

    using EventList = std::vector<epoll_event>;
    //存储epoll_wait返回的事件
    EventList events_;

    using ChannelMap = std::unordered_map<int, Channel *>;
    //存储Channel映射,通过socketfd找到channel
    ChannelMap channels_;

    //epoll_creat返回值
    int epollfd_;
    //绑定所属的EventLoop(Thread)
    EventLoop *ownerLoop_;

};