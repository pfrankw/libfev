#include <sys/time.h>

#include "fev/time.h"


uint64_t fev_clock()
{
    struct timeval tv;

    if (gettimeofday(&tv, 0) != 0)
        return 0;

    return (tv.tv_sec*1000) + (tv.tv_usec/1000);

}

void fev_sleep_usec(uint64_t usec)
{

#if defined(_WIN32)
    Sleep( ( usec + 999 ) / 1000 );
#else
    struct timeval tv;
    tv.tv_sec  = usec / 1000000;
    tv.tv_usec = usec % 1000000;
    select( 0, 0, 0, 0, &tv );
#endif

}
