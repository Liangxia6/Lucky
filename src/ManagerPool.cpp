#include "ManagerPool.h"

ManagerPool::ManagerPool(EventLoop *baseloop, const std::string &name)
    : is_start_(false), baseLoop_(baseloop), name_(name), threads_num_(0), nextLoop_(0)
{
}

ManagerPool::~ManagerPool()
{
}

void ManagerPool::start(const ThreadInitCallback &cb)
{
    is_start_ = true;
    // 循环创建线程
    for (int i = 0; i < threads_num_; ++i)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        Manager *t = new Manager(cb, buf);
        // 加入此EventLoop和Thread入容器
        threads_.push_back(std::unique_ptr<Manager>(t));
        // 底层创建线程 绑定一个新的EventLoop 并返回该loop的地址
        // 此时已经开始执行新线程了
        loops_.push_back(t->startLoop());
    }
    // 整个服务端只有一个线程运行baseLoop
    if (threads_num_ == 0 && cb)
    {
        // 那么不用交给新线程去运行用户回调函数了
        cb(baseLoop_);
    }
}

// 如果工作在多线程中，baseLoop_会以轮询的方式选择一个subloop，以便后续将连接分发给这个subloop
EventLoop *ManagerPool::getNextLoop()
{
    // 如果只设置一个线程 也就是只有一个mainReactor 无subReactor
    // 那么轮询只有一个线程 getNextLoop()每次都返回当前的baseLoop_
    EventLoop *loop = baseLoop_;

    // 通过轮询获取下一个处理事件的loop
    // 如果没设置多线程数量，则不会进去，相当于直接返回baseLoop
    if (!loops_.empty())
    {
        loop = loops_[nextLoop_];
        ++nextLoop_;
        if (nextLoop_ >= loops_.size())
        {
            nextLoop_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop *> ManagerPool::getAllLoops()
{
    if (loops_.empty())
    {
        return std::vector<EventLoop *>(1, baseLoop_);
    }
    else
    {
        return loops_;
    }
}

void ManagerPool::setThread_num(int num)
{
    threads_num_ = num;
}

bool ManagerPool::isStart() const
{
    return is_start_;
}

const std::string ManagerPool::getName() const
{
    return name_;
}
