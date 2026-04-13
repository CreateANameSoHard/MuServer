#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

#include "EventLoop.h"
#include "Thread.h"

class EventLoop;
//把eventloop放到一个线程上 合成一个具体的组件
class EventLoopThread :noncopyable
{
    // 线程在start前可能会有初始化的动作 
    using ThreadInitCallback = std::function<void(EventLoop*)>; 
public:
    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),const std::string& name = std::string());   //右值可以赋值给const的左值引用！！
    ~EventLoopThread();

    EventLoop* startLoop();
private:    
    void threadFunc();

    Thread thread_;
    EventLoop* loop_;
    std::mutex mutex_;
    std::condition_variable cond_; // startloop和threadFunc有同步关系 因为startloop要等待threadFunc的loop的初始化，返回线程栈上的loop指针
    bool exiting_;
    // 用于线程在创建前的初始化
    ThreadInitCallback initCallback_;   //这里的initCallback_是一个函数对象！！只不过没有被赋值初始化
};
