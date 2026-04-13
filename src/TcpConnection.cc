#include <functional>
#include <errno.h>
#include <unistd.h>

#include "../include/TcpConnection.h"
#include "../include/logger.h"
#include "../include/Socket.h"
#include "../include/Channel.h"
#include "../include/EventLoop.h"

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &name,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CHECK_NOT_NULL(loop)),
      name_(name),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024) // 高水位为64MB
{
  // 把处理方法全部绑到channel上
  channel_->setReadCallback(
      std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
  channel_->setWriteCallback(
      std::bind(&TcpConnection::handleWrite, this));
  channel_->setErrorCallback(
      std::bind(&TcpConnection::handleError, this));
  channel_->setCloseCallback(
      std::bind(&TcpConnection::handleClose, this));

  LOG_INFO << "TcpConnection::ctor[" << name_ << "] at " << this;

  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
  LOG_INFO << "TcpConnection::dtor[" << name_ << "] at " << this << "fd = " << channel_->fd();
}

void TcpConnection::handleRead(TimeStamp receiveTime)
{
  int savedErrno = 0;
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
  // 对端有数据
  if (n > 0)
  {
    // 用户需要根据inputbuffer来获取对端的数据，所以这里需要传inputbuffer
    messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
  }
  // 对端关闭连接
  else if (n == 0)
  {
    handleClose();
  }
  // 出错
  else
  {
    errno = savedErrno;
    LOG_ERROR << "TcpConnection::handleRead read error";
    handleError();
  }
}
// 正常情况下我们其实根本不需要给epoll注册写事件。但当内核不能及时发送数据时，就需要向epoll注册写事件，让它及时通知我们
void TcpConnection::handleWrite()
{
  // 用户是否注册可写事件
  if (channel_->isWriting())
  {
    // readable的部分是用来发送的 把output缓冲区的数据发送出去
    ssize_t n = ::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
    if (n > 0)
    {
      outputBuffer_.retrieve(n);
      // 发送完毕
      if (outputBuffer_.readableBytes() == 0)
      {
        // 防止在没有数据可写时，epoll持续返回可写事件 只在真正需要时监听写事件
        // 会在发送数据(send)中开启对写事件的监听
        channel_->disableWriting();
        if (writeCompleteCallback_)
        {
          loop_->queueInLoop(
              std::bind(&TcpConnection::writeCompleteCallback_, shared_from_this()));
        }
        if (state_ == kDisconnecting)
        {
          shutdownInLoop();
        }
      }
    }
    else
    {
      LOG_ERROR << "TcpConnection::handleWrite write error";
    }
  }
  else
  {
    LOG_TRACE << "fd = " << channel_->fd() << " TcpConnection::handleWrite channel not writing";
  }
}

void TcpConnection::handleClose()
{
  LOG_INFO << "TcpConnection::handleClose fd = " << channel_->fd() << " state = " << state_;
  setStates(kDisconnected);
  channel_->disableAll(); // disableALl后，会调用update，自动从epoll中删除这个channel

  TcpConnectionPtr guardThis(shared_from_this());

  // 链接成功、链接断开都要调用connectionCallback
  connectionCallback_(guardThis);
  closeCallback_(guardThis);
}

void TcpConnection::handleError()
{

  int err = Socket::getSocketError(channel_->fd());
  LOG_ERROR << "TcpConnection::handleError [" << name_ << "] - SO_ERROR = " << err;
}

void TcpConnection::send(Buffer* buffer)
{
  if(state_ == kConnected)
  {
    if(loop_->isInLoopThread())
    {
      sendInLoop(buffer->peek(), buffer->readableBytes());
      buffer->retrieveAll();
    }
    else
    {
      loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buffer->peek(), buffer->readableBytes()));
    }
  }
}

void TcpConnection::send(const std::string &buf)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(buf.c_str(), buf.size());
    }
    else
    {
      loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
    }
  }
}
// 需要协调应用和内核的数据发送速度
void TcpConnection::sendInLoop(const void *data, size_t len)
{
  int nwrote = 0;
  int remaining = len;
  bool faultError = false;
  if (state_ == kDisconnected)
  {
    LOG_WARN << "disconnected, give up writing";
    return;
  }
  // epoll没有注册写事件，则直接发送给对端 因为此时缓冲区是空的，没有必要向epoll注册
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
  {
    nwrote = ::write(channel_->fd(), data, len);
    if (nwrote >= 0)
    {
      remaining = len - nwrote;
      if (remaining == 0 && writeCompleteCallback_)
      {
        loop_->queueInLoop(std::bind(&TcpConnection::writeCompleteCallback_, shared_from_this()));
      }
    }
    else
    {
      nwrote = 0;
      if (errno != EWOULDBLOCK)
      {
        LOG_ERROR << "TcpConnection::sendInLoop write error";
        // 出错，关闭连接
        if (errno == EPIPE || errno == ECONNRESET)
        {
          faultError = true;
        }
      }
    }
  }
  // 没有把全部数据都发出去，说明内核的缓冲区可能没空间了，此时就需要暂存output缓冲区，给epoll注册写事件，让它来通知我们
  // 给epoll注册的写回调也就是handlewrite，这个函数的作用就是把outputbuffer的内容发出去 当内核可以发数据时，就会把剩余的内容发出去
  if (!faultError && remaining > 0)
  {
    // 残留的数据有两部分：outputbuffer和remaining
    size_t oldLen = outputBuffer_.readableBytes();
    if (oldLen + remaining >= highWaterMark_ // 添加新数据后，总数据量将达到或超过高水位
        && oldLen < highWaterMark_           // 这个条件确保高水位回调只触发一次
        && highWaterCallback_)
    {
      loop_->queueInLoop(std::bind(highWaterCallback_, shared_from_this(), oldLen + remaining));
    }

    outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);

    if (!channel_->isWriting())
    {
      channel_->enableReading(); // 这里要注册channel的写事件 后续的发送让handleWrite来发
    }
  }
}

/*
  这里也有一点可以学习：
  如果一个对象依赖另一个对象的回调（或者其他某种资源），可以像muduo一样，把对象通过弱引用指针绑在一起。
  如果如果被依赖对象被删掉了，那么就不执行相关的回调
*/
void TcpConnection::connectEstablished()
{
  // TcpConnection初始化为kConnecting，这里因为链接建立了，所以要设为connected
  setStates(kConnected);
  // channel用弱类型指针指向TcpConnection对象。如果TcpConnection被销毁，那么Channel就不会调用handleEvent，保证安全
  // share_ptr<Concrete>可以转为share_ptr<void>就像其他类型指针可以转为void*一样
  channel_->tie(shared_from_this());

  channel_->enableReading(); 
  connectionCallback_(shared_from_this());
}
// handleClose -> removeConnection -> connectDestroyed
void TcpConnection::connectDestroyed()
{
  if (state_ == kConnected)
  {
    // 把channel从epoll里删掉
    setStates(kDisconnected);
    channel_->disableAll(); // 关闭channel，从epoll中删除
    connectionCallback_(shared_from_this());
  }

  channel_->remove();
}

/*
  shutdown的调用链为：
  1. 用户调用shutdown
  2. shutdown通过runInloop调用shutdownInloop
  3. 如果此时因发送繁忙，而导致Epollout事件被监听，就会停止shutdownInloop的执行
  4. handleWrite中，在完成剩余数据的发送后，会先关闭epollout事件的监听，并判断是否为disconnecting状态。
  5. 如果为disconnecting状态，则会调用shutdownInloop。此时必定可以关闭
  6. shutdownInloop调用shutdownWrite，关闭socket写端。
  7. 关闭写端会导致EPOLLhup事件触发（不需要注册就会监听），从而调用closeCallback，即handleClose
  8. handleClose会关闭epoll中的channel，并调用connectionCallback和CloseCallback
*/
void TcpConnection::shutdown()
{
  if (state_ == kConnected)
  {
    // 注意这里是disconnecting!
    setStates(kDisconnecting);
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop()
{
  // 只有在写完后才能关闭channel
  if (!channel_->isWriting())
  {
    //本端写关闭 即发送fin，进入半关闭状态
    socket_->shutdownWrite();
  }
}
