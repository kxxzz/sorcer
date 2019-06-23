#include "sorcer_a.h"
#include "sorcer_utils.h"
#include <time.h>




char* SORCER_nowStr(char* timeBuf)
{
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    timeBuf[strftime(timeBuf, SORCER_TimeStrBuf_MAX, "%H:%M:%S", lt)] = '\0';
    return timeBuf;
}

























































































