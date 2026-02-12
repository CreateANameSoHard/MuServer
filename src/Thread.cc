#include <assert.h>
#include <string>
#include <semaphore.h>

#include "../include/Thread.h"
#include "../include/CurrentThread.h"



std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false),
      joined_(false),
      func_(std::move(func)),
      tid_(0),
      name_(name)
{
    setDefaultName();
}
// 析构的话（没有用join），就直接分离线程
Thread::~Thread()
{
    if(started_ && !joined_)
    {
        thread_->detach();
    }
}
// 这里如果不加同步关系的话，可能start已经返回了，甚至主线程已经访问thread对象的tid了，子线程里还没有赋值，就会出错
void Thread::start()
{
    assert(!started_);
    started_ = true;

    sem_t sem;
    sem_init(&sem, 0, 0);

    thread_ = std::make_shared<std::thread>([&](){
        tid_ = CurrentThread::tid();
        sem_post(&sem);
        func_();
    });
    sem_wait(&sem);
    // sem_destroy(&sem);
}

void Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    thread_->join();
}


void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if(name_.empty())
    {
        name_ = "Thread" + std::to_string(num);
    }
}