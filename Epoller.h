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
 *负责监听文件描述符事件是否触发以及返回发生事件的文件描述符以及具体事件
 * 每个EventLoop包含一个Epoller
 * 通过哈希表fileds_可以根据fd找到封装这个fd的Filed。
 * 将事件监听器监听到该fd发生的事件写进这个Filed中的revents成员变量中
 *
 * 然后把这个Filed装进activeFileds中（它是一个vector<Filed*>）。
 * 当外界调用完poll之后就能拿到事件监听器的监听结果（activeFileds_）
 * activeFileds就是事件监听器监听到的发生事件的fd，以及每个fd都发生了什么事件。
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

    // 调用私有的updateHel中的perepoll_ctl(),同步Filed类的状态
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