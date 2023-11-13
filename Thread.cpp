#include <semaphore.h>
#include <thread>
#include <memory>
#include <unistd.h>

#include "Thread.h"
#include "getThreadID.h"

std::atomic_int Thread::create_num_(0);

    // bool is_start_;
    // bool is_join_;
    
    // std::shared_ptr<std::thread> thread_;
    // pid_t tid_;
    // std::string name_;
    // ThreadFunc func_;

Thread::Thread(ThreadFunc func, const std::string &name)
    : is_start_(false)
    , is_join_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name){
    this->setName();
}

Thread::~Thread(){
    if (is_start_ && !is_join_){
        this->thread_->detach();
    }
}

void Thread::start(){
    this->is_start_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);

    this->thread_ = std::shared_ptr<std::thread>(
        new std::thread([&]() {
        this->tid_ = getThreadID::tid();            
        sem_post(&sem);
        this->func_();
        })
    );
    sem_wait(&sem);
}

void Thread::join(){
    this->is_join_ = true;
    this->thread_->join();
}

bool Thread::isStart(){
    return this->is_start_;
}

pid_t Thread::getTid() const{
    return this->tid_;
}

void Thread::setName(){
    int num = ++create_num_;
    if (this->name_.empty()){
        char buff[32] = {};
        snprintf(buff, sizeof(buff), "Thread%d", num);
    }
}

const std::string &Thread::getName() const{
    return this->name_;
}

int Thread::getCreate_num(){
    return Thread::create_num_;
}

