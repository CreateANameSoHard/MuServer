#include "../include/TcpServer.h"
#include "../include/TcpConnection.h"
#include "../include/EventLoop.h"
#include "../include/logger.h"
#include "../include/Callbacks.h"
#include "../include/Buffer.h"
#include <string>

class EchoServer
{
public:
    EchoServer(EventLoop *loop, const InetAddress &addr, const std::string &name)
        : loop_(loop),
          server_(loop, addr, name)
    {
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        server_.setThreadNum(2);
    }
    void start()
    {
        server_.start();
    }

private:
    

    void onConnection(const TcpConnectionPtr& conn)
    {
        if(conn->connected())
        {
            LOG_INFO << "EchoServer链接成功！对端为" << conn->peerAddress().toIpPort();
        }
        else
        {
            LOG_INFO << "EchoServer链接断开！对端为" << conn->peerAddress().toIpPort();
        }
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, TimeStamp time)
    {
        std::string data = buf->retrieveAllAsString();
        conn->send(data);
        conn->shutdown();
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    // ip:127.0.0.1 port:8000
    EchoServer server(&loop, InetAddress(8000), "EchoServer");
    server.start();
    loop.loop();
}