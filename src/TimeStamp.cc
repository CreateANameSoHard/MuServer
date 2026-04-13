#include "../include/TimeStamp.h"


// #include <iostream>
TimeStamp::TimeStamp()
    :microsecondsSinceEpoch_(0)
{

};

TimeStamp::TimeStamp(int64_t microsecondsSinceEpoch)
    : microsecondsSinceEpoch_(microsecondsSinceEpoch)
{
};

TimeStamp TimeStamp::now()
{
    struct timeval tv;
    // 获取微妙和秒
    // 在x86-64平台gettimeofday()已不是系统调用,不会陷入内核, 多次调用不会有性能损失.
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    // 转换为微妙
    return TimeStamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}


std::string TimeStamp::toString() const
{
    char buf[128] = {0};
    time_t seconds = SecondsSinceEpoch();
    tm* timeinfo = localtime(&seconds);
    snprintf(buf, sizeof buf, "%4d/%02d/%02d %02d:%02d:%02d",
        timeinfo->tm_year + 1900,
        timeinfo->tm_mon + 1, //月是从0开始的
        timeinfo->tm_mday,
        timeinfo->tm_hour,
        timeinfo->tm_min,
        timeinfo->tm_sec
    );
    return buf;
};

bool TimeStamp::operator<(const TimeStamp& ts)
{
    return microsecondsSinceEpoch_ < ts.microsecondsSinceEpoch_;
}

bool TimeStamp::operator>(const TimeStamp& ts)
{
    return microsecondsSinceEpoch_ > ts.microsecondsSinceEpoch_;
}

bool TimeStamp::operator==(const TimeStamp& ts)
{
    return microsecondsSinceEpoch_ == ts.microsecondsSinceEpoch_;
}
// 注意输入为秒
TimeStamp TimeStamp::operator+(const double& seconds)
{
    return TimeStamp(microsecondsSinceEpoch_ + seconds * kMicroSecondsPerSecond);
}
