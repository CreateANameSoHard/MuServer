#include<chrono>

#include "../include/TimeStamp.h"

TimeStamp::TimeStamp()
: microSecondsSinceEpoch_(0)
{}

TimeStamp::TimeStamp(int64_t microSecondsSinceEpoch)
: microSecondsSinceEpoch_(microSecondsSinceEpoch)
{}

TimeStamp TimeStamp::now()
{
    //should return the current time in microseconds since epoch
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return TimeStamp(std::chrono::duration_cast<std::chrono::microseconds>(duration).count());
}

std::string TimeStamp::toString() const
{
    time_t raw_time;
    time(&raw_time);
    //convert to string
    struct tm* timeinfo = localtime(&raw_time);
    char buf[40] = {0};
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    return buf;
}

// int main()
// {
//     TimeStamp ts = TimeStamp::now();
//     std::cout << "TimeStamp: " << ts.toString() << std::endl;
//     return 0;
// }