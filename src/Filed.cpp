#include "Filed.h"

const int Filed::kNoneEvent = 0;
const int Filed::kReadEvent = EPOLLIN | EPOLLPRI;
const int Filed::kWriteEvent = EPOLLOUT;

Filed::Filed(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1) // fd还未被Epoller监视
      ,
      tied_(0)
{
}

Filed::~Filed()
{
}

void Filed::handleEvent(TimeStamp receiveTime)
{
    /**
     * Connection::connectEstablished会调用filed_->tie(shared_from_this());
     * 调用了Filed::tie会设置tid_=true
     * 所以对于TcpConnection::filed_ 需要多一份强引用的保证以免用户误删Connection对象
     */
    if (tied_)
    {
        // 变成shared_ptr增加引用计数，防止误删
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
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
            // 如果提升失败了 就不做任何处理 说明Filed的Connection对象已经不存在了
        }
    }
    else
    {
        using Callback = std::function<void()>;
        std::array<Callback, 4> callbacks;

        // 将回调函数存储到函数指针数组中
        callbacks[0] = closeCallback_;
        callbacks[1] = errorCallback_;
        callbacks[2] = std::bind(readCallback_, receiveTime);
        callbacks[3] = writeCallback_;

        LOG_INFOM("filed handleEvent revents:%d\n", revents_);

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
}

// void Filed::handleEventGuard(TimeStamp receiveTime)
// {
//     LOG_INFOM("filed handleEvent revents:%d\n", revents_);
//     // 关闭
//     if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) // 当Connection对应Filed 通过shutdown 关闭写端 epoll触发EPOLLHUP
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

void Filed::enableReading()
{
    events_ |= kReadEvent;
    update();
}

void Filed::disableReading()
{
    events_ &= ~kReadEvent;
    update();
}

void Filed::enableWriting()
{
    events_ |= kWriteEvent;
    update();
}

void Filed::disableWriting()
{
    events_ &= ~kWriteEvent;
    update();
}

void Filed::disableAll()
{
    events_ = kNoneEvent;
    update();
}

bool Filed::isNoneEvent() const
{
    return events_ == kNoneEvent;
}

bool Filed::isWriting() const
{
    return events_ & kWriteEvent;
}

bool Filed::isReading() const
{
    return events_ & kReadEvent;
}

int Filed::getFd() const
{
    return fd_;
}

int Filed::getEvents() const
{
    return events_;
}

void Filed::setRevents(int revent)
{
    revents_ = revent;
}

int Filed::getIndex() const
{
    return index_;
}

void Filed::setIndex(int index)
{
    index_ = index;
}

EventLoop *Filed::getLoop()
{
    return loop_;
}

void Filed::setReadCallback(ReadEventCallback cb)
{
    readCallback_ = std::move(cb);
}
void Filed::setWriteCallback(EventCallback cb)
{
    writeCallback_ = std::move(cb);
}

void Filed::setCloseCallback(EventCallback cb)
{
    closeCallback_ = std::move(cb);
}

void Filed::setErrorCallback(EventCallback cb)
{
    errorCallback_ = std::move(cb);
}

void Filed::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

void Filed::update()
{
    loop_->update(this);
}

void Filed::remove()
{
    loop_->remove(this);
}