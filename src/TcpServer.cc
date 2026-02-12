#include <memory>
#include <functional>
#include <string>

#include "../include/TcpServer.h"
#include "../include/logger.h"
#include "../include/TcpConnection.h"

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, TcpServer::Option option)
    : loop_(CHECK_NOT_NULL(loop)),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop, name_)),
      connectionCallback_(),
      messageCallback_(),
      started_(0),
      nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
                                                  std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] - destructing";
    for(auto& item: connections_)
    {
        // 先创建一个强智能指针指向这个connection，然后再释放掉ConnectionMap的connection
        // 出了循环作用域后指针就会析构，解决了释放item后不能访问conn的问题
        TcpConnectionPtr conn(item.second);
        item.second.reset();

        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}


void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    // 防止TcpServer被多次启动
    if (started_++ == 0)
    {
        threadPool_->start();
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get())); // 启动acceptor_
    }
}

// TcpServer默认的链接传递方式
// 有一个新链接来了之后，acceptor就会执行这个回调
void TcpServer::newConnection(int sockfd, const InetAddress &addr)
{
    EventLoop *ioloop = threadPool_->getNextLoop();
    char buf[32];
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_; // 这个Connid不涉及线程安全问题
    std::string connName = name_ + buf;

    LOG_INFO << "TcpServer::newConnection [" << name_
             << "] - new connection [" << connName 
             << "] from " << addr.toIpPort();

    sockaddr_in localaddr;
    socklen_t addrlen = sizeof localaddr;
    if(::getsockname(sockfd, (sockaddr*)&localaddr, &addrlen) < 0)
    {
        LOG_ERROR << "getsockname error";
    }
    InetAddress local(localaddr);

    TcpConnectionPtr conn(
        new TcpConnection(
            ioloop,
            connName,
            sockfd,
            local,
            addr
        )
    );
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    // 这个是被动关闭执行的内容 shutdown是主动关闭！
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)
    );

    ioloop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    LOG_INFO << "TcpServer::removeConnection [" << name_
             << "] - connection " << conn->name();
    connections_.erase(conn->name());
    EventLoop *ioloop = conn->getLoop();
    ioloop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}