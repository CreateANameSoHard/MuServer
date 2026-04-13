#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "../include/EpollPoller.h"
#include "../include/Channel.h"
#include "../include/logger.h"

/*
    muduo通过三种状态来标记channel(index)，通过三种状态和channel的目标事件，更新epoll树的状态，从而实现自动化的事件驱动模型。
*/
constexpr int kNew = -1;    // 新连接, 不在ChannelMap中
constexpr int kAdded = 1;   // 连接已添加到epoll中
constexpr int kDeleted = 2; // 连接已从epoll中删除,但还在ChannelMap中.

using ChannelList = std::vector<Channel *>;

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop),
      epollfd_(epoll_create1(EPOLL_CLOEXEC)), // 创建epoll句柄, EPOLL_CLOEXEC表示在fork()时关闭原有句柄
      events_(kInitEventListSize)             // 初始化事件列表
{
    if (epollfd_ < 0)
    {
        LOG_FATAL << "create epoll failed in " <<  __FUNCTION__;
    }
}

EpollPoller::~EpollPoller()
{
    close(epollfd_);
}

TimeStamp EpollPoller::poll(int timeoutMs/*超时时间*/, ChannelList *activeChannels/*传出参数 存放活跃的channel*/)
{
    //实际的日志调用应该避免出现在经常被调用的地方,否则会影响性能
    LOG_INFO << "function=" << __FUNCTION__ << channels_.size();
    //这里很巧妙。因为epoll_wait要传入epoll_event的地址，但我们用的是vector，所以传vector首元素的地址 
    //timeout -1阻塞 0 非阻塞
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int savedErrno = errno; //errno是thread_local的 saveErrno保存的是epoll_wait执行完后的errno，为的是用LOG_ERROR输出epoll_wait错误原因
    TimeStamp now(TimeStamp::now());    //epoll_wait执行完后立马记录时间

    // 返回的事件数大于0
    if (numEvents > 0)
    {
        LOG_INFO << "function=" << __FUNCTION__ << numEvents;
        fillActiveChannels(numEvents, activeChannels); // 把事件通过传入的指针返回过去

        //扩容
        if(numEvents == static_cast<int>(events_.size()))   //size()的返回值是size_t类型，可能会溢出
            events_.resize(events_.size() * 2);
    }
    else if(numEvents == 0) //epoll_wait超时
    {
        LOG_DEBUG << "timeout! " << __FUNCTION__;
    }
    else    // numEvents < 0
    {
        //如果不是EINTR（中断信号），则为真正的错误，输出epoll_wait返回后的错误 
        //如果是EINTR的话(epoll_wait在等待时被中断)，就不是错误，不执行LOG_ERROR
        if(savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_ERROR << "epoll_wait error in " << __FUNCTION__ << ", errno=" << savedErrno;    //LOG_ERROR需要errno。通过赋值给errno来获取第一时间的错误
        }
    }
    return now;
}
/*
    1. updateChannel封装了EPOLL_CTL_*参数，使得用户通过改变Channel的监听事件就能自动触发正确的epoll操作，无需手动跟踪状态或指定操作类型
    2. 注意updatechannel只是更新了channel的状态和epoll树的状态，并没有在channels_里删除这一项,删除channels_的内容是在removeChannel里做的
    3. updateChannel通过channel感兴趣的事件，自动更新epoll树的状态，而无需手动调整epoll状态，简化了操作

    三个状态之间来回转化
*/
void EpollPoller::updateChannel(Channel *channel)
{
    LOG_INFO << "fd=" <<  channel->fd() << __FUNCTION__;
    const int index = channel->index();
    // 新连接或已删除的连接.状态为kDeleted的Channel保留在ChannelMap中,避免频繁的epoll_ctl系统调用
    if (index == kNew || index == kDeleted)
    {
        int fd = channel->fd();
        if (index == kNew)
        {
            assert(channels_.find(fd) == channels_.end()); // 新连接的fd不应该在ChannelMap中
            channels_[fd] = channel;
        }
        else
        {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->setIndex(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    // 已添加到epoll中的连接
    else
    {
        int fd = channel->fd();
        assert(index == kAdded);
        assert(channels_.find(fd)!= channels_.end());
        assert(channels_[fd] == channel);
        // channel没有感兴趣的事件 从epoll树上删掉
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->setIndex(kDeleted);
        }
        // channel有感兴趣的事件 修改
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

/*
有三件事：
1. 在epoll中删掉这个channel
2. 设置channel状态 
3. channel在channelmap里，删掉
*/
void EpollPoller::removeChannel(Channel *channel)
{
    LOG_INFO << "fd=" <<  channel->fd() << __FUNCTION__;
    const int fd = channel->fd();

    assert(channels_.find(fd)!= channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());

    const int index = channel->index();
    assert(index == kAdded || index == kDeleted);   //都在channels_里
    size_t n = channels_.erase(fd);
    assert(n == 1);
    (void)n;    // 避免未使用变量报警

    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->setIndex(kNew);
}

//这个activechannel是从Eventloop传来的，用于存放活跃的channel
void EpollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    LOG_INFO << "function=" << __FUNCTION__;
    for(int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        // LOG_INFO("fd=%d, event=%d", channel->fd(), events_[i].events);
        //channel发生了事件，所以要告知给它（设置revent）
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

// 向epoll中添加或修改连接 epoll_ctl_add or epoll_ctl_mod
void EpollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    memset(&event, 0, sizeof event);
    /*
        为什么要在update里绑定channel和epoll_event?
        fillactiveChannels里用到events_[i].data.ptr来定位channel对象，而这里的event.data.ptr指向channel对象，
    */
    event.data.ptr = channel;   // 指向Channel对象. 用于在回调中定位Channel对象
    event.events = channel->events();   //根据Channel的event来更新其状态

    int fd = channel->fd();

    LOG_INFO << "epoll_ctl op= " << operation << " fd=" << fd << " event=" << channel->events();

    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR << "epoll_ctl_del error, fd=%d" << fd;    // 删除连接失败, 记录日志
        }
        else
        {
            LOG_FATAL << "epoll_ctl op=%d, fd=%d failed in %s" << operation << fd << __FUNCTION__;    // 其他错误(read or write), 退出程序
        }
    }
}