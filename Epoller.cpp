#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "Epoller.h"
#include "LuckyLog.h"

const int keyNew = -1;    // 某个channel还没添加至Poller
const int keyAdded = 1;   // 某个channel已经添加至Poller
const int keyDeleted = 2; // 某个channel已经从Poller删除

Epoller::Epoller(EventLoop *loop) 
    : ownerLoop_(loop)
    , epollfd_(epoll_create1(EPOLL_CLOEXEC))
    , events_(keyEventListSize){
    if (this->epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

Epoller::~Epoller() {
    close(this->epollfd_);
}

TimeStamp Epoller::epoll(int timeout, ChannelList *activeChannels){
    LOG_INFOM("func=%s => fd total count:%lu\n", __FUNCTION__, channels_.size());

    int events_num = epoll_wait(this->epollfd_, 
            &*this->events_.begin(), 
            static_cast<int>(events_.size()), 
            timeout);
    int saveErrno = errno;
    TimeStamp now(TimeStamp::NowTime());
    
    if (events_num > 0){
        LOG_INFOM("%d events happend\n", events_num);
        this->fillActiveChannels(events_num, activeChannels);
        if (events_num == this->events_.size() - 1)
            this->events_.resize(this->events_.size() * 2);
    } else if (events_num == 0){
        LOG_DEBUG("%s timeout!\n", __FUNCTION__);
    } else if (saveErrno != EINTR){
        errno = saveErrno;
        LOG_ERROR("EPollPoller::poll() error!");
    }
    return now;
}


void Epoller::UpdateinEpoller(Channel *channel) {
    const int index = channel->getIndex();
    LOG_INFOM("func=%s => fd=%d events=%d index=%d\n", __FUNCTION__, channel->getFd(), channel->getEvents(), index);
    if (index == keyNew || index == keyDeleted){
        if (index == keyNew){
        int fd = channel->getFd();
        this->channels_[fd] = channel;
        }
        channel->setIndex(keyAdded);
        update(EPOLL_CTL_ADD, channel);
    } else {
        int fd = channel->getFd();
        if (channel->isNoneEvent()){
            update(EPOLL_CTL_DEL, channel);
            channel->setIndex(keyDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void Epoller::RemoveinEpoller(Channel *channel) {
    int fd = channel->getFd();

    LOG_INFOM("func=%s => fd=%d\n", __FUNCTION__, fd);

    this->channels_.erase(fd);

    int index = channel->getIndex();
    if (index == keyAdded){
        update(EPOLL_CTL_DEL, channel);    
    }
    channel->setIndex(keyNew);
}

bool Epoller::hasChannel(Channel *channel) const{
    auto it = this->channels_.find(channel->getFd());
    auto ret = it != this->channels_.end() && it->second == channel;
    return ret;
}

Epoller *Epoller::newEpoller(EventLoop *loop) {
    return new Epoller(loop);
}

void Epoller::fillActiveChannels(int events_num, ChannelList *activeChannels) const{
    for (int i = 0; i < events_num; ++i)
    {
        Channel *channel = static_cast<Channel *>(this->events_[i].data.ptr);
        channel->setRevents(this->events_[i].events);
        activeChannels->push_back(channel);
    }
}

void Epoller::update(int operation, Channel *channel){
    epoll_event event;
    memset(&event, 0, sizeof(event));

    event.events = channel->getEvents();
    event.data.fd = channel->getFd();
    event.data.ptr = channel;

    //int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
    if (epoll_ctl(epollfd_, operation, channel->getFd(), &event) < 0){
        if (operation == EPOLL_CTL_DEL){
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        } else {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}
