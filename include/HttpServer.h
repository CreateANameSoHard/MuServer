#pragma once
#include <functional>

#include "TcpServer.h"
#include "TcpConnection.h"
#include "noncopyable.h"

class HttpRequest;
class HttpResponse;

class HttpServer : noncopyable
{
    public:
        using HttpCallback = std::function<void(const HttpRequest&, HttpResponse*)>;
        HttpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, TcpServer::Option option = TcpServer::Option::kNoReusePort);

        EventLoop* getLoop() const {return server_.getLoop();}

        void start();
        void setHttpCallback(HttpCallback cb){ callback_ = cb; }
        void setThreadNum(int numThreads){ server_.setThreadNum(numThreads); };

    private:
        void onConnection(TcpConnectionPtr);
        void onMessage(TcpConnectionPtr, Buffer*, TimeStamp);
        void onRequest(const TcpConnectionPtr&, const HttpRequest&);

        TcpServer server_;
        HttpCallback callback_; //用户提供的httpcallback
};