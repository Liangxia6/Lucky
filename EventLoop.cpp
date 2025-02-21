#include "EventLoop.h"

// 独立于线程的指针,确保一个线程只有一个EventLoop
__thread EventLoop *t_LoopinThread = nullptr;

// epoll的超时时间
const int kEpollTime = 10000;

// 创建wakeupfd
int createEventfd()
{
    int efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (efd < 0)
    {
        LOG_FATAL("eventfd error:%d\n", errno);
    }
    return efd;
}

EventLoop::EventLoop()
    : looping_(false), quit_(false), threadId_(getThreadID::tid()), wakeupFd_(createEventfd()), epoller_(new Epoller(this)), wakeupFiled_(new Filed(this, wakeupFd_)), isCallPendFunc_(false)
{
    log<DEBUG>("EventLoop created %p in thread %d\n", this, threadId_);
    // 确保此线程上只有一个EventLoop
    if (t_LoopinThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d\n", t_LoopinThread, threadId_);
    }
    else
    {
        t_LoopinThread = this;
    }
    // 设置waupfd的回调函数并让epoll监听
    wakeupFiled_->setReadCallback(std::bind(&EventLoop::handelwakeupRead, this));
    wakeupFiled_->enableReading();
}

EventLoop::~EventLoop()
{
    // 撤销wakeupfiled
    wakeupFiled_->disableAll();
    wakeupFiled_->remove();
    close(wakeupFd_);
    t_LoopinThread = nullptr;
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFOM("EventLoop %p start looping\n", this);

    while (!quit_)
    {
        // 初始化activeFileds列表
        activeFileds_.clear();
        // 开启epoll_wait,填充activeFileds列表
        epollReturnTime_ = epoller_->epoll(
            kEpollTime, &activeFileds_);
        // 逐一处理上层回调
        for (Filed *filed : activeFileds_)
        {
            filed->handleEvent(epollReturnTime_);
        }
        /**
         * 执行当前EventLoop事件循环需要处理的回调操作 对于线程数 >=2 的情况 IO线程 mainloop(mainReactor) 主要工作：
         * accept接收连接 => 将accept返回的connfd打包为Filed => TcpServer::newConnection通过轮询将TcpConnection对象分配给subloop处理
         * mainloop调用queueInLoop将回调加入subloop的pendingFunctors_中（该回调需要subloop执行 但subloop还在epoller_->poll处阻塞）
         * queueInLoop通过wakeup将subloop唤醒，此时subloop就可以执行pendingFunctors_中的保存的函数了
         **/
        // 执行其他线程添加到pendingFunctors_中的函数
        doPendFunc();
    }

    LOG_INFOM("EventLoop %p stop looping.\n", this);
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    // 如果loop阻塞在epoll_wait,像wakeupFd写入数据,唤醒epoll从而退出loop
    if (!isLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runinLoop(Callback cb)
{
    // 在目标线程,直接执行回调,否则加入等待队列
    if (isLoopThread())
    {
        cb();
    }
    else
    {
        queueinLoop(cb);
    }
}

// 把cb放入队列,唤醒loop所在的线程执行cb
void EventLoop::queueinLoop(Callback cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendFuncs_.emplace_back(cb);
    }
    // 唤醒后才能向pendFuncs_添加新的回调
    if (!isLoopThread() || isCallPendFunc_)
    {
        wakeup();
    }
}

// wakeup() 的过程本质上是对wakeupFd_进行写操作，以触发该wakeupFd_上的可读事件
void EventLoop::wakeup()
{
    uint64_t u64one = 1;
    ssize_t n = write(wakeupFd_, &u64one, sizeof(u64one));

    if (n != sizeof(u64one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
    }
}

// 回调函数,持续wakeup
void EventLoop::handelwakeupRead()
{
    uint64_t u64one = 1;
    ssize_t n = write(wakeupFd_, &u64one, sizeof(u64one));

    if (n != sizeof(u64one))
    {
        LOG_ERROR("EventLoop::HandleRead() writes %lu bytes instead of 8\n", n);
    }
}

/**
 * 执行当前EventLoop事件循环需要处理的回调操作 对于线程数 >=2 的情况 IO线程 mainloop(mainReactor) 主要工作：
 * accept接收连接 => 将accept返回的connfd打包为Filed => TcpServer::newConnection通过轮询将TcpConnection对象分配给subloop处理
 * mainloop调用queueInLoop将回调加入subloop的pendingFunctors_中（该回调需要subloop执行 但subloop还在epoller_->poll处阻塞）
 * queueInLoop通过wakeup将subloop唤醒，此时subloop就可以执行pendingFunctors_中的保存的函数了
 **/
// 执行其他线程添加到pendingFunctors_中的函数
void EventLoop::doPendFunc()
{
    std::vector<Callback> functors;
    isCallPendFunc_ = true;
    {
        // 交换的方式减少了锁的临界区范围 提升效率 同时避免了死锁
        // 如果执行functor()在临界区内 且functor()中调用queueInLoop()就会产生死锁
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendFuncs_);
    }
    // 逐一执行PandFuncs中的函数
    for (const Callback &functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调操作
    }
    isCallPendFunc_ = false;
}

bool EventLoop::isLoopThread() const
{
    return threadId_ == getThreadID::tid();
}

bool EventLoop::hasFiled(Filed *filed)
{
    epoller_->hasFiled(filed);
}

TimeStamp EventLoop::epollReturnTime() const
{
    return epollReturnTime_;
}

void EventLoop::update(Filed *filed)
{
    epoller_->update(filed);
}

void EventLoop::remove(Filed *filed)
{
    epoller_->remove(filed);
}
