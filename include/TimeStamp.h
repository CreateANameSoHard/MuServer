#pragma once

#include <sys/time.h>
#include <string>
#include <ctime>

class TimeStamp
{
public:
    TimeStamp();
    explicit TimeStamp(int64_t);
    ~TimeStamp() = default;

    static TimeStamp now(); // 获得当前时间的时间戳
    std::string toString() const;
    

    static const int kMicroSecondsPerSecond = 1000 * 1000;

    bool operator>(const TimeStamp &);
    bool operator<(const TimeStamp &);
    bool operator==(const TimeStamp &);
    TimeStamp operator+(const double &);
    void setMicroSecondsSinceEpoch(int64_t v){ microsecondsSinceEpoch_ = v;}
    int64_t microsecondsSinceEpoch() const { return microsecondsSinceEpoch_; }
    time_t SecondsSinceEpoch() const { return static_cast<time_t>(microsecondsSinceEpoch_ / kMicroSecondsPerSecond); }
    TimeStamp invalid() { return TimeStamp(); } // 返回一个失效的时间
    void swap(TimeStamp& other)
    {
        int64_t tmp = microsecondsSinceEpoch_;
        microsecondsSinceEpoch_ = other.microsecondsSinceEpoch();
        other.setMicroSecondsSinceEpoch(tmp);
    }

    friend TimeStamp operator+(const double &seconds, TimeStamp ts);
    friend bool operator<(const TimeStamp& ts1, const TimeStamp& ts2);

private:
    int64_t microsecondsSinceEpoch_;
};

inline TimeStamp operator+(const double &seconds, TimeStamp ts)
{
    return ts.operator+(seconds);
}
inline bool operator<(const TimeStamp& ts1, const TimeStamp& ts2)
{
    return ts1.microsecondsSinceEpoch() < ts2.microsecondsSinceEpoch();
}