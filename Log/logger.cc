#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../include/logger.h"
#include "../include/CurrentThread.h"
#include "../include/TimeStamp.h"
//线程独立缓冲区
namespace ThreadInfo
{
    thread_local char t_errnobuf[512];
    thread_local char t_timer[64];
    thread_local time_t t_lastRecord;
}

const char* getErrnoMsg(int savederrno)
{
    return strerror_r(savederrno, ThreadInfo::t_errnobuf, sizeof ThreadInfo::t_errnobuf);
}

Logger::LogLevel initLogLevel()
{
    if (::getenv("LOG_TRACE"))
    {
        return Logger::TRACE;
    }
    else if (::getenv("LOG_DEBUG"))
    {
        return Logger::DEBUG;
    }
    else
    {
        return Logger::INFO;
    }
}
// 这个变量是给头文件用的
Logger::LogLevel g_loglevel = initLogLevel();

// 注意这里的字符串长度都是6
const char *logLevelName[] = {"TRACE ", "DEBUG ", "INFO  ", "WARN  ", "ERROR ", "FATAL "};

// 这个类是为了优化获取字符串长度的
class T
{
public:
    T(const char *str, unsigned int len) : str_(str), len_(len) {}
    const char *str_;
    const unsigned int len_;
};

inline LogStream &operator<<(LogStream &s, T t)
{
    s.append(t.str_, t.len_);
    return s;
}

inline LogStream &operator<<(LogStream &s, Logger::SourceFile file)
{
    s.append(file.data_, file.size_);
    return s;
}

void defaultOutput(const char *msg, size_t len)
{
    ::fwrite(msg, 1, len, stdout);
}

void defaultFlush()
{
    ::fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

// 日志格式：时间 + epoch(微妙) + tid + level + 内容 + basename + - + line
Logger::Impl::Impl(Logger::LogLevel level, int saved_errno, const SourceFile &file, int line)
    : time_(TimeStamp::now()),
      basename_(file),
      line_(line),
      level_(level),
      stream_() // stream对象的构造函数就是默认的
{
    //添加时间
    formatTime();
    std::string tid = std::to_string(CurrentThread::tid());
    stream_ << T(tid.c_str(), tid.size()) << " ";
    stream_ << T(logLevelName[level], 6);
    
    if(saved_errno != 0)
    {
        stream_ << getErrnoMsg(saved_errno) << " ";
    }
}
//格式化时间 并写入buffer
void Logger::Impl::formatTime()
{
    TimeStamp now = TimeStamp::now();
    time_t seconds = static_cast<time_t>(now.microsecondsSinceEpoch() / TimeStamp::kMicroSecondsPerSecond);
    int microSeconds = static_cast<int>(now.microsecondsSinceEpoch() % TimeStamp::kMicroSecondsPerSecond);

    tm* tm_format = localtime(&seconds);
    //写入到线程的时间缓存里
    snprintf(ThreadInfo::t_timer, sizeof ThreadInfo::t_timer, "%4d/%02d/%02d %02d:%02d:%02d",
        tm_format->tm_year + 1900,
        tm_format->tm_mon + 1, //tm_mon是从0开始的
        tm_format->tm_mday,
        tm_format->tm_hour,
        tm_format->tm_min,
        tm_format->tm_sec
    );

    ThreadInfo::t_lastRecord = seconds;

    char buf[32] = {0};
    snprintf(buf, sizeof buf, "%06d", microSeconds);

    stream_ << T(ThreadInfo::t_timer, strlen(ThreadInfo::t_timer)) << T(buf, strlen(buf));
}

void Logger::Impl::finish()
{
    stream_ << basename_ << "-" << line_ << "\n";
}

// 如果没有传level参数，就默认是INFO级别
Logger::Logger(SourceFile file, int line)
    : impl_(LogLevel::INFO, 0, file, line)
{
}
// abort是在析构里再调用的。要先把日志输出完再关闭嘛
Logger::Logger(SourceFile file, int line, bool toAbort)
    : impl_(toAbort ? FATAL : ERROR, errno, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level)
    : impl_(level, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char *func)
    : impl_(level, 0, file, line)
{
    impl_.stream_ << func << " ";
}

Logger::~Logger()
{
    impl_.finish();                                   // 日志的结尾部分 这里只是往Fixedbuffer里写，还没有输出
    const LogStream::Buffer &buf = stream().buffer(); // FixedBuffer是不可拷贝的。这里用的是LogStream的buffer方法，返回的是buffer的指针
    g_output(buf.data(), buf.length());               // 输出日志

    if (impl_.level_ == FATAL)
    {
        ::abort(); // 这里是调用abort函数，如果日志是FATAL级别，就直接abort
    }
}



void Logger::setLogLevel(LogLevel level)
{
    g_loglevel = level;
}

void Logger::setOutput(OutputFunc output)
{
    g_output = output;
}

void Logger::setFlush(FlushFunc flush)
{
    g_flush = flush;
}
