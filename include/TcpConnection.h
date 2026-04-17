#pragma once
#include <memory>
#include <atomic>
#include <string>
#include <deque>
#include <sys/sendfile.h>
#include <unistd.h>
#include <fcntl.h>

#include "noncopyable.h"
#include "Any.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "TimeStamp.h"
#include "logger.h"
#include "StaticFile.h"

class Channel;
class EventLoop;
class Socket;

// 这个对象就是在mainReactor和subReactor直接传递的对象。封装了socket和channel
// 代表着已经建立链接的一条链路 channel和对端就是通过connection传递消息的

class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection>
{
    enum StateE
    {
        kDisconnected,
        kDisconnecting,
        kConnecting,
        kConnected
    }; // 状态枚举类型
public:
    TcpConnection(EventLoop *loop, const std::string &name, int sockfd,
                  const InetAddress &localAddr, const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() const { return localAddr_; }
    const InetAddress &peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }
    bool disconnected() const { return state_ == kDisconnected; }

    void send(const void *data, size_t len);
    void send(const std::string &message);
    void send(Buffer *buffer);

    void sendFile(const StaticFile&);

    void shutdown(); // 关闭链接

    void connectEstablished(); // 连接建立
    void connectDestroyed();   // 连接销毁

    const Any &getContext() const { return context_; }
    Any &getMutableContext() { return context_; }
    // 触发拷贝 临时对象是ok的
    void setContext(const Any &context) { context_ = std::move(context); }

    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback &cb)
    {
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }
    void setCloseCallback(const CloseCallback &cb)
    {
        closeCallback_ = cb;
    }
    void setHighWaterMark(const HighWaterCallback &cb, size_t highWaterMark)
    {
        highWaterCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

private:
    void handleRead(TimeStamp receiveTime); // 读事件处理
    void handleWrite();                     // 写事件处理
    void handleError();                     // 错误事件处理
    void handleClose();                     // 关闭事件处理

    // void sendInLoop(const std::string &message);

    void shutdownInLoop(); // 关闭链接

    void setStates(const StateE s) { state_ = s; } // 设置状态

    EventLoop *loop_; // 这个loop指针指向的不是mainloop，而是subloop
    const std::string name_;
    std::atomic_int state_; // 状态变量

    Any context_; // Tcp链接的上下文 指向上层协议 这里仅用于指向httpContext

    bool reading_; // 是否正在读

    std::unique_ptr<Socket> socket_;   // 封装的socket对象
    std::unique_ptr<Channel> channel_; // 封装的channel对象 通过这个对象把相关的事件处理交给对应的loop

    const InetAddress localAddr_; // 本端地址
    const InetAddress peerAddr_;  // 对端地址

    ConnectionCallback connectionCallback_;       // 连接回调 成功会回调 失败也会回调
    MessageCallback messageCallback_;             // 消息回调
    WriteCompleteCallback writeCompleteCallback_; // 写完成回调
    CloseCallback closeCallback_;                 // 关闭回调
    HighWaterCallback highWaterCallback_;         // 高水位回调

    size_t highWaterMark_; // 高水位标记

    Buffer inputBuffer_; // 输入缓冲区

    class Context
    {
    public:
        virtual size_t writeToSocket(int) = 0;
        virtual ~Context() = default;
        virtual bool isComplete() const = 0;
        virtual size_t remaining() const = 0;
    };

    class DataContext : public Context
    {
    public:
        DataContext(const void *data, size_t len)
            : data_(static_cast<const char *>(data), static_cast<const char *>(data) + len), 
              pos_(0)
        {
        }

        size_t writeToSocket(int sockfd) override
        {
            ssize_t n = ::write(sockfd, data_.data() + pos_, data_.size() - pos_);
            if (n > 0)
            {
                pos_ += n;
                return n;
            }
            return n; // 可能返回 -1，错误码在外层处理
        }

        bool isComplete() const override { return pos_ == data_.size(); }
        size_t remaining() const { return data_.size() - pos_; }

    private:
        std::vector<char> data_; // 或者 std::string
        size_t pos_;
    };

    class FileContext : public Context
    {
    public:
        using writeCompleteCallback = std::function<void(int)>;
        int openFileOrDie(const StaticFile& file)
        {
            int fd = ::open(file.path().c_str(), O_RDONLY);
            if(fd < 0)
            {
                LOG_FATAL << "open file error ";
            }
            return fd;
        }

        FileContext(const StaticFile& file)
            : fd_(openFileOrDie(file)), 
            offset_(file.getOffset()), 
            initialOffset_(file.getOffset()), 
            len_(file.getFileSize())
        {
             LOG_INFO << "FileContext ctor: fd=" << fd_ 
             << " offset=" << offset_ 
             << " initialOffset=" << initialOffset_
             << " len=" << len_;
        }
        ~FileContext()
        {
            if(close(fd_) != 0)
                LOG_ERROR << "close error!";
        }

        bool isComplete() const override
        {
            return (offset_ - initialOffset_) == static_cast<off_t>(len_);
        }
        size_t remaining() const override
        {
            return len_ - (offset_ - initialOffset_);
        }
        size_t writeToSocket(int sockfd) override
        {
            size_t remain = len_ - (offset_ - initialOffset_);
            ssize_t n = ::sendfile(sockfd, fd_, &offset_, remain);
            return n;
        }

    private:
        int fd_;
        off_t offset_;
        off_t initialOffset_;
        size_t len_;
    };

    std::deque<std::unique_ptr<TcpConnection::Context>> contextQueue_;
    size_t totalPendingBytes_;

    void sendInLoop(const void *data, size_t len); // 发送数据到loop中
    void sendFileInLoop(const StaticFile& file);
    bool highWaterMarkTriggered_; // 标记是否触发高水位回调
    void doWriting();
};