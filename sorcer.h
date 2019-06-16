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


typedef struct SORCER_Node { u32 id; } SORCER_Node;

SORCER_Node SORCER_tokNew(SORCER_Context* ctx);
SORCER_Node SORCER_seqGet(SORCER_Context* ctx, u32 len, const SORCER_Node* elms);
SORCER_Node SORCER_funGet(SORCER_Context* ctx, u32 numIns, const SORCER_Node* ins, u32 numOuts, const SORCER_Node* outs);


u32 SORCER_dsSize(SORCER_Context* ctx);
const SORCER_Node* SORCER_dsBase(SORCER_Context* ctx);


SORCER_dsPush(SORCER_Context* ctx, SORCER_Node a);
SORCER_dsPop(SORCER_Context* ctx);
SORCER_dsReduce(SORCER_Context* ctx);







































































