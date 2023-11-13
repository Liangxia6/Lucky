#pragma once

#include <functional>

#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Addresss.h"

class Acceptor{

public:

    using NewConnectionCallback = std::function<void(int sockfd, const Address &)>;
    
    Acceptor(EventLoop *, const Address &, bool);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &);

    bool is_listen() const;
    void listen();

private:

    void HandleRead();

    bool is_listen_;

    Socket acceptSocket_;
    Channel acceptChannel_;
    EventLoop *loop_; 
    NewConnectionCallback NewConnectionCallback_;

};