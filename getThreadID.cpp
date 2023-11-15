#include "getThreadID.h"

__thread int getThreadID::t_cachedTid = 0;

void getThreadID::cacheTid(){
        if (t_cachedTid == 0){
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }