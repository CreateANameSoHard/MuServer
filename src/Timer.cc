#include "../include/Timer.h"

Timer::Timer(TimerCallback cb, TimeStamp when, double interval)
    :callback_(std::move(cb)),
    expiration_(when),
    interval_(interval),
    repeated_(interval > 0.0)
{

}

void Timer::restart(TimeStamp now)
{
    //重复定时事件
    if(repeated_)
    {
        expiration_ = expiration_ + interval_;
    }
    else
    {
        expiration_ = now;
    }
}