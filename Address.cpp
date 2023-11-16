#include "Address.h"

Address::Address(uint16_t port, std::string ip)
{
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

Address::Address(const sockaddr_in &addr)
    : addr_(addr)
{
}


std::string Address::toIp() const
{
    char buff[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr, buff, sizeof(buff));
    return buff;
}

uint16_t Address::toPort() const
{
    uint16_t temp = 0;
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

const sockaddr_in *Address::getSockAddr() const
{
    return &addr_;
}

void Address::setSockAddr(const sockaddr_in &addr)
{
    addr_ = addr;
}
