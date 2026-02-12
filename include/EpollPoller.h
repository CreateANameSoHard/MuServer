#pragma once
#include <vector>
#include <sys/epoll.h>

#include "Poller.h"
#include "TimeStamp.h"

class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop *loop);
    ~EpollPoller() override;
    TimeStamp poll(int timeoutMs, ChannelList* activeChannels) override;    //epoll_wait
    void updateChannel(Channel *channel) override;    //epoll_ctl   EPOLL_CTL_MOD
    void removeChannel(Channel *channel) override;    //epoll_ctl EPOLL_CTL_DEL

private:
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;  //将epoll_wait返回的事件放入activeChannels中
    void update(int operation, Channel* channel);     //updatechannel的实际调用 Channel.update() -> loop->poller->epollpoller.updateChannel()->update()

    static constexpr int kInitEventListSize = 16;   //初始化events_数组的大小 k开头的变量为const常量

    using EventList = std::vector<epoll_event>;
    int epollfd_;
    EventList events_;  // 用于存放epoll_event元素的数组.因为要考虑到扩容，所以使用vector
};