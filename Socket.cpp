#include "Socket.h"

Socket::Socket(int sockfd)
    : sockfd_(sockfd)
{
}

Socket::~Socket()
{
    close(sockfd_);
}

int Socket::getFd() const{
    return sockfd_;
}

//绑定地址
void Socket::bindAddress(const Address &localaddr)
{
    //bind()函数通常用于将函数与参数绑定，生成一个新的可调用对象（function object）
    //在网络编程中，bind()函数通常用于将套接字（socket）与特定的IP地址和端口进行绑定。
    //int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    if (0 != bind(sockfd_, (sockaddr *)localaddr.getSockAddr(), sizeof(sockaddr_in)))
    {
        log<FATAL>("bind sockfd:%d fail\n", sockfd_);
    }
}

void Socket::listen()
{
    //监听队列长度设置为1024, 超过1024则不受理客户连接
    if (0 != ::listen(sockfd_, 1024))
    {
        log<FATAL>("listen sockfd:%d fail\n", sockfd_);
    }
}

int Socket::accept(Address *peeraddr)
{
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));
    
    //accept() 函数通常用于服务器端接受客户端的连接请求，并创建一个新的套接字用于与客户端进行通信。
    //int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
    int connfd = ::accept4(sockfd_, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

//关闭sockfd_的写端，关闭后不可以继续写数据，但是依然可以接受数据
void Socket::shutdownWrite()
{
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        log<ERROR>("shutdownWrite error");
    }
}

/**
 * TCP_NODELAY禁用Nagle算法，参考：https://zhuanlan.zhihu.com/p/80104656
 * Nagle算法的作用是减少小包的数量，原理是：当要发送的数据小于MSS时，数据就先不发送，而是先积累起来
 * Nagle算法优点：有效减少了小包数量，减小了网络负担
 * 缺点：由于要积累数据，会带来一定延迟，对于实时性要求较高的场景最好禁用Nagle算法
*/
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}