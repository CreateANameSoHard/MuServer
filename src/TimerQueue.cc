#include <sys/timerfd.h>
#include <unistd.h>
#include <cstring>


#include "../include/logger.h"
#include "../include/TimerQueue.h"
#include "../include/TimeStamp.h"
#include "../include/Timer.h"
#include "../include/Channel.h"
#include "../include/EventLoop.h"


int CreateTimerfdOrDie()
{
    //绝对时间
    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if(timerfd < 0)
    {
        LOG_ERROR << "create timerfd error!";
    }
    return timerfd;
}

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
    timer_fd_(CreateTimerfdOrDie()),
    callingExpiredTimers(false),
    timerChannel_(loop, timer_fd_),
    timers_()
{
    // 在timer和poller之间建立channel
    timerChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    timerChannel_.disableAll();
    timerChannel_.remove();
    ::close(timer_fd_);
    //删除红黑树上的所有定时器
    for(auto& entry:timers_)
    {
        delete entry.second;
    }
}
//辅助函数 与类无关
//不关心读到的内容
void ReadTimerFd(int timerfd)
{
    int64_t expirtion;
    size_t ret = ::read(timerfd, &expirtion, sizeof expirtion);
    if(ret != sizeof expirtion)
    {
        LOG_ERROR << "read timerfd error!";
    }
}

//如果timerfd可读则触发读事件 调用这个方法
void TimerQueue::handleRead()
{
    TimeStamp now = TimeStamp::now();
    ReadTimerFd(timer_fd_);

    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers = true;
    //遍历超时的定时器 调用其回调
    for(const auto& entry:expired)
    {
        entry.second->run();
    }
    callingExpiredTimers = false;

    reset(expired, now);
}

void TimerQueue::addTimer(TimerCallback cb, TimeStamp when, double interval)
{
    Timer* timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
    bool eariestChanged = insert(timer);
    if(eariestChanged)
    {
        //需要重置timerfd的触发时间
        resetTimerFd(timer_fd_, timer->expiration());
    }
}

bool TimerQueue::insert(Timer* timer)
{
    bool eariestchanged = false;
    TimeStamp when = timer->expiration();
    auto it = timers_.begin();
    // 此时插入的timer为超时时间最短的
    if(it == timers_.end() || when < it->first)
    {   
        eariestchanged = true;
    }
    timers_.emplace(when, timer);
    return eariestchanged;
}
//timers集合里超时时间最短的定时器改了，所以要重新设置一下timerfd，通过timerfd_settime调用来完成
void resetTimerFd(int timer_fd, TimeStamp expiration)
{
    itimerspec new_value;
    itimerspec old_value;
    bzero(&new_value, sizeof new_value);
    bzero(&old_value, sizeof old_value);

    int64_t delta = expiration.microsecondsSinceEpoch() - TimeStamp::now().microsecondsSinceEpoch();
    //超时时间至少要100ms
    if(delta < 100)
        delta = 100;

    timespec ts;
    ts.tv_sec = static_cast<time_t>(delta / TimeStamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((delta % TimeStamp::kMicroSecondsPerSecond) * 1000);

    //不需要管interval
    new_value.it_value = ts;

    if(::timerfd_settime(timer_fd, 0, &new_value, &old_value) < 0)
    {
        LOG_ERROR << "timerfd_settime error!";
    }
}
//获取timerlist集合里的所有过期定时器 并删除
std::vector<TimerQueue::Entry> TimerQueue::getExpired(TimeStamp upperbound)
{
    std::vector<TimerQueue::Entry> expired;
    //通过比较TimeStamp来判断超时 此时second不重要
    Entry entry({upperbound, reinterpret_cast<Timer*>(UINTPTR_MAX)});
    auto it = timers_.lower_bound(entry); //二分查找 找到第一个大于等于now的
    //此时it之前的定时器全部超时
    std::copy(timers_.begin(), it, std::back_inserter(expired));
    timers_.erase(timers_.begin(), it);

    return expired;
}
//expired为已经删除了的Entry对象
void TimerQueue::reset(const std::vector<Entry>& expired, TimeStamp now)
{
    // TimeStamp nextExpired;
    for(const Entry& entry : expired)
    {
        if(entry.second->repeated())
        {
            Timer* timer = entry.second;
            timer->restart(now);
            insert(timer);
        }
        else
        {
            delete entry.second;
        }
        //根据最近的超时时间来重设timerfd
        //每处理一个都要重新更新timerfd的定时时间
        if(!timers_.empty())
        {
            resetTimerFd(timer_fd_, (timers_.begin()->second)->expiration());
        }
    }
}