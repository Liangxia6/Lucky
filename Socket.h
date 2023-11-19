#pragma once

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include "LuckyLog.h"
#include "Address.h"

class Address;

/**
 * @brief Socket类
 * 
 * 封装socket fd
*/
class Socket
{
public:

    Socket(int);
    ~Socket();
    
    int getFd() const;
    //绑定
    void bindAddress(const Address &localaddr);
    //监听
    void listen();
    //接收
    int accept(Address *peeraddr);
    //(半)关闭
    void shutdownWrite();

    void setTcpNoDelay(bool on);    // 设置Nagel算法 
    void setReuseAddr(bool on);     // 设置地址复用
    void setReusePort(bool on);     // 设置端口复用
    void setKeepAlive(bool on);     // 设置长连接


private:

    const int sockfd_;

};