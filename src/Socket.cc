#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <strings.h>
#include <errno.h>

#include "../include/Socket.h"
#include "../include/InetAddress.h"
#include "../include/logger.h"

Socket::Socket(int fd)
    : sockfd_(fd)
{
}

Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress &addr)
{
    // 源码写的是bindorDie函数
    int ret = ::bind(sockfd_, (sockaddr*)addr.getSockAddr(), (size_t)sizeof(sockaddr_in));
    if(ret < 0)
    {
        LOG_FATAL << "Socket::bindAddress";
    }
}
void Socket::listen()
{
    // 源码写的是listenOrDie函数
    // SOMAXCONN是最大连接数 1024
    if(::listen(sockfd_, SOMAXCONN) < 0)
    {
        LOG_FATAL << "Socket::listen";
    }
}
// 参数为传出参数
int Socket::accept(InetAddress *peeraddr)
{
    socklen_t addrlen = sizeof(sockaddr_in);
    sockaddr_in addr;
    bzero(&addr, sizeof addr);
    // accept的addrlen要初始化！
    // accept要设置为非阻塞
    int connfd = ::accept4(sockfd_, (sockaddr*)&addr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    if(::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR << "Socket::shutdownWrite";
    }
}
void Socket::shutdownRead()
{
    if(::shutdown(sockfd_, SHUT_RD) < 0)
    {
        LOG_ERROR << "Socket::shutdownRead";
    }
}

// 关闭或开启tcp的nagle算法（这个算法可以减少数据包，但会增加延迟）
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    if(::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)))
    {
        LOG_ERROR << "Socket::setTcpNoDelay";
    }
}
// 关闭或开启地址复用
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    if(::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)))
    {
        LOG_ERROR << "Socket::setReuseAddr";
    }
}
// 关闭或开启端口复用
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    if(::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)))
    {
        LOG_ERROR << "Socket::setReusePort";
    }
}
// 关闭或开启keep-alive
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    if(::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)))
    {
        LOG_ERROR << "Socket::setKeepAlive";
    }
}

int Socket::getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = sizeof(optval);
    if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        LOG_ERROR << "Socket::getSocketError";
        return errno;
    }
    else
    {
        return optval;
    }
}