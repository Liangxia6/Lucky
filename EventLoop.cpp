#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

#include "EventLoop.h"
#include "LuckyLog.h"

__thread EventLoop *t_LoopinThread = nullptr;

const int keyEpollTime = 10000;

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
        LOG_FATAL("Another EventLoop %p exists in this thread %d\n", t_LoopinThread, this->threadId_);
    } else {
        t_LoopinThread = this;
    }
    this->wakeupChannel_->setReadCallback(std::bind(&HandelRead, this));
    this->wakeupChannel_->enableReading(); 
}

EventLoop::~EventLoop(){
    this->wakeupChannel_->disableAll();
    this->wakeupChannel_->RemoveinChannel();    
    close(wakeupFd_);
    t_LoopinThread = nullptr;
}

void EventLoop::loop(){
    this->looping_ = true;
    this->quit_ = false;

    LOG_INFOM("EventLoop %p start looping\n", this);

    while (!this->quit_){
        this->activeChannels_.clear();
        this->epollReturnTime_ = this->epoller_->epoll(
            keyEpollTime, &this->activeChannels_);
        
        for (Channel *channel : this->activeChannels_){
            channel->HandleEvent(epollReturnTime_);
        }

        this->doPendFun();
    }

    LOG_INFOM("EventLoop %p stop looping.\n", this);
    this->looping_ = false;
}

void EventLoop::quit(){
    this->quit_ = true;
    if (!this->isLoopThread()){
        this->wakeup();
    }
}

void EventLoop::RuninLoop(Callback cb){
    if (this->isLoopThread()){
        cb();
    } else {
        this->QueueinLoop(cb);
    }
}
void EventLoop::QueueinLoop(Callback cb){
    {
        std::unique_lock<std::mutex> lock(this->mutex_);
        this->pendFuns_.emplace_back(cb);
    }
    if (!this->isLoopThread() || this->callPendFun_){
        this->wakeup();
    }
}

void EventLoop::wakeup() {
    uint64_t u64one = 1;
    ssize_t n = write(this->wakeupFd_, &u64one, sizeof(u64one));

    if (n != sizeof(u64one)){
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
    }
}

bool EventLoop::isLoopThread() const{
    return this->threadId_ == getThreadID::tid();
}

bool EventLoop::hasChannel(Channel *channel){
    this->epoller_->hasChannel(channel);
}

TimeStamp EventLoop::epollReturnTime() const{
    return this->epollReturnTime_;
}

void EventLoop::UpdateinEventLoop(Channel *channel){
    this->epoller_->UpdateinEpoller(channel);
}

void EventLoop::RemoveinEventLoop(Channel *channel){
    this->epoller_->RemoveinEpoller(channel);
}

void EventLoop::HandelRead(){
    uint64_t u64one = 1;
    ssize_t n = write(this->wakeupFd_, &u64one, sizeof(u64one));

    if (n != sizeof(u64one)){
        LOG_ERROR("EventLoop::HandleRead() writes %lu bytes instead of 8\n", n);
    }
}

void EventLoop::doPendFun(){
    std::vector<Callback> functors;
    this->callPendFun_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(this->pendFuns_); // 交换的方式减少了锁的临界区范围 提升效率 同时避免了死锁 如果执行functor()在临界区内 且functor()中调用queueInLoop()就会产生死锁
    }
    for (const Callback &functor : functors){
        functor(); // 执行当前loop需要执行的回调操作
    }
    this->callPendFun_ = false;
}