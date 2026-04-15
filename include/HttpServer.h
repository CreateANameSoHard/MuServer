#pragma once
#include <functional>

#include "TcpServer.h"
#include "TcpConnection.h"
#include "noncopyable.h"
#include "Router.h"

class HttpRequest;
class HttpResponse;

class HttpServer : noncopyable
{
public:
    using RouterHandler = std::function<void(const HttpRequest &, HttpResponse *)>;
    HttpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, TcpServer::Option option = TcpServer::Option::kNoReusePort);

    EventLoop *getLoop() const { return server_.getLoop(); }

    void start();
    // void setHttpCallback(HttpCallback cb){ callback_ = cb; }

    void get(const std::string &uri, const RouterHandler &handler) { router_.get(uri, handler); }
    void post(const std::string &uri, const RouterHandler &handler) { router_.post(uri, handler); }
    void file(const std::string& uri, const StaticFile &file) { router_.file(uri, file); }

    void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); };

private:
    void onConnection(TcpConnectionPtr);
    void onMessage(TcpConnectionPtr, Buffer *, TimeStamp);
    void onRequest(const TcpConnectionPtr &, const HttpRequest &);

    TcpServer server_;
    // HttpCallback callback_; //用户提供的httpcallback
    Router router_;
};