#include <sys/time.h>

int _gettimeofday(struct timeval *tv, void *tz)
{
    (void)tz;
    if(tv) {
        tv->tv_sec = 0;
        tv->tv_usec = 0;
    }
    return 0;
}
