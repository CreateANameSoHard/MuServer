#pragma once
#include <functional>
#include <string>
#include <unordered_map>

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "StaticFile.h"

//目前仅支持get和post路由
class Router
{
public:
    using RouterHandler = std::function<void(const HttpRequest &, HttpResponse *)>;

    Router() = default;
    ~Router() = default;

    //注册相应方法
    void get(const std::string& uri, const RouterHandler& handler) { getRoutes_.emplace(uri, handler); }
    void post(const std::string& uri, const RouterHandler& handler) { postRoutes_.emplace(uri, handler); };
    void file(const std::string& uri, const StaticFile& file) 
    { 
        getRoutes_.emplace
        (
            uri, 
            std::bind(&Router::handleFile,this,file, std::placeholders::_1, std::placeholders::_2)
        ); 
    };

    RouterHandler match(const HttpRequest::Method method, const std::string& path);
    void handleRequest(const HttpRequest &, HttpResponse *);
    void handleFile(const StaticFile& file, const HttpRequest&, HttpResponse*);

private:
    std::unordered_map<std::string, RouterHandler> getRoutes_;
    std::unordered_map<std::string, RouterHandler> postRoutes_;
};