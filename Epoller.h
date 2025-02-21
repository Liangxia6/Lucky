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
#include "Filed.h"
#include "EventLoop.h"

class Filed;
class EventLoop;

/**
 *
 */
class Epoller
{
public:
    using FiledList = std::vector<Filed *>;

    Epoller(EventLoop *);
    ~Epoller();

    // 底层函数epoll_wait,填充activeFiled列表
    TimeStamp epoll(int, FiledList *);

    // 判断filed是否已经注册到epoll
    bool hasFiled(Filed *) const;
    // //构造器
    // static Epoller *newEpoller(EventLoop *);

    // 调用私有的updateHel中的perepoll_ctl()
    void update(Filed *);
    // 连接销毁时,移除epoll中的filed
    void remove(Filed *);

private:
    // 默认监听数量,可扩容,二倍扩容法
    static const int kEventListSize = 16;

    void fillActiveFileds(int, FiledList *) const;
    void updateHelper(int, Filed *);

    using EventList = std::vector<epoll_event>;
    // 存储epoll_wait返回的事件
    EventList events_;

    using FiledMap = std::unordered_map<int, Filed *>;
    // 存储Filed映射,通过socketfd找到filed
    FiledMap fileds_;

    // epoll_creat返回值
    int epollfd_;
    // 绑定所属的EventLoop(Thread)
    EventLoop *ownerLoop_;
};