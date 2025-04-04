#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include "rocket/common/util.h"


namespace rocket
{
    static int g_pid = 0;
    static thread_local int g_thread_id = 0;

pid_t getPid()
{
    if(g_pid==0)
    {
        g_pid = getpid();
    }
    return g_pid;
}
pid_t getThreadId()
{
    if(g_thread_id == 0)
    {
        g_thread_id = syscall(SYS_gettid);
    }
    return g_thread_id;
}

int64_t getNowMs()
{
    timeval val;
    gettimeofday(&val,NULL);

    return val.tv_sec * 1000 + val.tv_usec / 1000;
}


}