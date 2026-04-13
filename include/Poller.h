#pragma once
#include <vector>
#include <unordered_map>

#include "noncopyable.h"
#include "TimeStamp.h"


class Channel;
class EventLoop;

//这是一个抽象类，
class Poller:noncopyable
{
    
public:
    // channellist不是poller的组件，是Eventloop的组件
    // 维护线程全部channel的容器
    using ChannelList = std::vector<Channel*>;
    // k: fd, v: channel
    // map才是poller维护的channel容器
    using ChannelMap = std::unordered_map<int, Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller();

    virtual TimeStamp poll(int timeoutMs, ChannelList* activeChannels) = 0;   // activeChannels为传出参数
    virtual void updateChannel(Channel* channel) = 0;  // 更新channel 它是被eventloop调用的
    virtual void removeChannel(Channel* channel) = 0;  // 在poller中删除channel

    bool hasChannel(Channel* channel) const;  // check if the channel is in the channels_ map

    static Poller* newDefaultPoller(EventLoop* loop);  // 创建默认的poller。注意，poller为抽象类，是要避免包含具体类的内容的，所以这个函数单开一个文件实现

protected:
    ChannelMap channels_;  //向epoll注册过了的channel（不一定在epoll树上）eventloop的channellist维护着所有的channel
private:
    EventLoop* ownerloop_; 
};