#pragma once
#include "copyable.h"

#include <string>
#include <unordered_map>

class Buffer;
class HttpResponse : public copyable
{
public:
    enum class HttpStateCode
    {
        kUnknown = -1,
        k200Ok = 200,
        k301MovePermanently = 301,
        k400BadRequest = 400,
        k404NotFound = 404
    };
    explicit HttpResponse(bool close)
        : stateCode_(HttpStateCode::kUnknown),
            closeConnection_(close)
    {}

    void setStatusCode(HttpStateCode code) { stateCode_ = code;}
    void setStatusMessage(const std::string& msg) { statusMessage_ = std::move(msg); }
    void setBody(const std::string& msg) { body_ = std::move(msg); }

    bool closeConnection() const { return closeConnection_; }
    void setCloseConnection(bool close) { closeConnection_ = close; }

    void setContentType(const std::string& contentType) { addHeader("Content-Type", contentType); }
    void addHeader(const std::string& key, const std::string& value) { headers_[key] = value; }

    void appendToBuffer(Buffer*) const;

private:
    std::unordered_map<std::string, std::string>  headers_;
    HttpStateCode stateCode_;
    std::string statusMessage_;
    bool closeConnection_; //标注是否需要keepalive
    std::string body_;
};