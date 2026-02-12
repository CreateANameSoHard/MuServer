#pragma once

#include <ctime>
#include <string>
#include <mutex>
#include <sys/types.h>
#include <memory>

class AppendFile;

class LogFile
{
public:
    
    LogFile(const std::string& basename, off_t rollSize, bool threadSafe = true,int flushInterval = 3, int checkEveryN = 1024);
    ~LogFile();

    void append(const char* logline, int len); // 写入日志  根据构造函数的输入参数确定是否需要线程安全

    void flush(); // 刷新缓存

    bool rollFile(); // 日志滚动

private:
    void append_unlocked(const char* logline, int len); // 线程不安全地写入
    static std::string getLogFileName(const std::string& basename, time_t* now); // 根据basename和时间 创建日志文件名

    const std::string basename_; // 日志文件的基础名称
    const int rollSize_; // 日志文件的最大大小，单位：字节
    const int flushInterval_; // 缓存的刷新间隔
    const int checkEveryN_; // 每N次写入检查一次是否滚动

    std::unique_ptr<std::mutex> mutex_; // 互斥锁
    time_t startOfPeriod_; // 日志文件的开始时间
    time_t lastRoll_; // 上次滚动的时间
    time_t lastFlush_; // 上次刷新的时间

    std::unique_ptr<AppendFile> file_; // 日志写入对象

    int count_; // 日志写入计数器

    static const int kRollPerSeconds_ = 60 * 60 * 24; // 每天固定滚动一次日志文件
};