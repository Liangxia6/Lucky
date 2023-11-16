#pragma once 

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

#include "EventLoop.h"
#include "Acceptor.h"
#include "Address.h"
#include "ThreadPool.h"
#include "Connection.h"
#include "Manager.h"
#include "Buffer.h"

class Server{

public:

    using ConnectionPtr = std::shared_ptr<Connection>;
    using ConnectionCallback = std::function<void(const ConnectionPtr &)>;
    using CloseCallback = std::function<void(const ConnectionPtr &)>;
    using WriteCompleteCallback = std::function<void(const ConnectionPtr &)>;
    using HighWaterMarkCallback = std::function<void(const ConnectionPtr &, size_t)>;
    using MessageCallback = std::function<void(const ConnectionPtr &, Buffer *, TimeStamp)>;

    using ThreadInitCallback = std::function<void(EventLoop *)>;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    Server(EventLoop *loop,
            const Address &listenAddr,
            const std::string &nameArg,
            Option option = kNoReusePort);
    ~Server();

    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    void setThreadNum(int numThreads);

    void start();

private:

    void newConnection(int sockfd, const Address &peerAddr);
    void removeConnection(const ConnectionPtr &conn);
    void removeConnectionInLoop(const ConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, ConnectionPtr>;

    EventLoop *loop_;

    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_; 
    std::shared_ptr<ThreadPool> threadPool_; 

    ConnectionCallback connectionCallback_;      
    MessageCallback messageCallback_;            
    WriteCompleteCallback writeCompleteCallback_;

    ThreadInitCallback threadInitCallback_;

    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_;
};