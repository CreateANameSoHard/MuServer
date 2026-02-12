#pragma once
#include <functional>

#include "noncopyable.h"
#include "InetAddress.h"
#include "Socket.h"
#include "Channel.h"

class EventLoop;

class Acceptor : noncopyable
{
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress& addr)>;
public:
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort); //TcpServer的参数为loop,listenAddr，name
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    {
        newConnectionCallback_ = std::move(cb);
    }
    bool listenning () const {return listening_;}

    void listen(); //创建监听队列

private:
    void handleRead(); //处理新连接 这个其实就是给subReactor执行的pendingFunctor

    EventLoop* loop_; //Acceptor是Reactor的子组件，用的是用户定义的baseloop，即mainReactor
    Socket acceptSocket_; //用于监听新连接的Socket
    Channel acceptChannel_; //用于监听新连接的Channel
    NewConnectionCallback newConnectionCallback_; //用于处理新连接的回调函数 这个就是给subReactor的functor
    bool listening_; //是否正在监听中
};