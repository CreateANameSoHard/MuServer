#include "../include/HttpResponse.h"
#include "../include/Buffer.h"
#include "../include/logger.h"

void HttpResponse::appendToBuffer(Buffer *output) const
{
    std::string crlf = "\r\n";

    std::string responseLine = "HTTP/1.1 " + std::to_string(int(stateCode_)) + " ";

    LOG_INFO << "response line: " << responseLine;

    output->append(responseLine);
    output->append(statusMessage_);
    output->append(crlf);

    if (closeConnection_)
    {
        output->append("Connection: close");
        output->append(crlf);
    }
    else
    {
        // snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
        std::string contentLength = "Content-Length: " + std::to_string(body_.size()) + "\r\n";
        output->append(contentLength);
        output->append("Connection: Keep-Alive");
        output->append(crlf);
    }

    // 添加头部
    for (const auto &header : headers_)
    {
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append(crlf);
    }
    output->append(crlf);
    output->append(body_);
}

void HttpResponse::appendHeaderToBuffer(Buffer *output) const
{
    std::string crlf = "\r\n";

    std::string responseLine = "HTTP/1.1 " + std::to_string(int(stateCode_)) + " ";

    LOG_INFO << "response line: " << responseLine;

    output->append(responseLine);
    output->append(statusMessage_);
    output->append(crlf);

    if (closeConnection_)
    {
        output->append("Connection: close");
        output->append(crlf);
    }
    else
    {
        // snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
        std::string contentLengthString = "Content-Length: " + std::to_string(file_->getFileSize()) + "\r\n";
        output->append(contentLengthString);
        output->append("Connection: Keep-Alive");
        output->append(crlf);
    }

    // 添加头部
    for (const auto &header : headers_)
    {
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append(crlf);
    }
    output->append(crlf);
}