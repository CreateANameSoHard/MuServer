#include "../include/HttpServer.h"
#include "../include/HttpRequest.h"
#include "../include/HttpResponse.h"
#include "../include/EventLoop.h"
#include "../include/logger.h"
#include <iostream>
#include <unordered_map>
#include <string>

void onRequest(const HttpRequest &req, HttpResponse *resp)
{
    LOG_INFO << "calling onRequest in User file ";
    std::unordered_map<std::string, std::string> headers = req.headers();
    for (auto &header : headers)
    {
        std::cout << "请求字段: " << header.first << " " << "请求值：" << header.second << std::endl;
    }

    resp->setStatusCode(HttpResponse::HttpStateCode::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("text/html");
    resp->addHeader("Server", "TinyMuduo");
    std::string now = TimeStamp::now().toString();
    std::string content =
        "<html><head><title>This is title</title></head>"
        "<body><h1>Hello</h1>Now is " +
        now +
        "</body></html>";
    resp->setBody(content);
}

void hello(const HttpRequest& req, HttpResponse* resp)
{
     resp->setStatusCode(HttpResponse::HttpStateCode::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("text/plain");
    resp->addHeader("Server", "TinyMuduo");

    std::string content = "hello world!";
    resp->setBody(content);
}

int main()
{
    InetAddress localaddr(8000); // 8000端口
    EventLoop loop;
    HttpServer server(&loop, localaddr, "dummy", TcpServer::kReusePort);
    
    server.get("/", onRequest); 
    server.get("/hello", hello);

    server.setThreadNum(0);
    server.start();
    loop.loop();
}
