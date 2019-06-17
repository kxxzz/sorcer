#pragma once



#include <stdbool.h>


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



typedef struct SORCER_Step { u32 id; } SORCER_Step;
typedef struct SORCER_Block { u32 id; } SORCER_Block;
typedef struct SORCER_Var { u32 id; } SORCER_Var;




typedef struct SORCER_Cell
{
    union
    {
        u64 val;
        void* ptr;
        SORCER_Block blk;
    } as;
} SORCER_Cell;

u32 SORCER_dsSize(SORCER_Context* ctx);
const SORCER_Cell* SORCER_dsBase(SORCER_Context* ctx);

void SORCER_dsPush(SORCER_Context* ctx, SORCER_Cell a);
void SORCER_dsPop(SORCER_Context* ctx, u32 n, SORCER_Cell* out);




typedef void(*SORCER_StepFunc)(const SORCER_Cell* ins, SORCER_Cell* outs);

typedef struct SORCER_StepInfo
{
    u32 numIns;
    u32 numOuts;
    SORCER_StepFunc func;
} SORCER_StepInfo;

SORCER_Step SORCER_stepFromInfo(SORCER_Context* ctx, const SORCER_StepInfo* info);
void SORCER_step(SORCER_Context* ctx, SORCER_Step step);



SORCER_Block SORCER_blockNew(SORCER_Context* ctx);

SORCER_Var SORCER_blockAddInstPopVar(SORCER_Context* ctx, SORCER_Block blk);

void SORCER_blockAddInstPushCell(SORCER_Context* ctx, SORCER_Block blk, SORCER_Cell a);
void SORCER_blockAddInstPushVar(SORCER_Context* ctx, SORCER_Block blk, SORCER_Var var);
void SORCER_blockAddInstStep(SORCER_Context* ctx, SORCER_Block blk, SORCER_Step step);
void SORCER_blockAddInstApply(SORCER_Context* ctx, SORCER_Block blk);
void SORCER_blockAddInstCall(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block callee);




void SORCER_blockCall(SORCER_Context* ctx, SORCER_Block blk);
















































