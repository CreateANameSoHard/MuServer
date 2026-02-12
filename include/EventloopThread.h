#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

#include "EventLoop.h"
#include "Thread.h"

class EventLoop;

class EventLoopThread :noncopyable
{
    using ThreadInitCallback = std::function<void(EventLoop*)>; //这里的ThreadInitCallback是类型！!所以Callback()是一个右值对象（默认的空对象）
public:
    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),const std::string& name = std::string());   //右值可以赋值给const的左值引用！！
    ~EventLoopThread();

    EventLoop* startLoop();
private:    
    void threadFunc();

    Thread thread_;
    EventLoop* loop_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool exiting_;
    // 用于线程在创建前的初始化
    ThreadInitCallback initCallback_;   //这里的initCallback_是一个函数对象！！只不过没有被赋值初始化
};
