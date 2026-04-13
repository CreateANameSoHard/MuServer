#pragma once
#include <set>
#include <functional>
#include <vector>

#include "TimeStamp.h"
#include "noncopyable.h"
#include "Channel.h"

class EventLoop;
class Timer;
class Channel;
//一个timerfd来提供全部的定时器的定时机制
class TimerQueue : noncopyable
{
    using TimerCallback = std::function<void()>;
    public:
        TimerQueue(EventLoop* loop);
        ~TimerQueue();
        //供其他组件调用
        void addTimer(TimerCallback cb, TimeStamp when, double interval); //添加定时器的入口 仅创建定时器，然后调用线程安全的addTimerInloop
    private:
        using Entry = std::pair<TimeStamp, Timer*>;
        using TimerList = std::set<Entry>;

        void addTimerInLoop(Timer*); //线程安全地添加定时器 即向set里插入定时器 并根据是否为最近超时来修改timerfd
        void handleRead(); //读集合 获取超时的定时器列表 执行列表里定时器的回调 执行完毕后要重设TimerQueue
        //这个reset是对timerfd reset
        void resetTimerFd(int timerfd_, TimeStamp expiration); //重设timerfd 即根据传入的expiration 调用timerfd_settime 注意最近的超时时间不能小于100ms
        //这个reset是对timers_容器reset
        void reset(const std::vector<Entry>& expired, TimeStamp now); // 重置TimerQueue 遍历expired列表，如果是持续定时器 则重新定时并插入timers,否则删除 然后重设timerfd

        std::vector<Entry> getExpired(TimeStamp); // 获取全部超时的定时器 并在timers_列表里删除

        bool insert(Timer* timer); //将定时器插入到timers_里，返回是否改变了最近超时时间

        EventLoop* loop_;
        const int timer_fd_;
        Channel timerChannel_;
        TimerList timers_;

        bool callingExpiredTimers;
};