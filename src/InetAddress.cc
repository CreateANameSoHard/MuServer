#include "../include/InetAddress.h"


InetAddress::InetAddress(uint16_t port, std::string ip)
{
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
}

std::string InetAddress::toIp() const
{
    char buf[INET_ADDRSTRLEN];
    //avoid namespace confilct add ::
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}

std::string InetAddress::toIpPort() const
{
    char buf[INET_ADDRSTRLEN];
    //avoid namespace confilct add ::
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    uint16_t port = ntohs(addr_.sin_port);
    return std::string(buf) + ":" + std::to_string(port);
}

uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}

// int main()
// {
//     InetAddress addr(8080, "123.255.255.10");

//     std::cout << addr.toIpPort() << std::endl;
//     std::cout << addr.toPort() << std::endl;
//     std:: cout << addr.toIp() << std::endl;
//     return 0;
// }