#pragma once


#include <txn.h>


typedef struct SORCER_Context SORCER_Context;

SORCER_Context* SORCER_ctxNew(TXN_Space* space);
void SORCER_ctxFree(SORCER_Context* ctx);


typedef struct SORCER_Cell
{
    union
    {
        u64 val;
        void* ptr;
    } as;
} SORCER_Cell;


u32 SORCER_dsSize(SORCER_Context* ctx);
const SORCER_Cell* SORCER_dsBase(SORCER_Context* ctx);


void SORCER_dsPush(SORCER_Context* ctx, SORCER_Cell a);
SORCER_Cell SORCER_dsPop(SORCER_Context* ctx);
void SORCER_dsReduce(SORCER_Context* ctx);







































































