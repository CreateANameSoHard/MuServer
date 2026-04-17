#pragma once

#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <stdexcept>

#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/exception.h>

class MySQLConnectionPool {
public:
    // 单例接口
    static MySQLConnectionPool& getInstance();

    // 初始化连接池，必须在首次 getConnection() 前调用
    void init(const std::string& url,
              const std::string& user,
              const std::string& password,
              const std::string& database,
              size_t poolSize = 10);

    // 获取一个连接，返回带自动归还功能的 shared_ptr
    // 如果池空，会等待直到有连接可用或超时（默认 5 秒）
    std::shared_ptr<sql::Connection> getConnection(
        std::chrono::milliseconds timeout = std::chrono::seconds(5));

    // 获取当前可用连接数
    size_t idleCount() const;

    // 获取总连接数（池容量）
    size_t totalCount() const;

    // 关闭所有连接（通常在程序退出时自动调用）
    void closeAll();

private:
    MySQLConnectionPool() = default;
    ~MySQLConnectionPool();

    // 创建一条新连接
    sql::Connection* createNewConnection();

    // 归还连接（由 shared_ptr 删除器调用）
    void returnConnection(sql::Connection* conn);

private:
    std::string url_;
    std::string user_;
    std::string password_;
    std::string database_;
    size_t poolSize_ = 0;

    sql::mysql::MySQL_Driver* driver_ = nullptr;

    std::queue<sql::Connection*> idleConnections_;  // 空闲连接
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    bool closed_ = false;          // 连接池已关闭标志
    size_t activeCount_ = 0;       // 当前已借出的连接数（可选，用于统计）
};