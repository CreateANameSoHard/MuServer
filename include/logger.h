#pragma once
#include <iostream>
#include <string>
#include <stdlib.h>

#include "noncopyable.h"
#include "TimeStamp.h"
#include "logstream.h"


// 这个类只用来提供接口，其他具体的实现是写在内部类里的
class Logger : noncopyable
{
public:
    enum LogLevel
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL
    };
    // 编译时期计算出源文件的名称
    class SourceFile
    {
    public:
        // 这里是关键。通过隐式传递模板参数，能够在编译时期就获取到文件名的长度，就省去了运行时的strlen的调用
        // 另外用这个构造的其实主要是__FILE__宏。因为它就是const char的数组
        template <int N>
        SourceFile(const char (&arr)[N])
            : data_(arr),
              size_(N - 1)
        {
            const char *slash = strrchr(data_, '/');
            if (slash)
            {
                data_ = slash + 1;
                size_ -= static_cast<size_t>(data_ - arr);
            }
        }

        // 普通构造函数，传入一个字符串
        SourceFile(const char *filename)
            : data_(filename)
        {
            const char *slash = strrchr(data_, '/');
            if (slash)
            {
                data_ = slash + 1;
            }
            size_ = static_cast<size_t>(strlen(data_));
        }

        const char *data_;
        size_t size_;
    };

    // 注意这里的构造函数中，除了那个toAbort，其他的都是宏,__FILE__,__LINE__,__func__
    Logger(SourceFile file, int line);
    Logger(SourceFile file, int line, bool toAbort);
    Logger(SourceFile file, int line, LogLevel level);
    Logger(SourceFile file, int line, LogLevel level, const char *func);
    ~Logger();

    LogStream &stream() { return impl_.stream_; }

    static LogLevel logLevel();
    static void setLogLevel(LogLevel level);

    using OutputFunc = void (*)(const char *msg, size_t len); // 日志输入方式，默认为stdout，用户可自定义
    using FlushFunc = void (*)();                             // 日志刷新方式，默认为刷新stdout，用户可自定义
    static void setOutput(OutputFunc);
    static void setFlush(FlushFunc);

private:
    // 核心实现类
    class Impl
    {
        typedef Logger::LogLevel LogLevel;

    public:
        Impl(LogLevel level, int old_errno, const SourceFile &file, int line);
        void finish(); // 输出日志末尾的内容：-时间 .log

        TimeStamp time_;
        SourceFile basename_;
        int line_;
        LogLevel level_;
        LogStream stream_; // 同步日志的后端对象，有缓冲区(小缓冲，大小为4000B)
    };
    Impl impl_;
};

// 全局的日志级别对象。只要包含了这个logger.h头文件，就可以获取日志级别
extern Logger::LogLevel g_loglevel;

inline Logger::LogLevel Logger::logLevel()
{
    return g_loglevel;
}

// 当向这些宏定义里写日志时，其实就是在向stream对象（缓冲里写日志）
// 日志输出的宏定义。通过创建临时对象、日志对象销毁的方式写到目标
#define LOG_TRACE                                      \
    if (Logger::logLevel() <= Logger::LogLevel::TRACE) \
    Logger(__FILE__, __LINE__, Logger::LogLevel::TRACE, __func__).stream()

#define LOG_DEBUG                                      \
    if (Logger::logLevel() <= Logger::LogLevel::DEBUG) \
    Logger(__FILE__, __LINE__, Logger::LogLevel::DEBUG, __func__).stream()

#define LOG_INFO                                      \
    if (Logger::logLevel() <= Logger::LogLevel::INFO) \
    Logger(__FILE__, __LINE__, Logger::LogLevel::INFO, __func__).stream()
// WARN之前的级别都是调试用的，所以要输出FUNCTION
#define LOG_WARN                                      \
    if (Logger::logLevel() <= Logger::LogLevel::WARN) \
    Logger(__FILE__, __LINE__, Logger::LogLevel::WARN).stream()

#define LOG_ERROR                                      \
    if (Logger::logLevel() <= Logger::LogLevel::ERROR) \
    Logger(__FILE__, __LINE__, Logger::LogLevel::ERROR).stream()
// 这里的true表示日志输出后，程序要abort
#define LOG_FATAL                                      \
    if (Logger::logLevel() <= Logger::LogLevel::FATAL) \
    Logger(__FILE__, __LINE__, true).stream()

// 指针判空
template <typename T>
T *CheckNotNull(Logger::SourceFile file, int line, const char *names, T *ptr)
{
    if (ptr == nullptr)
    {
        Logger(__FILE__, __LINE__, Logger::LogLevel::FATAL).stream() << names;
    }
    return ptr;
}

#define CHECK_NOT_NULL(val) \
    CheckNotNull(__FILE__, __LINE__, "'#val' Must Not Be Null", (val))
