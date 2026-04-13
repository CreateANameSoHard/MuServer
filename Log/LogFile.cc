#include <thread>
#include <string>

#include "../include/LogFile.h"
#include "../include/FileUtil.h"
#include "../include/CurrentThread.h"

LogFile::LogFile(const std::string &basename, off_t rollSize, bool threadSafe, int flushInterval, int checkEveryN)
    : basename_(basename),
      rollSize_(rollSize),
      flushInterval_(flushInterval),
      checkEveryN_(checkEveryN),
      mutex_(threadSafe ? new std::mutex : nullptr), // 根据输入选项，确定是否线程安全
      startOfPeriod_(0),
      lastRoll_(0),
      lastFlush_(0),
      count_(0)
{
    rollFile(); // 创建对象就滚动一次
}

LogFile::~LogFile() = default;

void LogFile::append(const char *logline, int len)
{
    if (mutex_)
    {
        std::lock_guard<std::mutex> lock(*mutex_);
        append_unlocked(logline, len);
    }
    else
    {
        append_unlocked(logline, len);
    }
}

void LogFile::flush()
{
    if (mutex_)
    {
        std::lock_guard<std::mutex> lock(*mutex_);
        file_->flush();
    }
    else
    {
        file_->flush();
    }
}

// 线程不安全地写入日志
void LogFile::append_unlocked(const char *logline, int len)
{
    file_->append(logline, len); // 写入日志 调用的File_的append(这个append调用的是fwrite_unlock)

    if (file_->writenBytes() > rollSize_)
    {
        rollFile(); // 日志滚动
    }
    else
    {
        ++count_; // 计数器加1 判断是否需要checkEveryN_次写入检查一次是否滚动
        // check一次(一次check会检查滚动周期和刷新周期)
        if (count_ >= checkEveryN_)
        {
            count_ = 0;
            time_t now = ::time(nullptr);
            // 向下取天数
            time_t thisPeriod = now * kRollPerSeconds_ / kRollPerSeconds_;
            // check
            if (thisPeriod != startOfPeriod_)
            {
                rollFile(); // 已经过了一天了，日志滚动
            }
            else if (now - lastFlush_ > flushInterval_)
            {
                file_->flush(); // 缓存刷新
                lastFlush_ = now;
            }
        }
    }
}
// 日志滚动
bool LogFile::rollFile()
{
    time_t now = ::time(nullptr);
    std::string filename = getLogFileName(basename_, &now);
    time_t start = now * kRollPerSeconds_ / kRollPerSeconds_; // 向下取天数
    if (now > lastRoll_)
    {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;
        file_.reset(new AppendFile(basename_));
        return true;
    }
    return false;
}
// 获取日志文件名 now是传入参数
// 日志文件名格式：basename + UTC时间 + hostname + pid + .log
std::string LogFile::getLogFileName(const std::string &basename, time_t *now)
{
    std::string filename;
    filename.reserve(basename.size() + 64); // 预留空间
    filename = basename;

    char timebuf[32];
    struct tm tm;
    *now = ::time(nullptr);
    ::gmtime_r(now, &tm);                          // 把当地时间转成UTC时间
    ::strftime(timebuf, 32, "%Y%m%d-%H%M%S", &tm); // 把tm结构体的值转成对应格式的字符串写到buf里

    filename += timebuf;
    filename += "tanghao";
    filename += std::to_string(::getpid());
    filename += ".log";
    return filename;
}