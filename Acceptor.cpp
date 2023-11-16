#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

#include "Acceptor.h"
#include "LuckyLog.h"

static int createNonblockFd(){
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0){
        log<FATAL>("%s:%s:%d listen socket create err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const Address &listenAddr, bool reuseport)
    : is_listen_(false)
    , acceptSocket_(createNonblockFd())
    , acceptChannel_(loop, acceptSocket_.getFd())
    , loop_(loop){
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback(
    std::bind(&Acceptor::HandleRead, this));
}

Acceptor::~Acceptor(){

}

void Acceptor::setNewConnectionCallback(const NewConnectionCallback &NewConnectionCallback){
    NewConnectionCallback_ = NewConnectionCallback;
}

bool Acceptor::is_listen() const{
    return is_listen_;
}

void Acceptor::listen(){
    is_listen_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::HandleRead(){
    Address peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0){
        if (NewConnectionCallback_){
            NewConnectionCallback_(connfd, peerAddr);
        } else {
            ::close(connfd);
        }
    } else {
        log<ERROR>("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE){
            log<ERROR>("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}
