#include <errno.h>
#include <sys/uio.h>

#include "../include/Buffer.h"
#include "../include/logger.h"

// 从文件描述符中读数据。
// 读数据到缓冲区有一个问题。 我们不知道用户的数据有多少，所以不能直接分配缓冲大小
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    char extrabuf[65536] = {0};
    iovec iov[2];

    const size_t writeable = writableBytes();
    iov[0].iov_base = begin() + writerIndex_;
    iov[0].iov_len = writeable;

    iov[1].iov_base = extrabuf;
    iov[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writeable < sizeof extrabuf) ? 2:1; // 如果剩余的缓冲已经很大了，就不需要这个extrabuf了
    const size_t n = ::readv(fd, iov, iovcnt);

    if(n < 0)
    {
        *savedErrno = errno;
    }
    // 读到的数据少于剩余的缓冲区大小，即没有用到extrabuf
    else if(n <= writeable)
    {
        writerIndex_ += n;
    }
    // 读到的数据大于剩余的缓冲区大小，即用到了extrabuf
    else
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writeable); // 注意append是会自动扩容的(调用了ensurewriteablebytes)
    }
    return n;
}