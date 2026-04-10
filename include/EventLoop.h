#pragma once
#include <memory>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>


#include "noncopyable.h"
#include "TimeStamp.h"
#include "CurrentThread.h"
#include "TimerQueue.h"

class Channel;
class Poller;

class EventLoop :noncopyable
{
    using ChannelList = std::vector<Channel*>;  //这个变量维护所有的channel
    using Functor = std::function<void()>; //loop需要在后续执行的函数

public:
    EventLoop();
    ~EventLoop();

    void loop(); //启动事件循环
    void quit(); //退出事件循环

    void runInLoop(Functor cb); //在事件循环中执行回调函数
    void queueInLoop(Functor cb); //将回调函数放入待执行队列

    //添加定时器功能
    void runAt(const Functor& cb, TimeStamp when)
    {
        timerqueue_->addTimer(std::move(cb), when, 0.0);
    }
    //这个waittime是相对时间
    void runAfter(const Functor& cb, double waittime)
    {
        TimeStamp ts(TimeStamp::now() + waittime);
        runAt(std::move(cb), ts);
    }
    void runEvery(const Functor& cb, double interval)
    {
        TimeStamp ts(TimeStamp::now() + interval);
        timerqueue_->addTimer(std::move(cb), ts, interval);
    }


    TimeStamp pollReturnTime() const { return pollReturnTime_; } //返回上一次poll返回的时间


    void wakeup(); //唤醒事件循环

    void updateChannel(Channel* channel); //更新channel的状态
    void removeChannel(Channel* channel); //从活跃channel列表中移除channel
    void hasChannel(Channel* channel); //判断channel是否活跃

    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); } //判断当前线程是否为EventLoop所在线程 如果不在，则会在queue里等到当前线程


private:
    void handleRead(); //wakeup
    void doPendingFunctors(); //执行待执行的回调函数

    std::atomic_bool looping_; //是否正在运行
    std::atomic_bool quit_; //是否退出
    
    
    const pid_t threadId_; //记录创建了eventloop对象的线程id
    TimeStamp pollReturnTime_; //记录上一次poll返回的时间
    std::unique_ptr<Poller> poller_; //事件循环 

    std::unique_ptr<TimerQueue> timerqueue_; //定时器队列
    
    int wakeupFd_; 
    std::unique_ptr<Channel> wakeupChannel_; 

    ChannelList activeChannels_; //活跃的channel列表 即fillactiveChannel的输入
    Channel* currentActiveChannel_; //当前正在处理的channel


    std::atomic_bool callingPendingFunctors_; //当前是否有需要执行的回调
    std::vector<Functor> pendingFunctors_; //待执行的回调函数列表


    std::mutex mutex_; //互斥锁 保证vector容器的线程安全
};