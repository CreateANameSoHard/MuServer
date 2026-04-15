#pragma once
#include <string>
#include <memory>
#include <unordered_map>

#include "copyable.h"
#include "StaticFile.h"


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
            closeConnection_(close),
            isStaticFile_(false)
    {}

    void setStatusCode(HttpStateCode code) { stateCode_ = code;}
    void setStatusMessage(const std::string& msg) { statusMessage_ = std::move(msg); }
    void setBody(const std::string& msg) { body_ = std::move(msg); }

    bool closeConnection() const { return closeConnection_; }
    void setCloseConnection(bool close) { closeConnection_ = close; }

    void setContentType(const std::string& contentType) { addHeader("Content-Type", contentType); }
    void addHeader(const std::string& key, const std::string& value) { headers_[key] = value; }

    void appendToBuffer(Buffer*) const;
    void appendHeaderToBuffer(Buffer*) const;

    bool isStaticFile() const {return isStaticFile_;}
    //enable后才创建
    void enableStaticFile(const StaticFile& file) { isStaticFile_ = true; file_.reset(new StaticFile(file)); }
    void disableStaticFile() { isStaticFile_ = false; file_.reset();}

    const StaticFile* getFileptr() const { return file_.get(); }


private:
    std::unordered_map<std::string, std::string>  headers_;
    HttpStateCode stateCode_;
    std::string statusMessage_;
    bool closeConnection_; //标注是否需要keepalive
    std::string body_;
    
    bool isStaticFile_;
    std::unique_ptr<StaticFile> file_;
};