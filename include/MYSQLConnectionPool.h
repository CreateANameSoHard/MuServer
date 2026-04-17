#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>

class MYSQLConnectionPool
{
public:
    //Singleton
    static MYSQLConnectionPool& getInstance()
    {
        static MYSQLConnectionPool pool;
        return pool;
    }

    void init(const std::string url, const std::string user,const std::string passwd ,const std::string database, size_t pool_size = 10);

    std::shared_ptr<sql::Connection> getConnection();
    

private:
    MYSQLConnectionPool() = default;
    ~MYSQLConnectionPool() = default;

    std::queue<sql::Connection*> connection_;
    std::mutex mutex_;
    std::condition_variable cond_;
    sql::mysql::MySQL_Driver* driver_;
};