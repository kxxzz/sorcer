#pragma once


#include <vec.h>
#include <apnum.h>



typedef enum SORCER_ValueType
{
    SORCER_ValueType_UintC = 0,
    SORCER_ValueType_IntC,
    SORCER_ValueType_IntAP,
    SORCER_ValueType_RatAP,
    SORCER_ValueType_Src,
    SORCER_ValueType_Block,
    SORCER_ValueType_Array,

    SORCER_NumValueTypes
} SORCER_ValueType;

typedef struct SORCER_Value
{
    SORCER_ValueType type;
    union
    {
        uintptr_t ui;
        intptr_t i;
        APNUM_int* intAP;
        APNUM_rat* ratAP;
        struct SORCER_Block* blk;
        struct SORCER_Array* ary;
    } as;
} SORCER_Value;

typedef vec_t(struct SORCER_Value) SORCER_ValueVec;



typedef struct SORCER_Context SORCER_Context;

SORCER_Context* SORCER_ctxNew(void);
void SORCER_ctxFree(SORCER_Context* ctx);






















































































