#pragma once
class copyable
{
// 只允许被继承 且被继承后不允许拷贝
protected:
    copyable() = default;
    ~copyable() = default;
    copyable(const copyable&) = default;
    copyable& operator=(const copyable&) = default;
};
