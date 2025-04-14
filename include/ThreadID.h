#pragma once

#include <unistd.h>
#include <sys/syscall.h>

/**
 *
 */
namespace ThreadID
{
    // 保存内核的tid,
    extern thread_local int Tid = 0;

    void cacheTid()
    {
        if (Tid == 0)
        {
            Tid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }

    inline int tid()
    {
        if (__builtin_expect(Tid == 0, 0))
        {
            cacheTid();
        }
        return Tid;
    }
}