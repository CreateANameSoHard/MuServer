#include "../include/EventloopThreadPool.h"
#include "../include/EventloopThread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop),
      name_(nameArg),
      started_(false),
      numThreads_(0),
      next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // 线程池管的是线程，线程执行的是evtloopThread的ThreadFunc函数。
    // 这个函数的资源都在线程栈上，执行完毕后自动会析构。所以这里不需要做什么
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    started_ = true;

    for (int i = 0; i < numThreads_; ++i)
    {
        // 设置线程名字。线程名字就用线程池名字+循环下标表示
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);

        EventLoopThread *thread = new EventLoopThread(cb, buf);

        // 这里只是创建了线程，但是还没有启动线程
        threads_.emplace_back(std::unique_ptr<EventLoopThread>(thread));
        // startLoop()会启动线程，并返回evtloop!!!!!!!!!
        loops_.push_back(thread->startLoop());
    }
    // 用户没有设置多线程，那就直接用baseloop_执行cb
    /*
        为什么numthreads == 0要额外判断？
        因为baseloop为eventloop，没有绑定thread，所以没有线程启动的cb流程。
        当只有一个线程时，baseloop既是mainloop，也是subloop，所以要执行cb
    */
    if (numThreads_ == 0 && cb)
    {
        cb(baseLoop_);
    }
}

// 轮询，查找下一个eventloop
EventLoop *EventLoopThreadPool::getNextLoop()
{
    EventLoop *loop = baseLoop_;
    if (!loops_.empty())
    {
        loop = loops_[next_];
        next_ = (next_ + 1) % loops_.size();
    }
    return loop;
}
std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    if (loops_.empty())
    {
        return std::vector<EventLoop *>(1, baseLoop_);
    }
    else
    {
        return loops_;
    }
}
