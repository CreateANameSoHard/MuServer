#include "../include/MYSQLConnectionPool.h"
#include "cppconn/statement.h"

MySQLConnectionPool& MySQLConnectionPool::getInstance() {
    static MySQLConnectionPool instance;
    return instance;
}

void MySQLConnectionPool::init(const std::string& url,
                               const std::string& user,
                               const std::string& password,
                               const std::string& database,
                               size_t poolSize) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (driver_ != nullptr) {
        // 已经初始化过
        return;
    }
    url_ = url;
    user_ = user;
    password_ = password;
    database_ = database;
    poolSize_ = poolSize;

    try {
        driver_ = sql::mysql::get_driver_instance();
        for (size_t i = 0; i < poolSize_; ++i) {
            sql::Connection* conn = createNewConnection();
            idleConnections_.push(conn);
        }
    } catch (sql::SQLException& e) {
        throw std::runtime_error("MySQLConnectionPool init failed: " +
                                 std::string(e.what()));
    }
}

sql::Connection* MySQLConnectionPool::createNewConnection() {
    sql::Connection* conn = driver_->connect(url_, user_, password_);
    conn->setSchema(database_);
    return conn;
}

std::shared_ptr<sql::Connection> MySQLConnectionPool::getConnection(
    std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (closed_) {
        throw std::runtime_error("Connection pool is already closed");
    }

    // 等待直到有空闲连接或超时
    bool hasIdle = cond_.wait_for(lock, timeout, [this] {
        return closed_ || !idleConnections_.empty();
    });

    if (closed_) {
        throw std::runtime_error("Connection pool is closed during waiting");
    }

    if (!hasIdle) {
        throw std::runtime_error("Get connection timeout");
    }

    sql::Connection* conn = idleConnections_.front();
    idleConnections_.pop();
    ++activeCount_;

    // 返回带有自定义删除器的 shared_ptr
    return std::shared_ptr<sql::Connection>(conn,
        [this](sql::Connection* connToReturn) {
            this->returnConnection(connToReturn);
        });
}

void MySQLConnectionPool::returnConnection(sql::Connection* conn) {
    if (conn == nullptr) return;

    std::lock_guard<std::mutex> lock(mutex_);

    if (closed_) {
        // 连接池已关闭，直接销毁连接
        delete conn;
        return;
    }

    // 可选：检查连接是否有效，无效则丢弃并新建一个补充
    bool isValid = true;
    try {
        // 简单心跳：执行一个无害查询
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        stmt->executeQuery("SELECT 1");
    } catch (sql::SQLException&) {
        isValid = false;
    }

    if (!isValid) {
        // 丢弃无效连接，新建一条放回池中
        delete conn;
        try {
            conn = createNewConnection();
        } catch (sql::SQLException& e) {
            // 创建失败，减少一个池容量（连接数不足）
            std::cerr << "Failed to replenish invalid connection: " << e.what() << std::endl;
            --poolSize_;  // 实际可用连接数减一
            --activeCount_;
            return;
        }
    }

    idleConnections_.push(conn);
    --activeCount_;
    cond_.notify_one();  // 通知等待的线程
}

size_t MySQLConnectionPool::idleCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return idleConnections_.size();
}

size_t MySQLConnectionPool::totalCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return poolSize_;
}

void MySQLConnectionPool::closeAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;

    // 销毁所有空闲连接
    while (!idleConnections_.empty()) {
        delete idleConnections_.front();
        idleConnections_.pop();
    }

    // 注意：已经借出的连接在归还时会因为 closed_=true 而被直接 delete
    // 因此这里不需要等待 activeCount_ 变为 0
    cond_.notify_all();  // 唤醒所有等待 getConnection 的线程，它们会抛出异常
}

MySQLConnectionPool::~MySQLConnectionPool() {
    closeAll();
}