#include <sys/epoll.h>

#include "Channel.h"
#include "EventLoop.h"
#include "LuckyLog.h"

const int Channel::keyNoneEvent = 0;
const int Channel::keyReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::keyWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)
    , tied_(0){

}

Channel::~Channel(){

}

void Channel::HandleEvent(TimeStamp receiveTime)
{
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            HandleEventGuard(receiveTime);
        }
        // 如果提升失败了 就不做任何处理 说明Channel的TcpConnection对象已经不存在了
    }
    else
    {
        HandleEventGuard(receiveTime);
    }
}

void Channel::enableReading() { 
    events_ |= keyReadEvent; 
    UpdateinChannel(); 
}

void Channel::disableReading() {
    events_ &= ~keyReadEvent; 
    UpdateinChannel(); 
}

void Channel::enableWriting() { 
    events_ |= keyWriteEvent; 
    UpdateinChannel(); 
}

void Channel::disableWriting() {
    events_ &= ~keyWriteEvent;
    UpdateinChannel();
}

void Channel::disableAll() {
    events_ =  keyNoneEvent;
    UpdateinChannel();
}

bool Channel::isNoneEvent() const{
    return events_ == keyNoneEvent;
}

bool Channel::isWriting() const{
    return events_ & keyWriteEvent;
}

bool Channel::isReading() const{
    return events_ & keyReadEvent;
}


int Channel::getFd() const {
    return fd_;
}
int Channel::getEvents() const {
    return events_;
}
void Channel::setRevents(int revent) {
    revents_ = revent;
}

int Channel::getIndex() const{
    return index_;
}

void Channel::setIndex(int index) {
    index_ = index;
}

EventLoop * Channel::ownerLoop(){
    return loop_;
}

void Channel::setReadCallback(ReadEventCallback cb) { 
    readCallback_ = std::move(cb); 
}
void Channel::setWriteCallback(EventCallback cb) { 
    writeCallback_ = std::move(cb); 
}

void Channel::setCloseCallback(EventCallback cb) { 
    closeCallback_ = std::move(cb); 
}

void Channel::setErrorCallback(EventCallback cb) { 
    errorCallback_ = std::move(cb); 
}
    
void Channel::tie(const std::shared_ptr<void> &obj){
    tie_ = obj;
    tied_ = true;
}

void Channel::UpdateinChannel() {
    loop_->UpdateinEventLoop(this);
}

void Channel::RemoveinChannel() {
    loop_->RemoveinEventLoop(this);
}

void Channel::HandleEventGuard(TimeStamp receiveTime)
{
    LOG_INFOM("channel handleEvent revents:%d\n", revents_);
    // 关闭
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) // 当TcpConnection对应Channel 通过shutdown 关闭写端 epoll触发EPOLLHUP
    {
        if (closeCallback_)
        {
            closeCallback_();
        }
    }
    // 错误
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }
    // 读
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }
    // 写
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}