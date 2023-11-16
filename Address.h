#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <cstring>


class Address
{
public:

    Address(uint16_t = 0, std::string = "127.0.0.1");
    Address(const sockaddr_in &);
    ~Address() = default;

    //转成ip字符串
    std::string toIp() const;
    //转成port的int
    uint16_t toPort() const;
    //转成ipport字符串
    std::string toIpPort() const;

    const sockaddr_in *getSockAddr() const;
    void setSockAddr(const sockaddr_in &);

private:

// struct sockaddr_in {
//     short int sin_family;        // 地址族，通常为 AF_INET
//     unsigned short int sin_port; // 端口号
//     struct in_addr sin_addr;     // IP 地址
//     unsigned char sin_zero[8];   // 填充字段，通常置零
// };
    sockaddr_in addr_;

};