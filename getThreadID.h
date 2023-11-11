#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace getThreadID
{
    __thread int t_cachedTid = 0;

    void cacheTid()
    {
        if (t_cachedTid == 0)
        {
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }

    inline int tid() // 内联函数只在当前文件中起作用
    {
        if (__builtin_expect(t_cachedTid == 0, 0)) // __builtin_expect 是一种底层优化 此语句意思是如果还未获取tid 进入if 通过cacheTid()系统调用获取tid
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}