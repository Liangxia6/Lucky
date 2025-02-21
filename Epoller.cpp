#include "Epoller.h"

const int kNew = -1;    // 某个filed还没添加至Poller
const int kAdded = 1;   // 某个filed已经添加至Poller
const int kDeleted = 2; // 某个filed已经从Poller删除

Epoller::Epoller(EventLoop *loop)
    : ownerLoop_(loop), epollfd_(epoll_create1(EPOLL_CLOEXEC)), events_(kEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

Epoller::~Epoller()
{
    close(epollfd_);
}

TimeStamp Epoller::epoll(int timeout, FiledList *activeFileds)
{
    LOG_INFOM("func=%s => fd total count:%lu\n", __FUNCTION__, fileds_.size());

    // epoll_wait把检测到的事件都存储在events_数组中
    size_t events_num = epoll_wait(epollfd_,
                                   &*events_.begin(),
                                   static_cast<int>(events_.size()),
                                   timeout);
    int saveErrno = errno;
    TimeStamp now(TimeStamp::getNowTime());
    // 有事件
    if (events_num > 0)
    {
        LOG_INFOM("%d events happend\n", events_num);

        fillActiveFileds(events_num, activeFileds);
        // 二倍扩容
        if (events_num == events_.size() - 1)
            events_.resize(events_.size() * 2);
    }
    // 超时
    else if (events_num == 0)
    {
        log<DEBUG>("%s timeout!\n", __FUNCTION__);
    }
    // 不是中断错误
    else if (saveErrno != EINTR)
    {
        errno = saveErrno;
        LOG_ERROR("EPollPoller::poll() error!");
    }
    return now;
}

// 判断filed是否已经注册到epoll
bool Epoller::hasFiled(Filed *filed) const
{
    auto it = fileds_.find(filed->getFd());
    bool ret = ((it != fileds_.end()) && (it->second == filed));
    return ret;
}

// //构造器 (可删除)
// Epoller *Epoller::newEpoller(EventLoop *loop)
// {
//     return new Epoller(loop);
// }

// 根据filed在EPoller中的当前状态来更新filed的状态
void Epoller::update(Filed *filed)
{
    // filed的状态
    const int index = filed->getIndex();

    LOG_INFOM("func=%s => fd=%d events=%d index=%d\n", __FUNCTION__, filed->getFd(), filed->getEvents(), index);

    int fd = filed->getFd();
    // 未添加状态和已删除状态都有可能会被再次添加到epoll中
    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            // 类hash的vector
            fileds_[fd] = filed;
        }
        else if (index == kDeleted)
        {
            // 这个处于kDeleted的filed还在fileds_中，则继续执行
            assert(fileds_.find(fd) != fileds_.end());
            // fileds_中记录的该fd对应的filed没有发生变化，则继续执行
            assert(fileds_[fd] == filed);
        }
        // 修改状态为已添加
        filed->setIndex(kAdded);
        updateHelper(EPOLL_CTL_ADD, filed);
    }
    // filed已经注册过
    else
    {
        // 从epoll删除此filed
        if (filed->isNoneEvent())
        {
            updateHelper(EPOLL_CTL_DEL, filed);
            filed->setIndex(kDeleted);
        }
        // 修改
        else
        {
            updateHelper(EPOLL_CTL_MOD, filed);
        }
    }
}

// 当连接销毁时，从EPoller移除filed，这个移除并不是销毁filed，而只是把chanel的状态修改一下
void Epoller::remove(Filed *filed)
{
    int fd = filed->getFd();

    LOG_INFOM("func=%s => fd=%d\n", __FUNCTION__, fd);

    fileds_.erase(fd);

    int index = filed->getIndex();
    if (index == kAdded)
    {
        updateHelper(EPOLL_CTL_DEL, filed);
    }
    // 重新设置为未注册
    filed->setIndex(kNew);
}

void Epoller::fillActiveFileds(int events_num, FiledList *activeFileds) const
{
    for (int i = 0; i < events_num; ++i)
    {
        // 获得events_对应的filed，并把events_[i].events赋值给该filed中用于记录实际发生事件的属性revents_
        Filed *filed = static_cast<Filed *>(events_[i].data.ptr);
        filed->setRevents(events_[i].events);
        activeFileds->push_back(filed);
    }
}

void Epoller::updateHelper(int operation, Filed *filed)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));

    event.events = filed->getEvents();
    event.data.fd = filed->getFd();
    // 关联至fillActiveFileds()中的
    // Filed *filed = static_cast<Filed *>(events_[i].data.ptr);
    event.data.ptr = filed;

    // int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
    if (epoll_ctl(epollfd_, operation, filed->getFd(), &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}
