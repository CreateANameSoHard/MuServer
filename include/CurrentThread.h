#pragma once
#include<unistd.h>
#include<sys/syscall.h>

namespace CurrentThread 
{
    // 跨文件的 TLS 变量共享
    // 线程局部存储变量，每个线程都有自己的一份拷贝，互不干扰
    extern thread_local int t_cachedTid;
    void cacheTid();

    // 获取线程 ID 调这个函数
    inline int tid()
    {
        // __builtin_expect:第二个参数如果为0，表示第一个参数的产生的可能性较小，否则较大。这个函数用于优化分支预测指令
        if(__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}