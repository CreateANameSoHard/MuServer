#pragma once
#include <functional>

#include "noncopyable.h"
#include "TimeStamp.h"

class Timer : noncopyable
{
    using TimerCallback = std::function<void()>;
    public:
        Timer(TimerCallback cb, TimeStamp when, double interval);
        ~Timer() = default;

        void run() const {callback_();}

        bool repeated() const {return repeated_;}
        TimeStamp expiration() const {return expiration_;}

        void restart(TimeStamp now);

    private:
        TimerCallback callback_;
        const double interval_;
        TimeStamp expiration_; //这个可不能为常对象
        const bool repeated_;
};