#include "ThreadPool.h"

ThreadPool::ThreadPool(EventLoop *baseloop, const std::string &name)
    : is_start_(false)
    , baseLoop_(baseloop)
    , name_(name)
    , threads_num_(0)
    , nextLoop_(0){

}

ThreadPool::~ThreadPool(){

}

void ThreadPool::setThread_num(int num){
    this->threads_num_ = num;
}

void ThreadPool::start(const ThreadInitCallback &cb = ThreadInitCallback()){
    this->is_start_ = true;
    for (int i = 0; i < threads_num_; ++i){
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        Manager *t = new Manager(cb, buf);
        this->threads_.push_back(std::unique_ptr<Manager>(t));
        this->loops_.push_back(t->startLoop());
    }
    if(this->threads_num_ == 0 && cb){
        cb(baseLoop_);
    }
}

EventLoop *ThreadPool::getNextLoop(){
    EventLoop *loop = baseLoop_;

    if(!loops_.empty()){
        loop = loops_[nextLoop_];
        ++nextLoop_;
        if(nextLoop_ >= this->loops_.size()){
            nextLoop_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop *> ThreadPool::getAllLoops(){
    if(this->loops_.empty()){
        return std::vector<EventLoop *>(1, baseLoop_);
    } else {
        return this->loops_;
    }
}

bool ThreadPool::is_start() const{
    return this->is_start_;
}

const std::string ThreadPool::getName() const{
    return this->name_;
}
