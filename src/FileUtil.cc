#include <string>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <sys/stat.h>
#include <fcntl.h>

#include "../include/FileUtil.h"

AppendFile::AppendFile(std::string filename)
    : fp_(::fopen(filename.c_str(), "ae")),
      writenBytes_(0)
{
    // 调整用户缓冲区的大小 这个buffer不用我们手动管理，是由系统管理的
    ::setbuffer(fp_, buffer_, sizeof(buffer_));
}

AppendFile::~AppendFile()
{
    ::fclose(fp_);
}

size_t AppendFile::write(const char *logline, size_t len)
{
    return ::fwrite_unlocked(logline, 1, len, fp_);
}

void AppendFile::append(const char *logline, size_t len)
{
    size_t writen = 0;
    while (writen < len)
    {
        size_t remain = len - writen;
        size_t n = write(logline + writen, remain);
        if (n != remain)
        {
            int err = ferror(fp_);
            if (err)
            {
                std::cerr << "append failed";
                break;
            }
        }
        writen += n;
    }
    writenBytes_ += writen;
}

void AppendFile::flush()
{
    ::fflush(fp_);
}

ReadSmallFile::ReadSmallFile(std::string filename)
    : fd_(::open(filename.c_str(), O_RDONLY | O_CLOEXEC)),
      err_(0)
{
    buffer_[0] = '\0';
    if (fd_ < 0)
    {
        err_ = errno;
    }
}
ReadSmallFile::~ReadSmallFile()
{
    if (fd_ >= 0)
    {
        ::close(fd_);
    }
}
// 这里的content是传出参数，从fd读出来的内容被存在里面
// maxSize是限制读出的最大字节数, 保证内存的安全
int ReadSmallFile::readToString(int maxSize,
                                std::string *content,
                                int64_t *fileSize,
                                int64_t *modifyTime,
                                int64_t *createTime)
{
    int err = err_;
    if (fd_ >= 0)
    {
        content->clear();
        if (fileSize)
        {
            struct stat statbuf;
            if (fstat(fd_, &statbuf) == 0)
            {
                if (S_ISREG(statbuf.st_mode))
                {
                    *fileSize = statbuf.st_size;
                    content->reserve(int(std::min(maxSize, int(*fileSize))));
                }
                else if (S_ISDIR(statbuf.st_mode))
                {
                    err = EISDIR;
                }   
                         
                if (statbuf.st_mtime)
                {
                    *modifyTime = statbuf.st_mtime;
                }
                if (statbuf.st_ctime)
                {
                    *createTime = statbuf.st_ctime;
                }
            }
            else
            {
                err = errno;
            }
        }
        while(content->size() < size_t(maxSize))
        {
            size_t toRead = std::min(maxSize - content->size(), sizeof(buffer_));
            size_t n = ::read(fd_, buffer_, toRead);
            if(n > 0)
            {
                content->append(buffer_);
            }
            else
            {
                if(n < 0)
                {
                    err = errno;
                }
                break;
            }
        }
    }
    return err;
}

int ReadSmallFile::readToBuffer(int* size)
{
    int err = err_;
    if(fd_ >= 0)
    {
        // 从fd的offset为0的地方读sizeof(buffer)-1个字节，读到buffer里
        ssize_t n = ::pread(fd_, buffer_, sizeof(buffer_) - 1, 0);
        if(n > 0)
        {
            if(size)
            {
                *size = n;
                buffer_[n] = '\0';
            }
        }
        else
        {
            err = errno;
        }
    }
    return err;
}