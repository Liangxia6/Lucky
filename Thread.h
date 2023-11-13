#pragma once

#include <functional>
#include <string>
#include <atomic>

class Thread{

public:

    using ThreadFunc = std::function<void()>;

    Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();

    bool isStart();
    pid_t getTid() const;
    void setName();
    const std::string &getName() const;

    static int getCreate_num();

private:

    bool is_start_;
    bool is_join_;
    
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    std::string name_;
    ThreadFunc func_;

    static std::atomic_int create_num_;

};