#include <stdlib.h>

#include "../include/Poller.h"
#include "../include/EpollPoller.h"
/*
    this class is a public class, so we could include poll and epoll
*/


Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        return nullptr;
    }
    else
    {
        return new EpollPoller(loop);
    }
}
