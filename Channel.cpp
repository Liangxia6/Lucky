#include "Channel.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)     //fd还未被Epoller监视 
    , tied_(0)
{
}

Channel::~Channel()
{

}

void Channel::handleEvent(TimeStamp receiveTime)
{
    /**
     * Connection::connectEstablished会调用channel_->tie(shared_from_this());
     * 调用了Channel::tie会设置tid_=true
     * 所以对于TcpConnection::channel_ 需要多一份强引用的保证以免用户误删Connection对象
     */
    if (tied_)
    {
        // 变成shared_ptr增加引用计数，防止误删
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEventGuard(receiveTime);
        }
        // 如果提升失败了 就不做任何处理 说明Channel的Connection对象已经不存在了
    }
    else
    {
        handleEventGuard(receiveTime);
    }
}


// void Channel::handleEventGuard(TimeStamp receiveTime)
// {
//     log<INFOM>("channel handleEvent revents:%d\n", revents_);
//     // 关闭
//     if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) // 当Connection对应Channel 通过shutdown 关闭写端 epoll触发EPOLLHUP
//     {
//         if (closeCallback_)
//         {
//             closeCallback_();
//         }
//     }
//     // 错误
//     if (revents_ & EPOLLERR)
//     {
//         if (errorCallback_)
//         {
//             errorCallback_();
//         }
//     }
//     // 读
//     if (revents_ & (EPOLLIN | EPOLLPRI))
//     {
//         if (readCallback_)
//         {
//             readCallback_(receiveTime);
//         }
//     }
//     // 写
//     if (revents_ & EPOLLOUT)
//     {
//         if (writeCallback_)
//         {
//             writeCallback_();
//         }
//     }
// }

//优化后
void Channel::handleEventGuard(TimeStamp receiveTime)
{
    using Callback = std::function<void()>;
    std::array<Callback, 4> callbacks;

    // 将回调函数存储到函数指针数组中
    callbacks[0] = closeCallback_;
    callbacks[1] = errorCallback_;
    callbacks[2] = std::bind(readCallback_, receiveTime);
    callbacks[3] = writeCallback_;

    // 处理关闭事件
    if (revents_ & EPOLLHUP)
    {
        // 如果同时满足EPOLLHUP和EPOLLIN条件，执行关闭回调函数
        if ((revents_ & (EPOLLHUP | EPOLLIN)) == EPOLLHUP && closeCallback_)
        {
            callbacks[0]();
        }
    }

    // 处理错误事件
    if (revents_ & EPOLLERR && errorCallback_)
    {
        callbacks[1]();
    }

    // 处理读事件
    if (revents_ & (EPOLLIN | EPOLLPRI) && readCallback_)
    {
        callbacks[2]();
    }

    // 处理写事件
    if (revents_ & EPOLLOUT && writeCallback_)
    {
        callbacks[3]();
    }
}


void Channel::enableReading() 
{ 
    events_ |= kReadEvent; 
    update(); 
}

void Channel::disableReading() 
{
    events_ &= ~kReadEvent; 
    update(); 
}

void Channel::enableWriting() 
{ 
    events_ |= kWriteEvent; 
    update(); 
}

void Channel::disableWriting() 
{
    events_ &= ~kWriteEvent;
    update();
}

void Channel::disableAll() 
{
    events_ =  kNoneEvent;
    update();
}

bool Channel::isNoneEvent() const
{
    return events_ == kNoneEvent;
}

bool Channel::isWriting() const
{
    return events_ & kWriteEvent;
}

bool Channel::isReading() const
{
    return events_ & kReadEvent;
}


int Channel::getFd() const 
{
    return fd_;
}

int Channel::getEvents() const 
{
    return events_;
}

void Channel::setRevents(int revent) 
{
    revents_ = revent;
}

int Channel::getIndex() const
{
    return index_;
}

void Channel::setIndex(int index) 
{
    index_ = index;
}

EventLoop * Channel::getLoop()
{
    return loop_;
}

void Channel::setReadCallback(ReadEventCallback cb) 
{ 
    readCallback_ = std::move(cb); 
}
void Channel::setWriteCallback(EventCallback cb) 
{ 
    writeCallback_ = std::move(cb); 
}

void Channel::setCloseCallback(EventCallback cb) 
{ 
    closeCallback_ = std::move(cb); 
}

void Channel::setErrorCallback(EventCallback cb) 
{ 
    errorCallback_ = std::move(cb); 
}
    
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

void Channel::update() 
{
    loop_->update(this);
}

void Channel::remove() 
{
    loop_->remove(this);
}