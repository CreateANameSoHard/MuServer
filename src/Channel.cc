#include <sys/epoll.h>
#include <assert.h>

#include "../include/Channel.h"
#include "../include/EventLoop.h"
#include "../include/logger.h"

 const int Channel::kNoneEvent = 0;
 const int Channel::kReadEvent = EPOLLIN | EPOLLPRI; // EPOLLPRI is for urgent data
 const int Channel::kWriteEvent = EPOLLOUT;

 Channel::Channel(EventLoop* loop, int fd)
     : loop_(loop),
       fd_(fd),
       events_(0),
       revents_(0),
       index_(-1),
       tied_(false),
       eventHandling_(false),
       addedToLoop_(false)
{}

Channel::~Channel()
{
    // assert(!eventHandling_);
    // assert(!addedToLoop_);
    // // whehter the channel belongs to this loop and in this thread  
    // if (loop_->IsInLoopThread())
    // {
    //     assert(!loop_->hasChannel(this));
    // }
}

//用于防止对象在事件处理期间被意外销毁 
// tie连接的是Tcpconnection。为了防止TcpConnection对象关闭后，Channel的回调产生未定义行为
void Channel::tie(const std::shared_ptr<void>& obj)
{
    tied_ = true;
    tie_ = obj;
}

// call poller to update channel by loop object
void Channel::update()
{
    addedToLoop_ = true;
    loop_->updateChannel(this); //call poller to update channel
}

// call poller to remove channel by loop object
void Channel::remove()
{
    addedToLoop_ = false;
    loop_->removeChannel(this); //call poller to remove channel
}    

void Channel::handleEvent(TimeStamp receivedTime)
{
    std::shared_ptr<void> guard;
    if(tied_)
    {
        guard = tie_.lock();
        // 如果channel指向的TcpConnection已经销毁了，那么就不会调用handleEventWithGuard了，就保证了安全（因为回调事件都属于TcpConnection）
        if(guard)
        {
            handleEventWithGuard(receivedTime);
        }
    }
    else
    {
        handleEventWithGuard(receivedTime);
    }
}

void Channel::handleEventWithGuard(TimeStamp receivedTime)
{
    LOG_INFO << "channel received event:" << revents_;
    eventHandling_ = true;
    // 关闭事件 读事件不能算
    if( (revents_ & EPOLLHUP) && !(revents_ & EPOLLIN) )
    {
        if(closeCallback_)
        {
            closeCallback_();
        }
    }
    // 错误事件 
    if(revents_ & EPOLLERR)
    {
        if(errorCallback_)
        {
            errorCallback_();
        }
    }
    // 可读事件 
    if(revents_ & (EPOLLIN | EPOLLPRI))
    {
        if(readCallback_)
        {
            readCallback_(receivedTime);
        }
    }
    // 可写事件 
    if(revents_ & EPOLLOUT)
    {
        if(writeCallback_)
        {
            writeCallback_();
        }
    }

}
