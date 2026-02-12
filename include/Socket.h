#pragma once


#include "noncopyable.h"

class InetAddress;

// 对文件描述符的封装
class Socket:noncopyable
{
public:
    explicit Socket(int sockfd);
    ~Socket();

    void bindAddress(const InetAddress& addr);
    void listen();
    int accept(InetAddress* peeraddr);

    void shutdownWrite();
    void shutdownRead();

    // 关闭或开启tcp的nagle算法（这个算法可以减少数据包，但会增加延迟）
    void setTcpNoDelay(bool on);
    // 关闭或开启地址复用
    void setReuseAddr(bool on);
    // 关闭或开启端口复用
    void setReusePort(bool on);
    // 关闭或开启keep-alive
    void setKeepAlive(bool on);

    static int getSocketError(int sockfd);

    int fd() const { return sockfd_; }
private:
    const int sockfd_;
};