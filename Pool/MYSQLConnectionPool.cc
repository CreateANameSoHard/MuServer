#include "../include/MYSQLConnectionPool.h"

void MYSQLConnectionPool::init(const std::string url, const std::string user, const std::string passwd, const std::string database, size_t pool_size)
{
    driver_ = sql::mysql::get_driver_instance();
    for (size_t i = 0; i < pool_size; i++)
    {
        sql::Connection *conn = driver_->connect(url, user, passwd);
        conn->setSchema(database);
        connection_.emplace(conn);
    }
}

std::shared_ptr<sql::Connection> MYSQLConnectionPool::getConnection()
{
    std::unique_lock<std::mutex> lock(mutex_);
    //FIXME:非法访问
    cond_.wait(lock, [this]() -> bool
               { !connection_.empty(); });
    sql::Connection *conn = connection_.front();
    connection_.pop();
    // resume through deleter
    return std::shared_ptr<sql::Connection>(conn, [this](sql::Connection *connection)
                                            {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_.emplace(connection); });
}