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
    this->is_exit_ = true;
    if (this,loop_ != nullptr){
        this->loop_->quit();
        this->thread_.join();
    }
}

EventLoop *Manager::startLoop(){
    this->thread_.start();
    EventLoop *loop = nullptr;

    {
        std::unique_lock<std::mutex> lock(this->mutex_);
        while(loop_ == nullptr){
            this->cond_.wait(lock);
        }
        loop = this->loop_;
    }
    return loop;
}

void Manager::threadFunc(){
    EventLoop loop;

    if (this->callback_){
        this->callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(this->mutex_);
        this->loop_ = &loop;
        this->cond_.notify_one();
    }

    loop.loop();
    std::unique_lock<std::mutex> lock(this->mutex_);
    this->loop_ = nullptr;
}
