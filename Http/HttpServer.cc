#include <unistd.h>

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
      router_(),
      fileCache_(6, 3)
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

    router_.handleRequest(request, &resp);

    if (resp.isStaticFile())
    {
        const StaticFile &sf = *resp.getFileptr();
        std::string path = sf.path();
        const size_t fileSize = sf.getFileSize();
        //small file
        if (fileSize <= MAX_CHACHE_FILE_SIZE)
        {
            auto content = fileCache_.get(path);
            // hit
            if (content)
            {
                Buffer buffer;
                resp.setBody(std::string(content->data(), content->size()));
                resp.appendToBuffer(&buffer);
                conn->send(&buffer);
            }
            // not hit
            else
            {
                LOG_INFO << "not hit, reload cache ";
                Buffer header;
                resp.appendHeaderToBuffer(&header);
                conn->send(&header);
                conn->sendFile(*resp.getFileptr());

                // TODO:改为线程池写入
                std::vector<char> data(fileSize);
                int fd = ::open(path.c_str(), O_RDONLY);
                if (fd >= 0)
                {
                    ::read(fd, data.data(), fileSize);
                    ::close(fd);
                    fileCache_.put(path, std::make_shared<std::vector<char>>(std::move(data)));
                }
            }
        }
        //big file
        else
        {
            Buffer header;
            resp.appendHeaderToBuffer(&header);
            conn->send(&header);
            conn->sendFile(*resp.getFileptr());
        }
    }
    else
    {
        // TODO:对象池优化
        Buffer buffer;
        resp.appendToBuffer(&buffer);
        conn->send(&buffer);
    }

    if (resp.closeConnection())
    {
        conn->shutdown();
    }
}