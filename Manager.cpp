#include "Manager.h"

Manager::Manager(const ThreadInitCallback &cb = ThreadInitCallback(),
            const std::string &name = std::string())
    : is_exit_(false)
    , loop_(nullptr)
    , thread_(std::bind(&Manager::threadFunc, this), name)
    , mutex_()
    , cond_()
    , callback_(cb){

}
Manager::~Manager(){
    is_exit_ = true;
    if (this,loop_ != nullptr){
        loop_->quit();
        thread_.join();
    }
}

EventLoop *Manager::startLoop(){
    thread_.start();
    EventLoop *loop = nullptr;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_ == nullptr){
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

void Manager::threadFunc(){
    EventLoop loop;

    if (callback_){
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}
