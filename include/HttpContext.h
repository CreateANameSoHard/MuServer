#pragma once

#include "HttpRequest.h"

class Buffer;
//http请求解析上下文 记录解析的状态
class HttpContext : copyable
{
    //主状态机
    enum class HttpRequestParseState
    {
        kExpectRequestLine,
        kExpectRequestHeader,
        kExpectRequestBody,
        kGotAll
    };
    using State = HttpContext::HttpRequestParseState;
public:
    HttpContext() : state_(State::kExpectRequestLine){}
    void reset()
    {
        //重置状态为state
        state_ = HttpRequestParseState::kExpectRequestLine;
        HttpRequest dummy;
        request_.swap(dummy);
    }

    HttpRequest& request() {return request_;}
    const HttpRequest& request() const {return request_;}

    bool gotAll() {return state_ == HttpRequestParseState::kGotAll;}

    bool parseRequest(Buffer* buffer, TimeStamp receivedTime); //返回是否解析成功
private:
    bool parseRequestLine(const char* start, const char* end);

    State state_;
    HttpRequest request_; //把解析出来的结果保存到对象里
};