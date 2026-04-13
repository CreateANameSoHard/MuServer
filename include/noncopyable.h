#pragma once

// 隐藏构造函数 让noncopyable只能通过继承来构造 且不能拷贝 
class noncopyable
{

public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

