#pragma once



#include "sorcer.h"

#include <assert.h>
#include <stdio.h>

#include <vec.h>
#include <upool.h>




#ifdef ARYLEN
# undef ARYLEN
#endif
#define ARYLEN(a) (sizeof(a) / sizeof((a)[0]))




#ifdef max
# undef max
#endif
#ifdef min
# undef min
#endif
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))




#define zalloc(sz) calloc(1, sz)





static char* stzncpy(char* dst, char const* src, u32 len)
{
    assert(len > 0);
#ifdef _WIN32
    char* p = _memccpy(dst, src, 0, len - 1);
#else
    char* p = memccpy(dst, src, 0, len - 1);
#endif
    if (p) --p;
    else
    {
        p = dst + len - 1;
        *p = 0;
    }
    return p;
}




static u32 findInArray32(const u32* a, u32 n, u32 x)
{
    for (u32 i = 0; i < n; ++i)
    {
        if (a[i] == x)
        {
            return i;
        }
    }
    return -1;
}


















































































