#pragma once

#include "Address.h"

class Address;

class Socket{

public:

    Socket(int);
    ~Socket();
    
    int getFd() const;
    void bindAddress(const Address &localaddr);
    void listen();
    int accept(Address *peeraddr);

    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);


private:

    const int sockfd_;

};