#include "../include/Poller.h"
#include "../include/Channel.h"

Poller::Poller(EventLoop* loop)
:ownerloop_(loop)
{}

Poller::~Poller() = default;

bool Poller::hasChannel(Channel* channel) const
{
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}


// this function should'nt write here. it return a object of poller,but poller is a abstract class.
// Poller* Poller::newDefaultPoller(EventLoop* loop)