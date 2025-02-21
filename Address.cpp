#include "Address.h"

Address::Address(uint16_t port, std::string ip)
{
    memset(&addr_, 0, sizeof(addr_));
    /*  AF_INET：IPv4 地址家族，表示 IPv4 地址。
        AF_INET6：IPv6 地址家族，表示 IPv6 地址。
        AF_UNIX：Unix 域套接字，用于本地通信。*/
    addr_.sin_family = AF_INET;
    // 将主机字节序的无符号短整型（16位）转换为网络字节序
    addr_.sin_port = htons(port);
    // 将点分十进制格式的 IPv4 地址转换为网络字节序的32位无符号整数
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

Address::Address(const sockaddr_in &addr)
    : addr_(addr)
{
}

std::string Address::toIp() const
{
    char buff[64] = {0};
    // 将网络字节序的 IP 地址转换为可读的字符串格式，并存储到提供的缓冲区中
    inet_ntop(AF_INET, &addr_.sin_addr, buff, sizeof(buff));
    return buff;
}

uint16_t Address::toPort() const
{
    uint16_t temp = 0;
    // 将网络字节序的 16 位无符号整数转换为主机字节序的 16 位无符号整数
    temp = ntohs(addr_.sin_port);
    return temp;
}

std::string Address::toIpPort() const
{
    char buff[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr, buff, sizeof buff);
    size_t end = strlen(buff);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buff + end, ":%u", port);
    return buff;
}

// const sockaddr_in *Address::getSockAddr() const
// {
//     return &addr_;
// }

// void Address::setSockAddr(const sockaddr_in &addr)
// {
//     addr_ = addr;
// }

// int main()
// {
//     Address addr(8080);
//     std::string a = addr.toIpPort();
//     std::cout << a;
// }