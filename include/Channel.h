#pragma once
#include <functional>
#include <memory>

#include "noncopyable.h"
#include "TimeStamp.h"

//只会存在指针 所以不需要引入eventloop
class EventLoop;

class Channel : noncopyable
{
    using EventCallback = std::function<void()>;    // 事件回调
    using ReadEventCallback = std::function<void(TimeStamp)>;    // 读事件回调 比事件回调多一个时间戳

public:
    Channel(EventLoop* loop, int fd);
    ~Channel();

    
    void handleEvent(TimeStamp receiveTime); // 处理事件
    void setReadCallback(const ReadEventCallback& cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(const EventCallback& cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(const EventCallback& cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(const EventCallback& cb) { errorCallback_ = std::move(cb); }

    void tie(const std::shared_ptr<void>&); // 把channel和eventConnection绑定。因为channel是依赖于eventconnection的，如果eventConnection被析构了，那么它的依赖就会出错

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revents) { revents_ = revents; }   // 把epoll实际返回的事件设置到channel上

    // 这下面的5个方法 是给更高层的组件调用的
    void enableReading()  { events_ |= kReadEvent; update(); }  // 读事件有效 即把channel上的读事件添加到epoll
    void disableReading() { events_ &= ~kReadEvent; update();  }    // 读事件失效 即把channel上的读事件从epoll上删除
    void enableWriting()  { events_ |= kWriteEvent; update(); }    // 写事件有效 即把channel上的写事件添加到epoll
    void disableWriting() { events_ &= ~kWriteEvent; update(); }    // 写事件失效 即把channel上的写事件从epoll上删除
    void disableAll()     { events_ = kNoneEvent; update(); }    // 直接关闭channel上的所有事件监听

    // channel状态查看
    bool isNoneEvent() const { return events_ == kNoneEvent; }    
    bool isWriting() const { return events_ & kWriteEvent; }    
    bool isReading() const { return events_ & kReadEvent; }    
    
    int index() const { return index_; }    // 返回channel状态
    void setIndex(int index) { index_ = index; }    // 设置channel状态

    EventLoop* ownerLoop() const { return loop_; }    // the main loop of channel
    void remove();    // 把channel从循环里删除 这个方法是eventloop调用的

private:

    void update();    // channel调用的update是通过loop调用其update的，但loop的update是通过poller调用update的 这其实就是Reactor调用demultiplex（eventloop调用poller）
    void handleEventWithGuard(TimeStamp receiveTime);    // 保证事件回调的安全

    
    // epoll_ctl的参数
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_; // main loop
    int fd_;
    int events_;    // the events of channel(registered by user)    EPOLLIN | EPOLLOUT
    int revents_;    // the events of channel(detected by kernel)    EPOLLIN | EPOLLOUT
    int index_;    // the status of channel in poller. -1 means not in poller. 1 means in poller. 2 means removed from poller.

    std::weak_ptr<void> tie_;    // 指向eventconnection的弱指针
    bool tied_;    // whether the channel is tied to a object
    bool eventHandling_;    // whether the channel is handling event
    bool addedToLoop_;    // whether the channel is added to main loop

    //channel上的事件回调 
    // channel的回调都是TcpConnection给的
    ReadEventCallback readCallback_;    // the callback of read event
    EventCallback writeCallback_;    // the callback of write event
    EventCallback closeCallback_;    // the callback of close event
    EventCallback errorCallback_;    // the callback of error event
};