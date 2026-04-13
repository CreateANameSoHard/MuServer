#pragma once
#include <vector>
#include <assert.h>
#include <string>
#include <algorithm>

#include "copyable.h"
/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
// 读缓冲和写缓冲放一起，提升读取和写效率。另外readable是用来读取数据的，writable是用来写入数据的。

// 网络库底层的缓冲器类型定义
class Buffer : public copyable
{
public:
    static const size_t kCheapPrepend = 8;   //  记录数据包长度 解决粘包问题
    static const size_t kInitialSize = 1024; // 初始大小(不包括prepend的大小)

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend)
    {
    }
    ~Buffer() = default;

    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const { return buffer_.size() - writerIndex_; } 
    size_t prependableBytes() const { return readerIndex_; }
    const char *peek() const { return begin() + readerIndex_; } // 获取缓冲区的读起始地址
    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        if (len < readableBytes())
        {
            readerIndex_ += len;
        }
        else
        {
            retrieveAll();
        }
    }
    // 复位指针
    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }
    // 把onMessage函数上报的buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString() { return retrieveAsString(readableBytes()); }
    std::string retrieveAsString(size_t len)
    {
        assert(len <= readableBytes());
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }
    // 调用接下来需要写入的数据长度
    void ensureWritableBytes(size_t len)
    {
        // 接收的数据可能会大于现有的writeableBytes，所以需要考虑到扩容的问题
        if (len > writableBytes())
        {
            // 扩容
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }
    void append(const std::string& str)
    {
        append(str.c_str(), str.size());
    }

    void append(const void* data, size_t len)
    {
        append(static_cast<const char*>(data), len);
    }

    // 把内存中[data, data+len]内存上的数据，添加到writeable缓冲区当中
    void append(const char *data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }
    // 从文件描述符中读数据。数据读到缓冲里
    ssize_t readFd(int fd, int *savedErrno);
    // ssize_t writeFd(int fd, int *savedErrno);

    const char* findCRLF() const
    {
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
        return crlf ? crlf:nullptr;
    }
    const char* findCRLF(const char* start) const 
    {
        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
        return crlf ? crlf:nullptr;
    }

private:
    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        // 优化指针位置
        else
        {
            size_t readable = readableBytes();
            // 把可读区域的内容赋值到前面，可写的区域就变大了
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
            assert(readable == readableBytes());
        }
    }

    char *begin() { return &*buffer_.begin(); }                       // 获取缓冲区的起始地址
    const char *begin() const { return &*buffer_.begin(); }           // 获取缓冲区的起始地址
    char *beginWrite() { return begin() + writerIndex_; }             // 获取可写区域的起始地址
    const char *beginWrite() const { return begin() + writerIndex_; } // 获取可写区域的起始地址

    std::vector<char> buffer_; // 缓冲区 每个元素是一个字符
    size_t readerIndex_;       // 读指针
    size_t writerIndex_;       // 写指针

    static const char kCRLF[];
};