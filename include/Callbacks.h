#pragma once
//工具。定义了各种回调操作
#include <functional>
#include <memory>
#include <string>


class Buffer;
class TcpConnection;
class TimeStamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
//Timer
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, TimeStamp)>; //message的回调特殊一点。其他都是传TcpConn的指针就行
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using HighWaterCallback = std::function<void(const TcpConnectionPtr&, size_t)>; // 高水位回调。当tcp缓冲区内的数据到达某个临界值时，执行的回调
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;