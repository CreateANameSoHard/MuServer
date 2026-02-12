#pragma once
#include <memory>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>


#include "noncopyable.h"
#include "TimeStamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;
/*
在多线程环境下，eventloop往往需要跨线程执行（比如工作线程只负责运算，不负责io），所以需要functor
*/


// event loop. contain two modules: poller(abstract for epoll, select, poll, etc.) channel(abstract for event and call)
// subReactor
class EventLoop :noncopyable
{
    using ChannelList = std::vector<Channel*>;  //这个变量维护所有的channel
    using Functor = std::function<void()>;

public:
    EventLoop();
    ~EventLoop();

    void loop(); //启动事件循环
    void quit(); //退出事件循环

    void runInLoop(Functor cb); //在事件循环中执行回调函数
    void queueInLoop(Functor cb); //将回调函数放入待执行队列

    TimeStamp pollReturnTime() const { return pollReturnTime_; } //返回上一次poll返回的时间


    void wakeup(); //唤醒事件循环

    void updateChannel(Channel* channel); //更新channel的状态
    void removeChannel(Channel* channel); //从活跃channel列表中移除channel
    void hasChannel(Channel* channel); //判断channel是否活跃

    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); } //判断当前线程是否为EventLoop所在线程 如果不在，则会在queue里等到当前线程


private:
    void handleRead(); //wakeup
    void doPendingFunctors(); //执行待执行的回调函数

    std::atomic_bool looping_; //是否正在运行
    std::atomic_bool quit_; //是否退出
    
    /*
        监听的fd由mainReactor打包成channel，交给subReactor执行。每个subReactor都有一组channel，每个channel必须在自己的subReactor里执行
        这也是为什么channel会有一个tie函数和tied_变量
    */
    const pid_t threadId_; //记录创建了eventloop对象的线程id
    TimeStamp pollReturnTime_; //记录上一次poll返回的时间
    std::unique_ptr<Poller> poller_; //事件循环 

    // 当mainloop获得一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒subloop处理channel，用的eventfd机制
    // libevent用的是socketpair机制
    int wakeupFd_; //是否为唤醒状态 很重要！！！！！ 涉及到muduo库的工作线程的负载均衡  
    std::unique_ptr<Channel> wakeupChannel_; //唤醒channel 用于主线程和子线程之间的通信和激活

    ChannelList activeChannels_; //活跃的channel列表 即fillactiveChannel的输入
    Channel* currentActiveChannel_; //当前正在处理的channel


    std::atomic_bool callingPendingFunctors_; //当前是否有需要执行的回调
    std::vector<Functor> pendingFunctors_; //待执行的回调函数列表


    std::mutex mutex_; //互斥锁 保证vector容器的线程安全
};