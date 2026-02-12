#pragma once
#include <vector>
#include <unordered_map>

#include "noncopyable.h"
#include "TimeStamp.h"


class Channel;
class EventLoop;


class Poller:noncopyable
{
    
public:
    // channellist不是poller的组件，是Eventloop的组件
    using ChannelList = std::vector<Channel*>;
    // k: fd, v: channel
    using ChannelMap = std::unordered_map<int, Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller();

    virtual TimeStamp poll(int timeoutMs, ChannelList* activeChannels) = 0;   // poll the channels
    virtual void updateChannel(Channel* channel) = 0;  // update the channel . Called in Channel through by loop,execuate in poller
    virtual void removeChannel(Channel* channel) = 0;  // remove the channel

    bool hasChannel(Channel* channel) const;  // check if the channel is in the channels_ map

    static Poller* newDefaultPoller(EventLoop* loop);  // create a new poller. used by loop(singleton)

protected:
    ChannelMap channels_;  //向epoll注册过了的channel（不一定在epoll树上）eventloop的channellist维护着所有的channel
private:
    EventLoop* ownerloop_; 
};