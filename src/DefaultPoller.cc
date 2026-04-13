#include <stdlib.h>

#include "../include/Poller.h"
#include "../include/EpollPoller.h"
/*
    抽象类poller的newDefaultPoller方法
    因为该方法涉及到具体的实现，所以要把它单独作为一个公共工具文件来实现
*/


Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    // 根据环境变量来决定用什么事件驱动
    if(::getenv("MUDUO_USE_POLL"))
    {
        return nullptr;
    }
    else
    {
        return new EpollPoller(loop);
    }
}
