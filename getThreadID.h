#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace getThreadID
{
    //保存内核的tid,避免多次io
    extern __thread int t_cachedTid;

    void cacheTid(); 

    inline int tid() {
        if (__builtin_expect(t_cachedTid == 0, 0)) {
            cacheTid();
        }
        return t_cachedTid;
    }
}