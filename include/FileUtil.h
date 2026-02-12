#pragma once
#include <string>
#include <sys/types.h>

#include "noncopyable.h"


// 读小文件用的对象
class ReadSmallFile : noncopyable
{
    public:
        ReadSmallFile(std::string filename);
        ~ReadSmallFile();

        int readToBuffer(int* size); // 读取文件到缓冲区，返回errno
        int readToString(int maxSize,
                   std::string* content,
                   int64_t* fileSize,
                   int64_t* modifyTime,
                   int64_t* createTime);     // 读取文件到字符串，返回errno

        const char* buffer() const { return buffer_; } // 获取缓冲区指针


    static const int kBufferSize = 64 * 1024; // 64KB
    private:
        int fd_; // 文件描述符
        int err_; // 错误码
        char buffer_[kBufferSize]; // 缓冲区 
};

class AppendFile : noncopyable
{
    public:
        AppendFile(std::string filename);
        ~AppendFile();

        void append(const char* logline, size_t len); // 追加日志
        void flush(); // 刷新缓冲区

        off_t writenBytes() const { return writenBytes_; } // 已写入的字节数 这里是给日志回滚计数用的！！！
    private:
        // append调用的就是这个方法。这个方法调的是fwrite_unlock(非锁版fwrite)
        size_t write(const char* logline, size_t len);

        FILE* fp_; // 文件指针
        char buffer_[64 * 1024]; // 缓冲区 64KB
        off_t writenBytes_; // 已写入的字节数
};