#pragma once



#include "sorcer.h"

#include <assert.h>

#include <vec.h>



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












typedef enum SORCER_Step
{
    SORCER_Step_Invalid = -1,

    SORCER_Step_Not,

    SORCER_NumSteps
} SORCER_Step;




typedef void(*SORCER_StepFunc)(const SORCER_Cell* ins, SORCER_Cell* outs);

typedef struct SORCER_StepInfo
{
    const char* name;
    u32 numIns;
    u32 numOuts;
    SORCER_StepFunc func;
} SORCER_StepInfo;

const SORCER_StepInfo SORCER_StepInfoTable[SORCER_NumSteps];






































































