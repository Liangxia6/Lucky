#include "Epoller.h"

const int kNew = -1;    // 某个channel还没添加至Poller
const int kAdded = 1;   // 某个channel已经添加至Poller
const int kDeleted = 2; // 某个channel已经从Poller删除

Epoller::Epoller(EventLoop *loop) 
    : ownerLoop_(loop)
    , epollfd_(epoll_create1(EPOLL_CLOEXEC))
    , events_(kEventListSize)
{
    if (epollfd_ < 0)
    {
        log<FATAL>("epoll_create error:%d \n", errno);
    }
}

Epoller::~Epoller() 
{
    close(epollfd_);
}

TimeStamp Epoller::epoll(int timeout, ChannelList *activeChannels)
{
    log<INFOM>("func=%s => fd total count:%lu\n", __FUNCTION__, channels_.size());

    //epoll_wait把检测到的事件都存储在events_数组中
    size_t events_num = epoll_wait(epollfd_, 
                    &*events_.begin(), 
                    static_cast<int>(events_.size()), 
                    timeout);
    int saveErrno = errno;
    TimeStamp now(TimeStamp::getNowTime());
    //有事件
    if (events_num > 0)
    {
        log<INFOM>("%d events happend\n", events_num);

        fillActiveChannels(events_num, activeChannels);
        //二倍扩容
        if (events_num == events_.size() - 1)
            events_.resize(events_.size() * 2);
    } 
    //超时
    else if (events_num == 0)
    {
        log<DEBUG>("%s timeout!\n", __FUNCTION__);
    } 
    //不是中断错误
    else if (saveErrno != EINTR)
    {
        errno = saveErrno;
        log<ERROR>("EPollPoller::poll() error!");
    }
    return now;
}

//判断channel是否已经注册到epoll
bool Epoller::hasChannel(Channel *channel) const
{
    auto it = channels_.find(channel->getFd());
    bool ret = ((it != channels_.end()) && (it->second == channel));
    return ret;
}

// //构造器 (可删除)
// Epoller *Epoller::newEpoller(EventLoop *loop) 
// {
//     return new Epoller(loop);
// }

//根据channel在EPoller中的当前状态来更新channel的状态
void Epoller::update(Channel *channel) 
{
    //channel的状态
    const int index = channel->getIndex();

    log<INFOM>("func=%s => fd=%d events=%d index=%d\n", __FUNCTION__, channel->getFd(), channel->getEvents(), index);

    int fd = channel->getFd();
    //未添加状态和已删除状态都有可能会被再次添加到epoll中
    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            //类hash的vector
            channels_[fd] = channel;
        }
        else if(index == kDeleted)
        {
            //这个处于kDeleted的channel还在channels_中，则继续执行
            assert(channels_.find(fd) != channels_.end());
            //channels_中记录的该fd对应的channel没有发生变化，则继续执行
            assert(channels_[fd] == channel);
        }
        //修改状态为已添加
        channel->setIndex(kAdded);
        updateHelper(EPOLL_CTL_ADD, channel);
    }
    //channel已经注册过
    else 
    {
        //从epoll删除此channel
        if (channel->isNoneEvent())
        {
            updateHelper(EPOLL_CTL_DEL, channel);
            channel->setIndex(kDeleted);
        } 
        //修改
        else 
        {
            updateHelper(EPOLL_CTL_MOD, channel);
        }
    }
}

// 当连接销毁时，从EPoller移除channel，这个移除并不是销毁channel，而只是把chanel的状态修改一下
void Epoller::remove(Channel *channel) 
{
    int fd = channel->getFd();

    log<INFOM>("func=%s => fd=%d\n", __FUNCTION__, fd);

    channels_.erase(fd);

    int index = channel->getIndex();
    if (index == kAdded)
    {
        updateHelper(EPOLL_CTL_DEL, channel);    
    }
    //重新设置为未注册
    channel->setIndex(kNew);
}

void Epoller::fillActiveChannels(int events_num, ChannelList *activeChannels) const
{
    for (int i = 0; i < events_num; ++i)
    {
        //获得events_对应的channel，并把events_[i].events赋值给该channel中用于记录实际发生事件的属性revents_
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->setRevents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void Epoller::updateHelper(int operation, Channel *channel)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));

    event.events = channel->getEvents();
    event.data.fd = channel->getFd();
    //关联至fillActiveChannels()中的
    //Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
    event.data.ptr = channel;

    //int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
    if (epoll_ctl(epollfd_, operation, channel->getFd(), &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            log<ERROR>("epoll_ctl del error:%d\n", errno);
        } 
        else 
        {
            log<FATAL>("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}
