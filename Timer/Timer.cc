#include "../include/Timer.h"

Timer::Timer(TimerCallback cb, TimeStamp when, double interval)
    :callback_(std::move(cb)),
    interval_(interval),
    expiration_(when),
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