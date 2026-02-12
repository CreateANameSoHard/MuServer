#include <netinet/tcp.h>
#include <functional>
#include <unistd.h>

#include "../include/Acceptor.h"
#include "../include/logger.h"

// 创建非阻塞套接字。老版本是在创建套接字后用fcntl，但新版本可以直接用socket创建了
static int createNonblockingOrDie()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if(sockfd < 0)
    {
        LOG_FATAL << "create socket error";
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reusePort)
    : loop_(loop), // 通过这个loop指针，把functor传给Eventloop，让loop执行functor
      acceptSocket_(createNonblockingOrDie()),
      acceptChannel_(loop, acceptSocket_.fd()),
      listening_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reusePort);
    acceptSocket_.bindAddress(listenAddr);
    // tcp_server.start()=> acceptor.listen() 当有新事件发生时，Acceptor会执行一个回调（connfd打包成channel，交给subloop）
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this)); // 当有新连接到来时，执行handleRead函数
}
Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}


// Channel注册读事件。 把Acceptor放到baseloop上，就会监听链接事件（还没有注册）
void Acceptor::listen()
{
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading(); // 开启监听事件
}

// listenfd有事件发生了，就是有新用户链接了。把链接打包成channel，交给subReactor(具体的回调是由TcpServer提供的，这里只是负责执行)
void Acceptor::handleRead()
{
    InetAddress peeraddr;
    int connfd = acceptSocket_.accept(&peeraddr); // 接受新连接
    if(connfd >= 0)
    {
        if(newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peeraddr); // 调用用户提供的回调函数处理新连接
        }
        else
        {
            ::close(connfd); // 如果没有回调函数，关闭连接
        }
    }
    else
    {
        LOG_ERROR << "accept error";
        // 线程的打开文件描述符达到上限
        if(errno == EMFILE)
        {
            LOG_ERROR << "accept error: too many open files";
        }
    }
}