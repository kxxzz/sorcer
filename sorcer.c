#include "sorcer_a.h"




typedef struct SORCER_BlockInfo
{
    int x;
} SORCER_BlockInfo;




typedef vec_t(SORCER_Cell) SORCER_CellVec;
typedef vec_t(SORCER_StepInfo) SORCER_StepInfoVec;
typedef vec_t(SORCER_BlockInfo) SORCER_BlockInfoVec;



typedef struct SORCER_Context
{
    SORCER_CellVec dataStack[1];
    SORCER_StepInfoVec stepInfoTable[1];
    SORCER_BlockInfoVec blockInfoTable[1];

    SORCER_CellVec inBuf[1];
} SORCER_Context;




SORCER_Context* SORCER_ctxNew(void)
{
    SORCER_Context* ctx = zalloc(sizeof(SORCER_Context));
    return ctx;
}


void SORCER_ctxFree(SORCER_Context* ctx)
{
    vec_free(ctx->inBuf);

    vec_free(ctx->blockInfoTable);
    vec_free(ctx->stepInfoTable);
    vec_free(ctx->dataStack);
    free(ctx);
}



u32 SORCER_dsSize(SORCER_Context* ctx)
{
    return ctx->dataStack->length;
}

const SORCER_Cell* SORCER_dsBase(SORCER_Context* ctx)
{
    return ctx->dataStack->data;
}




void SORCER_dsPush(SORCER_Context* ctx, SORCER_Cell a)
{
    vec_push(ctx->dataStack, a);
}

void SORCER_dsPop(SORCER_Context* ctx, u32 n, SORCER_Cell* out)
{
    SORCER_CellVec* ds = ctx->dataStack;
    assert(ds->length >= n);
    memcpy(out, ds->data + ds->length - n, sizeof(SORCER_Cell)*n);
    vec_resize(ds, ds->length - n);
}




SORCER_Step SORCER_stepFromInfo(SORCER_Context* ctx, const SORCER_StepInfo* info)
{
    SORCER_StepInfoVec* st = ctx->stepInfoTable;
    vec_push(st, *info);
    SORCER_Step a = { st->length - 1 };
    return a;
}

void SORCER_step(SORCER_Context* ctx, SORCER_Step step)
{
    SORCER_StepInfoVec* st = ctx->stepInfoTable;
    const SORCER_StepInfo* info = st->data + step.id;
    SORCER_CellVec* inBuf = ctx->inBuf;
    SORCER_CellVec* ds = ctx->dataStack;

    vec_resize(inBuf, info->numIns);
    vec_resize(ds, ds->length + info->numOuts - info->numIns);

    SORCER_dsPop(ctx, info->numIns, ctx->inBuf->data);
    info->func(inBuf->data, ds->data + ds->length - info->numOuts);
}





SORCER_Block SORCER_blockNew(SORCER_Context* ctx)
{
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_Block a = { bt->length - 1 };
    return a;
}






void SORCER_blockAddPushCell(SORCER_Context* ctx, SORCER_Block blk, SORCER_Cell a)
{

}
void SORCER_blockAddPushVar(SORCER_Context* ctx, SORCER_Block blk, SORCER_Var var)
{

}
void SORCER_blockAddStep(SORCER_Context* ctx, SORCER_Block blk, SORCER_Step step)
{

}
void SORCER_blockAddApply(SORCER_Context* ctx, SORCER_Block blk)
{

}
void SORCER_blockAddCall(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block callee)
{

}





SORCER_Var SORCER_blockAddPopVar(SORCER_Context* ctx, SORCER_Block blk)
{
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* info = bt->data + blk.id;
    SORCER_Var a = { 0 };
    return a;
}




void SORCER_blockCall(SORCER_Context* ctx, SORCER_Block blk)
{

}











































































