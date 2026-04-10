#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <functional>
#include <assert.h>

#include "../include/EventLoop.h"
#include "../include/logger.h"
#include "../include/CurrentThread.h"
#include "../include/Channel.h"
#include "../include/Poller.h"

thread_local EventLoop *t_loopInThisThread = nullptr; // 防止一个线程创建多个eventloop对象。每个线程都是所以加thread_local

constexpr int kPollTimeMs = 10000; // 超时时间

// 用于创建wakeupfd，用于通知和唤醒subReactor
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (evtfd < 0)
    {
        LOG_FATAL << "eventfd error:" << errno << "\n";
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      timerqueue_(new TimerQueue(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      currentActiveChannel_(nullptr),
      callingPendingFunctors_(false)

{
    LOG_DEBUG << "EventLoop created in thread " << threadId_ << "\n";
    if (t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread << " exists in this thread " << threadId_ << "\n";
    }
    else
    {
        t_loopInThisThread = this;
    }
    // 用于与poller通信的channel
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this)); // evtloop默认就有一个channel(wakeupchannel)，用于唤醒poller
    wakeupChannel_->enableReading();
}

// 清理资源：释放wakeupFd，t_loop指针置空
EventLoop::~EventLoop()
{
    assert(!looping_);
    wakeupChannel_->disableAll(); // 这里一定要disableall，因为updateChannel是根据event_和index来删除的
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

// 一旦主Reactor唤醒subReactor（向evtfd里写），那么subReactor从evtfd里读内容，从poll返回，执行functor
void EventLoop::handleRead()
{
    uint64_t one;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one); // 读evtfd，会让fd直接清零
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::handleRead() reads %zd bytes instead of 8" << n;
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    // 如果当前线程不是EventLoop所在线程或正在执行Functor，则唤醒EventLoop
    // 如果线程正在执行回调，为了防止执行完毕后再次阻塞，所以需要wakeup(muduo为LT模式，只要写了，再进入poll后还是会触发返回)
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

// 如果在当前线程调用，直接执行。如果不在（被其他线程调用），则放在对象的pending列表里，等到loop被唤醒后再执行
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

// 其实是Eventloop与其poller建立了eventfd通信，也就是说是父组件唤醒子组件
void EventLoop::wakeup() // 唤醒事件循环
{
    uint64_t one = 1;
    size_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::wakeup() writes %zd bytes instead of 8" << n;
    }
}
void EventLoop::updateChannel(Channel *channel) // 更新channel的状态
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel) // 从活跃channel列表中移除channel
{
    poller_->removeChannel(channel);
}
void EventLoop::hasChannel(Channel *channel) // 判断channel是否活跃
{
    poller_->hasChannel(channel);
}

// 处理待执行的回调函数
void EventLoop::doPendingFunctors()
{
    /*
        为什么创建局部的functors?
        1. 减少锁的持有时间 如果直接在pendingFunctors_上遍历执行，需要在整个执行期间都持有锁,但是通过swap将pendingFunctors_的内容转移到局部变量，只需持锁很短时间
        2. 避免死锁 回调函数可能会再次调用queueInLoop()，而queueInLoop()也需要获取同一个锁,这就会导致死锁
        3. 保证公平性和避免饥饿 执行期间可能有新回调加入,新加入的回调可能永远执行不到，因为循环条件在变化
    */
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_); // 通过交换容器来减少持有锁的时间
    }

    for (const Functor &cb : functors)
    {
        cb();
    }
    callingPendingFunctors_ = false;
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
    LOG_INFO << "EventLoop %p starts loop\n"
             << this;
    while (!quit_)
    {
        activeChannels_.clear();
        // 因为线程可能会阻塞在epoll_wait上面，所以通过eventfd的方式来让epoll_wait立刻返回
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        
        currentActiveChannel_ = nullptr;
        // 发生了事件
        for (Channel *channel : activeChannels_)
        {
            currentActiveChannel_ = channel;
            // 处理事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行待执行的回调函数
        doPendingFunctors();
    }
    LOG_INFO << "EventLoop"
             << this << " stops looping\n";
    looping_ = false;
}

// 如果非loop线程调用quit，要先wakeup本loop再因while(!quit)退出。
void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}
