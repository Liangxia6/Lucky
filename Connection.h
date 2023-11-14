#pragma once

#include <memory>
#include <string>
#include <atomic>

#include "Addresss.h"
#include "TimeStamp.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Buffer.h"

class Connection : public std::enable_shared_from_this<Connection>{

public:

    using TcpConnectionPtr = std::shared_ptr<Connection>;

    using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
    using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr &, Buffer *, TimeStamp)>;
    Connection(EventLoop *, const std::string &, int , const Address &, const Address &);
    ~Connection();

    EventLoop *getLoop() const;
    const std::string &getName() const;
    const Address &localAddress() const;
    const Address &peerAddress() const;

    bool is_connect() const;

    void send(std::string &);
    void shutdown();

    void setConnectionCallback(const ConnectionCallback &cb);
    void setMessageCallback(const MessageCallback &cb);
    void setWriteCompleteCallback(const WriteCompleteCallback &cb);
    void setCloseCallback(const CloseCallback &cb);
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark);

    void connectEstablished();
    void connectDestroyed();
    
private:

    enum StateE{
        keyDisconnected,
        keyConnecting,  
        keyConnected,   
        keyDisconnecting
    };
    void setState(StateE state);

    void handleRead(TimeStamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void *data, size_t len);
    void shutdownInLoop();

    EventLoop *loop_;
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const Address localAddr_;
    const Address peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;

    Buffer inputBuffer_;
    Buffer outputBuffer_; 

};