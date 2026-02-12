#pragma once
#include <functional>
#include <memory>
#include <thread>
#include <unistd.h>
#include <string>
#include <atomic>

#include "noncopyable.h"

class Thread : noncopyable
{
    using ThreadFunc = std::function<void()>;

public:
    explicit Thread(ThreadFunc func, const std::string &name = std::string());
    ~Thread();
    void start();
    void join();
    bool started() const { return started_; }
    bool joined() const { return joined_; }
    pid_t tid() const { return tid_; }
    std::string &name() { return name_; }
    static int numCreated() { return numCreated_; }

private:
    void setDefaultName();

    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    ThreadFunc func_;
    pid_t tid_;
    std::string name_;
    static std::atomic_int numCreated_;
};