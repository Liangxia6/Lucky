// #include "ThreadID.h"

// thread_local int ThreadID::Tid = 0;

// void ThreadID::cacheTid()
// {
//     if (Tid == 0)
//     {
//         Tid = static_cast<pid_t>(::syscall(SYS_gettid));
//     }
// }