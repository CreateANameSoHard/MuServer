#pragma once
#include <functional>
#include <memory>

#include "noncopyable.h"
#include "TimeStamp.h"

// The Principle of Least Knowledge: header file should expose the minimum necessary interface to its clients.
// so there is no need to expose the implementation details of EventLoop and TimeStamp to Channel.
class EventLoop;

/*
Reactor模式的核心思想是：“不要让用户知道底层的实现细节，而是通过暴露简单的接口来提供服务”。Event(channel)使用的接口是Reactor(eventloop)的接口，Reactor调用的是poller(demultiplex)的接口
*/
class Channel : noncopyable
{
    using EventCallback = std::function<void()>;    // The callback of event
/*
1. 性能统计和日志记录：当有数据可读时，记录下事件发生的时间，可以用于计算处理延迟、统计网络流量等。例如，我们可以记录每次读事件发生的时间，然后与数据包的时间戳对比，或者计算两次读事件的时间间隔。

2.超时处理：在实现心跳机制或连接超时时，我们需要知道上一次读到数据的时间。每次读事件发生时，我们更新这个时间，然后定时检查当前时间与上一次读事件的时间差，如果超过一定阈值，则认为连接超时。

3.定时器相关：muduo 的定时器回调也是通过读事件（timerfd）来触发的，定时器触发时需要知道当前的时间，以便执行相应的定时任务。

4.协议解析：在某些协议中，可能需要根据数据到达的时间做一些处理，比如流媒体中的同步、游戏中的帧同步等。 
*/
    using ReadEventCallback = std::function<void(TimeStamp)>;    // The callback of read event

public:
    Channel(EventLoop* loop, int fd);
    ~Channel();

    // after poller detects the event, call this function to handle it.
    void handleEvent(TimeStamp receiveTime);
    void setReadCallback(const ReadEventCallback& cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(const EventCallback& cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(const EventCallback& cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(const EventCallback& cb) { errorCallback_ = std::move(cb); }

    // this function is prevent the channel execute the callback after removed.
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revents) { revents_ = revents; }   //this function is used by poller to set the revents of channel.

    void enableReading()  { events_ |= kReadEvent; update(); }  // enable reading event 
    void disableReading() { events_ &= ~kReadEvent; update();  }    // disable reading event 
    void enableWriting()  { events_ |= kWriteEvent; update(); }    // enable writing event 
    void disableWriting() { events_ &= ~kWriteEvent; update(); }    // disable writing event 
    void disableAll()     { events_ = kNoneEvent; update(); }    // disable all events

    bool isNoneEvent() const { return events_ == kNoneEvent; }    // whether the channel has no event
    bool isWriting() const { return events_ & kWriteEvent; }    // whether the channel is writing
    bool isReading() const { return events_ & kReadEvent; }    // whether the channel is reading
    
    int index() const { return index_; }    // the index of channel in the main loop
    void setIndex(int index) { index_ = index; }    // set the index of channel in the main loop

    EventLoop* ownerLoop() const { return loop_; }    // the main loop of channel
    void remove();    // remove the channel from main loop

private:

    void update();    // channel调用的update是通过loop调用其update的，但loop的update是通过poller调用update的 这其实就是Reactor调用demultiplex（eventloop调用poller）
    void handleEventWithGuard(TimeStamp receiveTime);    // handle the event with guard

    
    // the description of event
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_; // main loop
    int fd_;
    int events_;    // the events of channel(registered by user)    EPOLLIN | EPOLLOUT
    int revents_;    // the events of channel(detected by kernel)    EPOLLIN | EPOLLOUT
    int index_;    // the status of channel in poller. -1 means not in poller. 1 means in poller. 2 means removed from poller.

    std::weak_ptr<void> tie_;    // the tie of channel
    bool tied_;    // whether the channel is tied to a object
    bool eventHandling_;    // whether the channel is handling event
    bool addedToLoop_;    // whether the channel is added to main loop

    // the callback of event.called by channel

    ReadEventCallback readCallback_;    // the callback of read event
    EventCallback writeCallback_;    // the callback of write event
    EventCallback closeCallback_;    // the callback of close event
    EventCallback errorCallback_;    // the callback of error event
};