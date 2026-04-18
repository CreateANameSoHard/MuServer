// 抽象基类
#pragma once
template <class Key, class Value>
class CachePolicy
{
public:
    virtual ~CachePolicy() = default;
    virtual Value get(Key key) = 0;
    virtual bool get(Key, Value &) = 0;

    virtual void put(Key, Value) = 0;
};