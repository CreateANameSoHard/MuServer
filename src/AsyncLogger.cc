#include <strings.h>
#include <mutex>
#include <assert.h>
#include <chrono>

#include "../include/AsyncLogger.h"
#include "../include/LogFile.h"
#include "../include/TimeStamp.h"

AsyncLogger::AsyncLogger(const std::string &basename, off_t rollSize, int flushInterval)
    
    :flushInterval_(flushInterval),
    running_(false),
     basename_(basename),
      rollSize_(rollSize),
      thread_(std::bind(&AsyncLogger::threadFunc, this)),
      
      mutex_(),
      
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_()
{
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16); // 预分配16个缓冲区
}

void AsyncLogger::append(const char *logline, int len)
{
    std::lock_guard<std::mutex> lock(mutex_); // 加锁 前后端是共用一个锁的

    // 缓冲区剩余的空间足够
    if(currentBuffer_->avail() > len) 
    {
        currentBuffer_->append(logline,len);
    }
    // 剩余的空间不够
    else
    {
        // 把用满了的缓冲添加到集合里
        buffers_.emplace_back(std::move(currentBuffer_));
        // 如果有备用的缓冲区 则先用备用缓冲
        if(nextBuffer_)
        {
            currentBuffer_ = std::move(nextBuffer_);
        }
        // 没有备用的缓冲了
        else
        {
            // 让currentBuffer指向一个新的buffer
            currentBuffer_.reset(new Buffer);
        }
        currentBuffer_->append(logline,len);
        cond_.notify_one(); // 通知后端线程
    }
}
// 后端线程函数 
// 前后端有一对同步关系 前端写数据到缓冲，后端交换缓冲，并把内容写到文件里
void AsyncLogger::threadFunc()
{
    LogFile Output(basename_, rollSize_, false);

    // 后端也有两个缓冲区，原因和前端一样
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);

    newBuffer1->bzero();
    newBuffer2->bzero();

    BufferVector buffersToWrite;
    buffersToWrite.reserve(16); // 预分配16个缓冲区

    while(running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        // 拿锁，交换缓冲区
        {
            std::unique_lock<std::mutex> lock(mutex_); // 加锁
            // 常规的条件变量是while + wait。但这里不是，如果超时就直接出去了
            // 固定flushInterval的间隔刷新到日志里
            if(buffers_.empty())
            {
                cond_.wait_for(lock, std::chrono::seconds(flushInterval_));
            }
            buffers_.emplace_back(std::move(currentBuffer_)); // 把前端缓冲区放到完成列表里
            buffersToWrite.swap(buffers_); // 交换两端的完成列表，这样前端的列表就是空的了

            // 
            currentBuffer_ = std::move(newBuffer1);
            if(!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }


        assert(!buffersToWrite.empty());


        // 缓存的日志长度大于临界值了
        if(buffersToWrite.size() > 25)
        {
            char buf[256];
            snprintf(buf, sizeof buf, "Dropped log messages at %s,%zd larger buffers\n", TimeStamp::now().toString().c_str(), buffersToWrite.size() - 2);
            fputs(buf, stderr);
            Output.append(buf, strlen(buf));
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end()); // 保留前两个缓冲区，剩下的全部丢掉
        }

        // 写日志文件
        for(const auto& buffer:buffersToWrite)
        {
            Output.append(buffer->data(), buffer->length());
        }

        // 写完之后，需要还原完成列表的大小。因为可能会出错
        if(buffersToWrite.size() > 2)
            buffersToWrite.resize(2);


        // 后端把日志数据都写完了，所以需要还原两个缓冲区
        if(!newBuffer1)
        {
            // 这里判断的是buffersToWrite里是否还有两个空的缓冲区。缓冲区的数据已经被写到日志文件里了
            assert(!buffersToWrite.empty());
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }
        // 一样的
        if(!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }
        buffersToWrite.clear(); // 清空完成列表
        Output.flush();
    }
    Output.flush();
}