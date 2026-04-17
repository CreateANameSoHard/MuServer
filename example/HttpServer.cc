#include "../include/HttpServer.h"
#include "../include/HttpRequest.h"
#include "../include/HttpResponse.h"
#include "../include/EventLoop.h"
#include "../include/MYSQLConnectionPool.h"

#include <iostream>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <unordered_map>
#include <string>
#include <sys/stat.h>

// 简单解析 x-www-form-urlencoded
std::unordered_map<std::string, std::string> parseForm(const std::string &body)
{
    std::unordered_map<std::string, std::string> result;

    size_t pos = 0;
    while (pos < body.size())
    {
        size_t eq = body.find('=', pos);
        size_t amp = body.find('&', pos);

        std::string key = body.substr(pos, eq - pos);
        std::string val = body.substr(eq + 1, amp - eq - 1);

        result[key] = val;

        if (amp == std::string::npos)
            break;
        pos = amp + 1;
    }

    return result;
}

void login(const HttpRequest &req, HttpResponse *resp)
{
    auto form = parseForm(req.body());

    std::string username = form["username"];
    std::string password = form["password"];

    try
    {
        MySQLConnectionPool& pool = MySQLConnectionPool::getInstance();
        auto conn = pool.getConnection();

        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement(
                "SELECT passwd_hash FROM user WHERE username=?"));

        stmt->setString(1, username);

        std::unique_ptr<sql::ResultSet> resSet(stmt->executeQuery());

        if (!resSet->next())
        {
            resp->setStatusCode(HttpResponse::HttpStateCode::k400BadRequest);
            resp->setStatusMessage("Bad Request");
            resp->setContentType("text/plain");
            resp->setBody("用户不存在");
            return;
        }

        std::string db_hash = resSet->getString("passwd_hash");

        if (password == db_hash)
        {
            // ✅ 重定向到 index 页面
            resp->setStatusCode(HttpResponse::HttpStateCode::k302Found);
            resp->setStatusMessage("Found");
            resp->addHeader("Location", "/index");
        }
        else
        {
            resp->setStatusCode(HttpResponse::HttpStateCode::k200Ok);
            resp->setStatusMessage("OK");
            resp->setContentType("text/plain");
            resp->setBody("密码错误");
        }
    }
    catch (std::exception &e)
    {
        resp->setStatusCode(HttpResponse::HttpStateCode::k500InternalServerError);
        resp->setStatusMessage("Internal Server Error");
        resp->setContentType("text/plain");
        resp->setBody("服务器错误");
    }
}

int main()
{
    InetAddress localaddr(8000); // 8000端口
    EventLoop loop;
    HttpServer server(&loop, localaddr, "dummy", TcpServer::kReusePort);

    MySQLConnectionPool& pool = MySQLConnectionPool::getInstance();
    pool.init("tcp://127.0.0.1:3306", "root", "tanghao", "MuServer");

    struct stat buf1;
    struct stat buf2;
    struct stat buf3;
    struct stat buf4;
    struct stat buf5;
    stat("../resource/index.html", &buf1);
    stat("../resource/logo.png", &buf2);
    stat("../resource/Login.html", &buf3);
    stat("../resource/video.mp4", &buf4);
    stat("../resource/file.pdf", &buf5);

    StaticFile index("../resource/index.html", 0, buf1.st_size);
    StaticFile logo("../resource/logo.png", 0, buf2.st_size, StaticFile::FileType::kimage_png);
    StaticFile Login("../resource/Login.html", 0, buf3.st_size);
    StaticFile video("../resource/video.mp4", 0, buf4.st_size, StaticFile::FileType::kvideo_mp4);
    StaticFile file("../resource/file.pdf", 0, buf5.st_size, StaticFile::FileType::kapplication_pdf);

    server.file("/index", index);
    server.file("/login", Login);
    server.file("/image", logo);
    server.file("/video", video);
    server.file("/file", file);

    server.post("/login", login);

    server.setThreadNum(0);
    server.start();
    loop.loop();
}
