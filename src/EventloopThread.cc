#include "../include/EventloopThread.h"
#include "../include/EventLoop.h"

#include <mutex>

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    : thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      loop_(nullptr),
      mutex_(),
      cond_(),
      exiting_(false),
      initCallback_(cb)
{
}
EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join(); // 等待线程退出
    }
}

/*
    startloop和ThreadFunc有同步关系（startloop为开启线程，ThreadFunc为创建loop），startloop要等到loop对象初始化完毕才能返回loop对象
*/
EventLoop *EventLoopThread::startLoop()
{
    thread_.start(); // 启动线程 新线程执行的是ThreadFunc方法
    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]
                   { return loop_ != nullptr; }); // 等待loop初始化完毕
    }
    loop = loop_;
    return loop;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop; // 创建一个eventloop
    if (initCallback_)
    {
        initCallback_(&loop); // 执行回调函数
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_all(); // loop初始化完毕，线程可以访问loop指针
    }

    loop.loop(); // 进入eventloop循环

    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = nullptr; // 线程退出，loop_置空
}