#pragma once

#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

#include "LuckyLog.h"
#include "Socket.h"
#include "Filed.h"
#include "EventLoop.h"
#include "Address.h"

class EventLoop;
class Address;

/**
 * @brief 接收器类
 *
 * 运行在baseLoop中
 * TcpServer发现Acceptor有一个新连接，则将此filed分发给一个subLoop
 */
class Acceptor
{

public:
    // 接收新连接的回调
    using NewConnectionCallback = std::function<void(int sockfd, const Address &)>;

    Acceptor(EventLoop *, const Address &, bool);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &);

    bool isListen() const;
    void listen();

private:
    void handleRead();

    bool is_listen_;

    Socket acceptSocket_;
    Filed acceptFiled_;
    EventLoop *loop_; // 主线程
    NewConnectionCallback newConnectionCallback_;
};