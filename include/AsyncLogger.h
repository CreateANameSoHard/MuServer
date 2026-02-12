#pragma once

#include <vector>
#include <string>
#include <atomic>
#include <memory>
#include <condition_variable>
#include <mutex>

#include "logstream.h"
#include "noncopyable.h"
#include "Thread.h"

// 异步日志
class AsyncLogger
{
    public:
        AsyncLogger(const std::string& basename, off_t rollSize, int flushInterval = 3);
        ~AsyncLogger()
        {
            if(running_)
            {
                stop();
            }
        }

        void append(const char* logline, int len); // 日志追加接口
        
        void start()
        {
            running_ = true;
            thread_.start();
        }
        void stop()
        {
            running_ = false;
            cond_.notify_all(); // 通知线程退出
            thread_.join();
        }
        
    private:
        void threadFunc(); // 后端线程函数 在muduo的线程模型中，线程只负责执行，执行的具体内容是由其他组件定义的

        using Buffer = FixedBuffer<kLargeBuffer>; // 日志缓冲区 4000*1000B
        using BufferVector = std::vector<std::unique_ptr<Buffer>>; // 日志缓冲区向量 存储已经放满了的缓冲区（用于前后端交换）
        using BufferPtr = BufferVector::value_type; // 日志缓冲区指针 指向日志缓冲区的指针 这里的value_type是vector存储的元素类型。stl的vector存储的元素是有一个类的

        // 这几个参数是给logfile用的
        const int flushInterval_; // 刷新间隔
        std::atomic_bool running_; // 运行状态
        const std::string basename_; // 日志文件名
        const off_t rollSize_; // 日志滚动大小

        Thread thread_; // 后端线程。用于把日志写到磁盘上

        std::mutex mutex_; // 互斥锁
        std::condition_variable cond_; // 条件变量

        BufferPtr currentBuffer_; // 主缓冲区指针
        BufferPtr nextBuffer_; // 次缓冲区指针 这个缓冲是作备份用的，如果日志量很大可以提高性能
        BufferVector buffers_; // 这个向量是缓存已经用完的缓冲区，前后端通过这个缓冲区来交换
};