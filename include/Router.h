#pragma once
#include <functional>
#include <string>
#include <unordered_map>

#include "../include/HttpRequest.h"
#include "../include/HttpResponse.h"

//目前仅支持get和post路由
class Router
{
public:
    using RouterHandler = std::function<void(const HttpRequest &, HttpResponse *)>;

    Router() = default;
    ~Router() = default;

    //注册相应方法
    void get(const std::string& path, const RouterHandler& handler) { getRoutes_.emplace(path, handler); }
    void post(const std::string& path, const RouterHandler& handler) { postRoutes_.emplace(path, handler); };

    RouterHandler match(const HttpRequest::Method method, const std::string& path);
    void handleRequest(const HttpRequest &, HttpResponse *);
private:
    std::unordered_map<std::string, RouterHandler> getRoutes_;
    std::unordered_map<std::string, RouterHandler> postRoutes_;
};