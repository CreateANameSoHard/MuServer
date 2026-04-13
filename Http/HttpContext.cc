#include <cctype>

#include "../include/HttpContext.h"
#include "../include/Buffer.h"
#include "../include/TimeStamp.h"
#include "../include/logger.h"

// 解析请求的入口
bool HttpContext::parseRequest(Buffer* buffer, TimeStamp receivedTime)
{
    bool done = true;
    bool hasMore = true;
    while(hasMore)
    {
        if(state_ == HttpRequestParseState::kExpectRequestLine)
        {
            const char* crlf = buffer->findCRLF();
            if(crlf)
            {
                done = parseRequestLine(buffer->peek(), crlf + 2);
                if(done)
                {
                    request_.setReceivedTime(receivedTime);
                    buffer->retrieve(crlf + 2 - buffer->peek()); //readerIndex移到请求头的第一个字符
                    state_ = HttpRequestParseState::kExpectRequestHeader;
                }
                else
                {
                    LOG_INFO << "parseRequestLine failed!";
                    hasMore = false;
                }
            }
            else
            {
                hasMore = false;
            }
        }
        else if(state_ == HttpRequestParseState::kExpectRequestHeader)
        {
            //请求头的某一行的结尾
            const char* crlf = buffer->findCRLF();
            if(crlf)
            {
                const char* colon = std::find(buffer->peek(), crlf, ':');
                if(colon != crlf)
                {
                    request_.addHeaders(buffer->peek(), colon, crlf);
                    // buffer->retrieve(crlf + 2 - buffer->peek());
                }
                //请求头解析结束
                else
                {
                    hasMore = false;
                    state_ = HttpRequestParseState::kGotAll;
                }
                buffer->retrieve(crlf + 2 - buffer->peek());
            }
            else
            {
                hasMore = false;
            }
        }
        else if(state_ == HttpRequestParseState::kExpectRequestBody)
        {
            //不处理请求体
        }
    }
    return done;
}
//FIXME:
bool HttpContext::parseRequestLine(const char* start, const char* end) {
    enum class State {
        kMethod,
        kSpacesBeforePath,
        kPath,
        kQuery,
        kSpacesBeforeVersion,
        kVersion,
        kCR,
        // kLF,
        kDone
    };
    LOG_INFO << "requestline: " << std::string(start, end);

    const size_t kMaxRequestLine = 8192;

    State state = State::kMethod;

    const char* p = start;
    const char* method_start = nullptr;
    const char* path_start = nullptr;
    const char* query_start = nullptr;
    const char* version_start = nullptr;

    for (; p != end; ++p) {
        // 防御：超长请求行
        if (static_cast<size_t>(p - start) > kMaxRequestLine) return false;

        char ch = *p;

        switch (state) {

        case State::kMethod:
            if (!method_start) method_start = p;

            if (ch == ' ') {
                if (p == method_start) return false; // 空 method

                request_.setMethod(method_start, p);

                if (!request_.isValidMethod()) return false;

                state = State::kSpacesBeforePath;
            }
            break;

        case State::kSpacesBeforePath:
            if (ch == ' ') break;

            if (ch != '/') return false; // path 必须以 '/' 开头

            path_start = p;
            state = State::kPath;
            break;

        case State::kPath:
            if (ch == '?') {
                request_.setPath(path_start, p);
                query_start = p + 1;
                state = State::kQuery;
            }
            else if (ch == ' ') {
                request_.setPath(path_start, p);
                state = State::kSpacesBeforeVersion;
            }
            else if (ch == '\r' || ch == '\n') {
                return false; // path 中不能出现 CRLF
            }
            break;

        case State::kQuery:
            if (ch == ' ') {
                request_.setQuery(query_start, p);
                state = State::kSpacesBeforeVersion;
            }
            else if (ch == '\r' || ch == '\n') {
                return false;
            }
            break;

        case State::kSpacesBeforeVersion:
            if (ch == ' ') break;

            if (ch != 'H') return false; // 必须是 HTTP/

            version_start = p;
            state = State::kVersion;
            break;

        case State::kVersion:
            if (ch == '\r') {
                std::string v(version_start, p);

                if (v == "HTTP/1.1") {
                    request_.setVersion(HttpRequest::Version::kHttp11);
                } else if (v == "HTTP/1.0") {
                    request_.setVersion(HttpRequest::Version::kHttp10);
                } else {
                    return false;
                }

                state = State::kCR;
            }
            break;

        case State::kCR:
            if (ch == '\n') {
                state = State::kDone;
                return true;
            }
            return false;

        case State::kDone:
            return true;
        }
    }

    return false;
}

