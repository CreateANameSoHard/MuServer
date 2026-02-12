#pragma once
/*
    同步日志的后端
*/
#include <strings.h>
#include <string.h>
#include <string>

#include "noncopyable.h"

constexpr int kSmallBuffer = 4000;
constexpr int kLargeBuffer = 4000 * 1000;

// 这个模板不是给用户用的，是为了方便作者编码（因为会在cc文件里显式实例化）
template <int SIZE>
class FixedBuffer : noncopyable
{
public:
    FixedBuffer() : cur_(data_)
    {
        setCookie(cookieStart);
    }
    ~FixedBuffer()
    {
        setCookie(cookieEnd);
    }

    const char *data() const { return data_; }
    const char *end() const { return data_ + sizeof data_; }
    int length() const { return cur_ - data_; }
    char *current() const { return cur_; }
    void reset() { cur_ = data_; }
    void bzero() { ::bzero(data_, sizeof data_); }
    int avail() const { return end() - cur_; }
    void add(size_t len) { cur_ += len; }
    std::string toString() const { return std::string(data_, length()); }
    void setCookie(void (*cookie)()) { cookie_ = cookie; }

    void append(const char *buf, size_t len)
    {
        if ((size_t)avail()  > len)
        {
            ::memcpy(cur_, buf, len);
            add(len);
        }
    }

    static void cookieStart();
    static void cookieEnd();

private:
    void (*cookie_)(); // 调试用
    char data_[SIZE];
    char *cur_;
};

class LogStream : noncopyable
{
public:
    typedef FixedBuffer<kSmallBuffer> Buffer; // 默认使用小缓冲区
    typedef LogStream Self;                   // 在涉及到返回类本身时，可以这样定义以提高代码可读性

    LogStream() = default;
    ~LogStream() = default;

    Self& operator<<(const void* p); // 这个类型还是有重载的，因为为指针

    // 重载布尔类型数据的输出
    Self &operator<<(bool v)
    {
        buffer_.add(v ? 1 : 0);
        return *this;
    }

    Self &operator<<(short v);              // 重载short类型数据的输出
    Self &operator<<(unsigned short v);     // 重载unsigned short类型数据的输出
    Self &operator<<(int v);                // 重载int类型数据的输出
    Self &operator<<(unsigned int v);       // 重载unsigned int类型数据的输出
    Self &operator<<(long v);               // 重载long类型数据的输出
    Self &operator<<(unsigned long v);      // 重载unsigned long类型数据的输出
    Self &operator<<(long long v);          // 重载long long类型数据的输出
    Self &operator<<(unsigned long long v); // 重载unsigned long long类型数据的输出

    Self &operator<<(float v); // 重载float类型数据的输出

    Self &operator<<(double v); // 重载double类型数据的输出

    Self &operator<<(char v)
    {
        buffer_.append(&v, 1);
        return *this;
    }
    Self &operator<<(const char *s) // 重载const char *类型数据的输出
    {
        if (s)
        {
            buffer_.append(s, ::strlen(s));
        }
        else
        {
            buffer_.append("(null)", 6);
        }
        return *this;
    }
    Self &operator<<(std::string str)
    {
        buffer_.append(str.c_str(), str.size());
        return *this;
    }

    Self &operator<<(const Buffer buf)
    {
        *this << buf.toString();
        return *this;
    }

    void append(const char *logline, size_t len) { return buffer_.append(logline, len); }
    const Buffer &buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }

private:
    template <typename T>
    void formatInteger(T v); // 格式化整数 即把各种长度的整数 通过这个统一的接口加到buffer里

    Buffer buffer_;
    static const int kMaxNumericSize = 48; // 最大的数字长度
};
