#pragma once
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <mutex>

#include "noncopyable.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>; //EventLoopThread的参数
    EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads){numThreads_ = numThreads;} //TcpServer里的setThreadnum调用的就是这里的setThreadnum
    void start(const ThreadInitCallback& cb = ThreadInitCallback()); //启动线程池

    EventLoop* getNextLoop(); //获取下一个线程的EventLoop baseloop通过轮询的方式分配channel给subloop
    std::vector<EventLoop*> getAllLoops(); //获取所有线程的EventLoop

    bool started() const { return started_; } //判断线程池是否已经启动
    const std::string& name() const {return name_; }

private:
    EventLoop* baseLoop_; //用户传的最基本的evtloop，如果用户没有setThreadNum的话，就一个线程，一个evtloop 这个就是mainloop
    std::string name_;   //线程池名字
    bool started_;
    int numThreads_;    //线程数 不过Thread类里有一个静态变量numThreads记录了线程数。但因为这里是线程池类，还是需要这个变量
    int next_;         //下一个待分配的索引
    std::vector<std::unique_ptr<EventLoopThread>> threads_; //线程池里的线程 这里用unique_ptr来管理线程
    std::vector<EventLoop*> loops_; //线程里的evtloop
};
