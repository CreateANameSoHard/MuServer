#pragma once
#include <string>
#include <unordered_map>
#include <cassert>

#include "copyable.h"
#include "TimeStamp.h"
// HttpRequest对象是不包含解析功能的
class HttpRequest
{
public:
    enum class Method
    {
        kInvalid,
        kGet,
        kPost,
        kDelete,
        kPut,
        kHead
    };
    enum class Version
    {
        kUnkown,
        kHttp10,
        kHttp11
    };

    HttpRequest()
        : method_(Method::kInvalid),
          version_(Version::kUnkown)
    {
    }

    HttpRequest::Method method() const { return method_; }
    bool setMethod(const char *start, const char *end)
    {
        assert(method_ == Method::kInvalid);
        std::string method(start, end);
        if (method == "GET")
        {
            method_ = HttpRequest::Method::kGet;
        }
        else if (method == "POST")
        {
            method_ = HttpRequest::Method::kPost;
        }
        else if (method == "DELETE")
        {
            method_ = HttpRequest::Method::kDelete;
        }
        else if (method == "PUT")
        {
            method_ = HttpRequest::Method::kPut;
        }
        else if (method == "HEAD")
        {
            method_ = HttpRequest::Method::kHead;
        }
        else
        {
            method_ = HttpRequest::Method::kInvalid;
        }
        return method_ != HttpRequest::Method::kInvalid;
    }
    bool isValidMethod() const
    {
        std::string method = methodString();
        return method == "GET" ||
               method == "POST" ||
               method == "HEAD" ||
               method == "PUT" ||
               method == "DELETE";
    }

    const std::string methodString() const
    {
        const char *v = "UNKNOWN";
        switch (method_)
        {
        case HttpRequest::Method::kGet:
            v = "GET";
            break;
        case HttpRequest::Method::kPost:
            v = "POST";
            break;
        case HttpRequest::Method::kDelete:
            v = "DELETE";
            break;
        case HttpRequest::Method::kPut:
            v = "PUT";
            break;
        case HttpRequest::Method::kHead:
            v = "HEAD";
            break;
        default:
            break;
        }
        return std::string(v);
    }

    const std::string &path() const { return path_; }
    void setPath(const char *start, const char *end) { path_.assign(start, end); }

    HttpRequest::Version version() const { return version_; }
    void setVersion(HttpRequest::Version version) { version_ = version; }

    const std::string &query() const { return query_; }
    void setQuery(const char *start, const char *end) { query_.assign(start, end); }

    const TimeStamp &receivedTime() const { return receivedTime_; }
    void setReceivedTime(const TimeStamp &t) { receivedTime_ = std::move(t); }

    const std::unordered_map<std::string, std::string> &headers() const { return headers_; }
    // colon:冒号
    void addHeaders(const char *start, const char *colon, const char *end)
    {
        std::string field(start, colon);
        colon++;
        // 跳过colon后的空格
        while (colon != end && isspace(*colon))
            colon++;
        std::string value(colon, end);
        // 删除后面的\t \n 通过isspace判断 每次删一个字符
        while (!value.empty() && isspace(value[value.size() - 1]))
            value.resize(value.size() - 1);
        headers_[field] = value;
    }

    std::string getHeader(const std::string &field) const
    {
        std::string ret;
        auto it = headers_.find(field);
        if (it != headers_.end())
            ret = it->second;
        return ret;
    }

    // 这里访问控制是没问题的 C++以类为单位控制访问 而不是对象
    void swap(HttpRequest &other)
    {
        std::swap(method_, other.method_);
        path_.swap(other.path_);
        std::swap(version_, other.version_);
        query_.swap(other.query_);
        receivedTime_.swap(other.receivedTime_);
        headers_.swap(other.headers_);
    }

private:
    Method method_;
    std::string path_;
    Version version_;
    std::string query_; // url的参数
    TimeStamp receivedTime_; //这个参数是为了配合onMessage的timestamp参数
    std::unordered_map<std::string, std::string> headers_;
};