#include "Thread.h"

std::atomic_int Thread::create_num_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    : is_start_(false)
    , is_join_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
{
    setName();
}

Thread::~Thread()
{
    if (is_start_ && !is_join_)
    {
        //主线程不再等待此线程,成为守护进程
        thread_->detach();
    }
}

void Thread::start()
{
    is_start_ = true;
    //信号量用于保证线程创建成功后start函数再运行结束
    sem_t sem;
    sem_init(&sem, false, 0);

    //创建一个新的线程，并使用lambda表达式作为线程函数
    thread_ = std::shared_ptr<std::thread>( 
        //使用智能指针管理线程资源
        new std::thread([&]() 
        {  
            tid_ = getThreadID::tid(); 
            //增加信号量的值，使等待的线程可以继续执行
            sem_post(&sem);
            //调用线程要执行的函数
            func_(); 
        })
    );
    //等待信号量
    sem_wait(&sem);
}

void Thread::join()
{
    is_join_ = true;
    //等待线程执行完毕
    thread_->join();
}

bool Thread::isStart()
{
    return is_start_;
}

pid_t Thread::getTid() const
{
    return tid_;
}

void Thread::setName()
{
    int num = ++create_num_;
    if (name_.empty())
    {
        char buff[32] = {};
        //将格式化字符串 "Thread%d" 和 num 转换为字符，并存储到 buff 中
        snprintf(buff, sizeof(buff), "Thread%d", num);
        name_ = buff;
    }
}

const std::string &Thread::getName() const
{
    return name_;
}

int Thread::getCreateNum()
{
    return Thread::create_num_;
}

