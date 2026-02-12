#pragma once
class copyable
{
// for extend not for use
protected:
    copyable() = default;
    ~copyable() = default;
    copyable(const copyable&) = default;
    copyable& operator=(const copyable&) = default;
};
