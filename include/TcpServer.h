#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <atomic>

#include "Acceptor.h"
#include "InetAddress.h"
#include "EventloopThreadPool.h"
#include "Callbacks.h"

class TcpServer : noncopyable
{
    // EventloopThread类里定义的回调。在线程初始化时的调用
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>; // 维护所有的Connection

public:
    enum Option : bool
    {
        kNoReusePort,
        kReusePort
    };
    TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option = Option::kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; } // 给acceptor的newConnectionCallback
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }
    void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }

    EventLoop* getLoop() const {return loop_;}

    void setThreadNum(int numThreads);
    void start(); // 开启服务器监听。调用Acceptor的listen

    std::string name() const { return name_; }
    std::string ipPort() const {return ipPort_;}

private:
    void newConnection(int sockfd, const InetAddress &addr);   // 新连接的处理
    void removeConnection(const TcpConnectionPtr &conn);       // 移除连接
    void removeConnectionInLoop(const TcpConnectionPtr &conn); // 移除连接的线程安全版本

    EventLoop *loop_;                                 // acceptor_的EventLoop 用户定义的
    const std::string ipPort_;                        // ip和端口号
    const std::string name_;                          // 服务器名称
    std::unique_ptr<Acceptor> acceptor_;              // acceptor_对象
    std::shared_ptr<EventLoopThreadPool> threadPool_; // 线程池对象

    ConnectionCallback connectionCallback_;       // 连接回调
    MessageCallback messageCallback_;             // 消息回调
    WriteCompleteCallback writeCompleteCallback_; // 写完成回调
    CloseCallback closeCallback_;                 // 关闭回调
    ThreadInitCallback threadInitCallback_;       // 线程初始化回调

    std::atomic_int started_;   // 是否已经启动
    int nextConnId_;            // 下一个连接的id
    ConnectionMap connections_; // 连接集合
};