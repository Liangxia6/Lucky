#pragma once

#include <memory>
#include <string>
#include <cstring>
#include <atomic>
#include <functional>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "LuckyLog.h"
#include "Address.h"
#include "TimeStamp.h"
#include "Filed.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Buffer.h"

class Filed;
class EventLoop;
class Socket;

/**
 *
 */
class Connection : public std::enable_shared_from_this<Connection>
{
public:
    using ConnectionPtr = std::shared_ptr<Connection>;
    using ConnectionCallback = std::function<void(const ConnectionPtr &)>;
    using CloseCallback = std::function<void(const ConnectionPtr &)>;
    using WriteCompleteCallback = std::function<void(const ConnectionPtr &)>;
    using HighWaterMarkCallback = std::function<void(const ConnectionPtr &, size_t)>;
    using MessageCallback = std::function<void(const ConnectionPtr &, Buffer *, TimeStamp)>;

    Connection(EventLoop *, const std::string &, int, const Address &, const Address &);
    ~Connection();

    EventLoop *getLoop() const;
    const std::string &getName() const;
    const Address &getLocalAddress() const;
    const Address &getPeerAddress() const;

    bool isConnect() const;

    // 发送数据
    void send(std::string &);
    void send(Buffer *buf);

    // 关闭连接
    void shutdown();

    // 设置用户回调函数(用户自定义)
    void setConnectionCallback(const ConnectionCallback &cb);
    void setMessageCallback(const MessageCallback &cb);
    void setWriteCompleteCallback(const WriteCompleteCallback &cb);
    void setCloseCallback(const CloseCallback &cb);
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark);

    // 服务于Server类
    void connectEstablished();
    void connectDestroyed();

private:
    enum StateE
    {
        kDisconnected, // 已经断开连接
        kConnecting,   // 正在连接
        kConnected,    // 已连接
        kDisconnecting // 正在断开连接
    };
    // 设置连接状态
    void setState(StateE);

    // 注册到filed上有事件发生时,回调下类函数
    void handleRead(TimeStamp);
    void handleWrite();
    void handleClose();
    void handleError();

    // 在自己的EventLoop中发送数据
    void sendinLoop(const void *, size_t);
    // 在自己的EventLoop中关闭连接
    void shutdownInLoop();

    EventLoop *loop_; // 属于哪个subLoop（如果是单线程则为baseLoop）
    const std::string name_;
    std::atomic_int state_; // 连接状态
    bool reading_;

    std::unique_ptr<Socket> socket_; // 把fd封装成socket，这样便于socket析构时自动关闭fd
    std::unique_ptr<Filed> filed_;   // fd对应的filed

    const Address localAddr_; // 本服务器地址
    const Address peerAddr_;  // 对端地址

    /**
     * 用户自定义的这些事件的处理函数，然后传递给 Server
     * Server 再在创建 Connection 对象时候设置这些回调函数到Connection中
     */
    ConnectionCallback connectionCallback_;       // 有新连接时的回调
    MessageCallback messageCallback_;             // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成以后的回调
    CloseCallback closeCallback_;                 // 客户端关闭连接的回调
    HighWaterMarkCallback highWaterMarkCallback_; // 超出水位实现的回调
    size_t highWaterMark_;

    // 读写缓冲区
    Buffer inputBuffer_;
    Buffer outputBuffer_;
};