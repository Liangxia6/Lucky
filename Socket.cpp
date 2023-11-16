#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include "Socket.h"
#include "LuckyLog.h"

Socket::Socket(int sockfd)
    : sockfd_(sockfd){
}

Socket::~Socket(){
    ::close(sockfd_);
}

int Socket::getFd() const{
    return sockfd_;
}

void Socket::bindAddress(const Address &localaddr){
    if (0 != ::bind(sockfd_, 
        (sockaddr *)localaddr.getSockAddr(), 
        sizeof(sockaddr_in))){
        log<FATAL>("bind sockfd:%d fail\n", sockfd_);
    }
}

void Socket::listen(){
    if (0 != ::listen(sockfd_, 1024)){
        log<FATAL>("listen sockfd:%d fail\n", sockfd_);
    }
}

int Socket::accept(Address *peeraddr){
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));
    
    int connfd = ::accept4(sockfd_, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0){
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite(){
    if (::shutdown(sockfd_, SHUT_WR) < 0){
        log<ERROR>("shutdownWrite error");
    }
}

void Socket::setTcpNoDelay(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}

void Socket::setReuseAddr(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}

void Socket::setReusePort(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}

void Socket::setKeepAlive(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}