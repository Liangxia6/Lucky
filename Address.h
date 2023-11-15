#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

class Address{

public:

    Address(uint16_t = 0, std::string = "127.0.0.1");
    Address(const sockaddr_in &);
    ~Address() = default;

    std::string toIp() const;
    uint16_t toPort() const;  
    std::string toIpPort() const;

    const sockaddr_in *getSockAddr() const;
    void setSockAddr(const sockaddr_in &);

private:

    sockaddr_in addr_;

};