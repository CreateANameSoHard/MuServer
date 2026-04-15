#include <unistd.h>
#include <fcntl.h>

#include "../include/Router.h"
#include "../include/logger.h"

Router::RouterHandler Router::match(const HttpRequest::Method method, const std::string &path)
{
    using Method = HttpRequest::Method;
    switch (method)
    {
    case Method::kGet:
    {
        auto it = getRoutes_.find(path);
        return it == getRoutes_.end() ? nullptr : it->second;
        break;
    }
    case Method::kPost:
    {
        auto it = postRoutes_.find(path);
        return it == postRoutes_.end() ? nullptr : it->second;
    }
    case Method::kDelete:
        break;
    case Method::kPut:
        break;
    case Method::kHead:
        break;
    default:
        LOG_ERROR << "method dismatch!";
        break;
    }
    return nullptr;
}

void Router::handleRequest(const HttpRequest &req, HttpResponse *rep)
{
    RouterHandler handler = match(req.method(), req.path());
    if (handler)
    {
        handler(req, rep);
    }
    else
    {
        rep->setStatusCode(HttpResponse::HttpStateCode::k404NotFound);
        rep->setStatusMessage("Not Found");
        rep->setCloseConnection(true);
    }
}


void Router::handleFile(const StaticFile& file, const HttpRequest& req, HttpResponse* rep)
{
        rep->setContentType(file.getTypeString());
        rep->enableStaticFile(file);
}
