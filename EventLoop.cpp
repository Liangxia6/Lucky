#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

#include "EventLoop.h"
#include "LuckyLog.h"

__thread EventLoop *t_LoopinThread = nullptr;

const int kEpollTime = 10000;

int createEventfd() {
    int efd = eventfd(0, EFD_NONBLOCK|EFD_CLOEXEC );
    if (efd < 0) {
        LOG_FATAL("eventfd error:%d\n", errno);
    }
    return efd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , threadId_(getThreadID::tid())
    , wakeupFd_(createEventfd())
    , epoller_(Epoller::newEpoller(this))
    , wakeupChannel_(new Channel(this, wakeupFd_))
    , callPendFun_(false){

    LOG_DEBUG("EventLoop created %p in thread %d\n", this, threadId_);

    if (t_LoopinThread){
        LOG_FATAL("Another EventLoop %p exists in this thread %d\n", t_LoopinThread, threadId_);
    } else {
        t_LoopinThread = this;
    }
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::HandelRead, this));
    wakeupChannel_->enableReading(); 
}

EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->RemoveinChannel();    
    close(wakeupFd_);
    t_LoopinThread = nullptr;
}

void EventLoop::loop(){
    looping_ = true;
    quit_ = false;

    LOG_INFOM("EventLoop %p start looping\n", this);

    while (!quit_){
        activeChannels_.clear();
        epollReturnTime_ = epoller_->epoll(
            kEpollTime, &activeChannels_);
        
        for (Channel *channel : activeChannels_){
            channel->HandleEvent(epollReturnTime_);
        }

        doPendFun();
    }

    LOG_INFOM("EventLoop %p stop looping.\n", this);
    looping_ = false;
}

void EventLoop::quit(){
    quit_ = true;
    if (!isLoopThread()){
        wakeup();
    }
}

void EventLoop::RuninLoop(Callback cb){
    if (isLoopThread()){
        cb();
    } else {
        QueueinLoop(cb);
    }
}
void EventLoop::QueueinLoop(Callback cb){
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendFuns_.emplace_back(cb);
    }
    if (!isLoopThread() || callPendFun_){
        wakeup();
    }
}

void EventLoop::wakeup() {
    uint64_t u64one = 1;
    ssize_t n = write(wakeupFd_, &u64one, sizeof(u64one));

    if (n != sizeof(u64one)){
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
    }
}

bool EventLoop::isLoopThread() const{
    return threadId_ == getThreadID::tid();
}

bool EventLoop::hasChannel(Channel *channel){
    epoller_->hasChannel(channel);
}

TimeStamp EventLoop::epollReturnTime() const{
    return epollReturnTime_;
}

void EventLoop::UpdateinEventLoop(Channel *channel){
    epoller_->UpdateinEpoller(channel);
}

void EventLoop::RemoveinEventLoop(Channel *channel){
    epoller_->RemoveinEpoller(channel);
}

void EventLoop::HandelRead(){
    uint64_t u64one = 1;
    ssize_t n = write(wakeupFd_, &u64one, sizeof(u64one));

    if (n != sizeof(u64one)){
        LOG_ERROR("EventLoop::HandleRead() writes %lu bytes instead of 8\n", n);
    }
}

void EventLoop::doPendFun(){
    std::vector<Callback> functors;
    callPendFun_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendFuns_); // 交换的方式减少了锁的临界区范围 提升效率 同时避免了死锁 如果执行functor()在临界区内 且functor()中调用queueInLoop()就会产生死锁
    }
    for (const Callback &functor : functors){
        functor(); // 执行当前loop需要执行的回调操作
    }
    callPendFun_ = false;
}