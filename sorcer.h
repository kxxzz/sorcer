#pragma once


#include <stdbool.h>
#include <stdint.h>



typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef float f32;
typedef double f64;



typedef struct SORCER_Context SORCER_Context;

SORCER_Context* SORCER_ctxNew(void);
void SORCER_ctxFree(SORCER_Context* ctx);


typedef struct SORCER_Cell
{
    union
    {
        uintptr_t val;
        void* ptr;
    } as;
} SORCER_Cell;


u32 SORCER_dsSize(SORCER_Context* ctx);
const SORCER_Cell* SORCER_dsBase(SORCER_Context* ctx);


void SORCER_dsPush(SORCER_Context* ctx, SORCER_Cell a);
SORCER_Cell SORCER_dsPop(SORCER_Context* ctx);
void SORCER_dsReduce(SORCER_Context* ctx);







































































