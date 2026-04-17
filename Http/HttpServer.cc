#include "../include/HttpServer.h"
#include "../include/logger.h"
#include "../include/HttpResponse.h"
#include "../include/HttpContext.h"

void HttpServer::start()
{
    LOG_INFO << "HttpServer[" << server_.name() << "] listening on " << server_.ipPort();
    server_.start();
}


HttpServer::HttpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, TcpServer::Option option)
    : server_(loop, listenAddr, nameArg, option),
      router_()
{
    server_.setConnectionCallback(std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}
// 连接建立后 创建上下文存储解析进度
void HttpServer::onConnection(TcpConnectionPtr conn)
{
    if (conn->connected())
    {
        conn->setContext(HttpContext());
    }
}

void HttpServer::onMessage(TcpConnectionPtr conn, Buffer *buffer, TimeStamp receivedTime)
{
    HttpContext &context = conn->getMutableContext().as<HttpContext>();
    if (!context.parseRequest(buffer, receivedTime))
    {
        LOG_INFO << "parseRequest failed!";
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
    }
    if (context.gotAll())
    {
        LOG_INFO << "calling onRequest ";
        onRequest(conn, context.request());
        context.reset();
    }
}

void HttpServer::onRequest(const TcpConnectionPtr &conn, const HttpRequest &request)
{
    const std::string &connection = request.getHeader("Connection");
    bool close = connection == "close" || (request.version() == HttpRequest::Version::kHttp10 && connection != "Keep-Alive");
    HttpResponse resp(close);

    LOG_INFO << "calling httpcallback ";
    // callback_(request, &resp); // 用户提供的回调
    router_.handleRequest(request, &resp);

    if (resp.isStaticFile())
    {
        //TODO:对象池优化
        Buffer header;
        resp.appendHeaderToBuffer(&header);
        conn->send(&header);
        conn->sendFile(*resp.getFileptr());
    }
    else
    {
        //TODO:对象池优化
        Buffer buffer;
        resp.appendToBuffer(&buffer);
        conn->send(&buffer);
    }

    if (resp.closeConnection())
    {
        conn->shutdown();
    }
}