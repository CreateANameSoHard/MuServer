#include <functional>
#include <errno.h>

#include "../include/TcpConnection.h"
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
      highWaterMark_(64 * 1024 * 1024), // 高水位为64MB
      totalPendingBytes_(0),
      highWaterMarkTriggered_(false)
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

void TcpConnection::send(Buffer *buffer)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(buffer->peek(), buffer->readableBytes());
      buffer->retrieveAll();
    }
    else
    {
      std::string copy(buffer->peek(), buffer->readableBytes());
      buffer->retrieveAll();
      // 拷贝
      loop_->runInLoop([this, copy]()
                       { sendInLoop(copy.data(), copy.size()); });
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
    // 本端写关闭 即发送fin，进入半关闭状态
    socket_->shutdownWrite();
  }
}

void TcpConnection::sendFile(const StaticFile& file)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendFileInLoop(file);//FIXME:copy?
    }
    else
    {
      loop_->runInLoop([this, file]()
                       { sendFileInLoop(file); });
    }
  }
}

void TcpConnection::sendFileInLoop(const StaticFile& file)
{
  assert(loop_->isInLoopThread());

  std::unique_ptr<TcpConnection::Context> filectx(new TcpConnection::FileContext(file));
  contextQueue_.emplace_back(std::move(filectx));
  totalPendingBytes_ += file.getFileSize();
  doWriting();
}

void TcpConnection::send(const std::string &buf)
{
  if (state_ == kConnected)
  { 
    // send(const std::string&)
    if (loop_->isInLoopThread())
    {
      sendInLoop(buf.c_str(), buf.size());
    }
    else
    {
      loop_->runInLoop([this, buf]()
                       { sendInLoop(buf.c_str(), buf.size()); });
    }
  }
}
// 需要协调应用和内核的数据发送速度
void TcpConnection::sendInLoop(const void *data, size_t len)
{
  if (state_ == kDisconnected)
  {
    LOG_WARN << "disconnected, give up writing";
    return;
  }
  std::unique_ptr<TcpConnection::Context> datactx(new DataContext(data, len));
  contextQueue_.emplace_back(std::move(datactx));
  totalPendingBytes_ += len;
  doWriting();
}

void TcpConnection::doWriting()
{
  assert(loop_->isInLoopThread());

  while (!contextQueue_.empty())
  {
    auto &ctx = contextQueue_.front();
    size_t n = ctx->writeToSocket(channel_->fd());
    if (n >= 0)
    {
      totalPendingBytes_ -= n;
      if (ctx->isComplete())
      {
        contextQueue_.pop_front();
        if (contextQueue_.empty())
        {
          loop_->runInLoop(std::bind(&TcpConnection::writeCompleteCallback_, this));
          if (channel_->isWriting())
            channel_->disableWriting();
        }
      }
    }
    else
    {
      if (errno != EWOULDBLOCK && errno != EAGAIN)
      {
        contextQueue_.pop_front();
        LOG_ERROR << "TcpConnection::sendInLoop write error";
        handleClose();
        return;
      }
      else // errno == EWOULDBLOCK
      {
        if (ctx->remaining() > 0)
        {

          if (totalPendingBytes_ > highWaterMark_ && !highWaterMarkTriggered_ && highWaterCallback_)
          {
            highWaterMarkTriggered_ = true;
            loop_->runInLoop(std::bind(highWaterCallback_, shared_from_this(), totalPendingBytes_));
          }

          if (!channel_->isWriting())
            channel_->enableWriting();
        }
        break;
      }
    }
  }
}

void TcpConnection::handleWrite()
{
  // 用户是否注册可写事件
  if (channel_->isWriting() && !contextQueue_.empty())
  {
    doWriting();
  }
}
