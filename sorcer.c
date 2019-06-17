#include "sorcer_a.h"





typedef enum SORCER_OP
{
    SORCER_OP_PopVar = 0,
    SORCER_OP_PushCell,
    SORCER_OP_PushVar,
    SORCER_OP_PushBlock,
    SORCER_OP_Step,
    SORCER_OP_Apply,
    SORCER_OP_Call,

    SORCER_OP_Ret,
    SORCER_OP_Jz,
    SORCER_OP_Jmp,

    SORCER_NumOPs
} SORCER_OP;



typedef struct SORCER_Inst
{
    SORCER_OP op;
    union
    {
        SORCER_Cell cell;
        SORCER_Var var;
        SORCER_Step step;
        SORCER_Block block;
        u32 address;
    } arg;
} SORCER_Inst;

typedef vec_t(SORCER_Inst) SORCER_InstVec;




typedef struct SORCER_BlockInfo
{
    u32 varCount;
    SORCER_InstVec code[1];
    u32 baseAddress;
} SORCER_BlockInfo;

static void SORCER_blockInfoFree(SORCER_BlockInfo* a)
{
    vec_free(a->code);
}




typedef vec_t(SORCER_Cell) SORCER_CellVec;
typedef vec_t(SORCER_StepInfo) SORCER_StepInfoVec;
typedef vec_t(SORCER_BlockInfo) SORCER_BlockInfoVec;


typedef struct SORCER_Context
{
    SORCER_CellVec dataStack[1];
    SORCER_StepInfoVec stepInfoTable[1];
    SORCER_BlockInfoVec blockInfoTable[1];

    bool codeUpdated;
    SORCER_InstVec code[1];
    vec_u32 retStack[1];

    SORCER_CellVec inBuf[1];
    SORCER_CellVec varTable[1];
} SORCER_Context;



SORCER_Context* SORCER_ctxNew(void)
{
    SORCER_Context* ctx = zalloc(sizeof(SORCER_Context));
    return ctx;
}

void SORCER_ctxFree(SORCER_Context* ctx)
{
    vec_free(ctx->varTable);
    vec_free(ctx->inBuf);

    vec_free(ctx->retStack);
    vec_free(ctx->code);

    for (u32 i = 0; i < ctx->blockInfoTable->length; ++i)
    {
        SORCER_blockInfoFree(ctx->blockInfoTable->data + i);
    }
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








static void SORCER_codeOutdate(SORCER_Context* ctx)
{
    if (ctx->codeUpdated)
    {
        ctx->codeUpdated = false;
    }
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
    SORCER_codeOutdate(ctx);
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
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo info = { 0 };
    vec_push(bt, info);
    SORCER_Block a = { bt->length - 1 };
    return a;
}





void SORCER_blockAddInstPushCell(SORCER_Context* ctx, SORCER_Block blk, SORCER_Cell x)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* info = bt->data + blk.id;
    SORCER_Inst inst = { SORCER_OP_PushCell, .arg.cell = x };
    vec_push(info->code, inst);
}


void SORCER_blockAddInstPushVar(SORCER_Context* ctx, SORCER_Block blk, SORCER_Var v)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* info = bt->data + blk.id;
    SORCER_Inst inst = { SORCER_OP_PushVar, .arg.var = v };
    vec_push(info->code, inst);
}


void SORCER_blockAddInstPushBlock(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block b)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* info = bt->data + blk.id;
    SORCER_Inst inst = { SORCER_OP_PushBlock, .arg.block = b };
    vec_push(info->code, inst);
}


void SORCER_blockAddInstStep(SORCER_Context* ctx, SORCER_Block blk, SORCER_Step step)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* info = bt->data + blk.id;
    SORCER_Inst inst = { SORCER_OP_Step, .arg.step = step };
    vec_push(info->code, inst);
}


void SORCER_blockAddInstApply(SORCER_Context* ctx, SORCER_Block blk)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* info = bt->data + blk.id;
    SORCER_Inst inst = { SORCER_OP_Apply };
    vec_push(info->code, inst);
}


void SORCER_blockAddInstCall(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block callee)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* info = bt->data + blk.id;
    SORCER_Inst inst = { SORCER_OP_Call, .arg.block = callee };
    vec_push(info->code, inst);
}





SORCER_Var SORCER_blockAddInstPopVar(SORCER_Context* ctx, SORCER_Block blk)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* info = bt->data + blk.id;
    SORCER_Var var = { info->varCount++ };
    SORCER_Inst inst = { SORCER_OP_PopVar, .arg.var = var };
    vec_push(info->code, inst);
    return var;
}












static void SORCER_codeUpdate(SORCER_Context* ctx)
{
    if (ctx->codeUpdated)
    {
        return;
    }
    ctx->codeUpdated = true;

    SORCER_InstVec* code = ctx->code;
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;

    for (u32 bi = 0; bi < bt->length; ++bi)
    {
        SORCER_BlockInfo* blkInfo = bt->data + bi;
        blkInfo->baseAddress = code->length;
        for (u32 i = 0; i < blkInfo->code->length; ++i)
        {
            SORCER_Inst* inst = blkInfo->code->data + i;
            vec_push(code, *inst);
        }
        SORCER_Inst ret = { SORCER_OP_Ret };
        vec_push(code, ret);
    }

    for (u32 i = 0; i < code->length; ++i)
    {
        SORCER_Inst* inst = code->data + i;
        switch (inst->op)
        {
        case SORCER_OP_PushBlock:
        case SORCER_OP_Call:
        {
            SORCER_BlockInfo* blkInfo = bt->data + inst->arg.block.id;
            inst->arg.address = blkInfo->baseAddress;
            break;
        }
        default:
            break;
        }
    }
}










void SORCER_blockCall(SORCER_Context* ctx, SORCER_Block blk)
{
    SORCER_codeUpdate(ctx);

    SORCER_InstVec* code = ctx->code;
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_CellVec* ds = ctx->dataStack;
    vec_u32* rs = ctx->retStack;

    u32 p = bt->data[blk.id].baseAddress;
    SORCER_Inst* inst = NULL;
next:
    inst = code->data + p++;
    switch (inst->op)
    {
    case SORCER_OP_PopVar:
    {
        goto next;
    }
    case SORCER_OP_PushCell:
    {
        goto next;
    }
    case SORCER_OP_PushVar:
    {
        goto next;
    }
    case SORCER_OP_PushBlock:
    {
        goto next;
    }
    case SORCER_OP_Step:
    {
        goto next;
    }
    case SORCER_OP_Apply:
    {
        goto next;
    }
    case SORCER_OP_Call:
    {
        vec_push(rs, p);
        p = inst->arg.address;
        goto next;
    }
    case SORCER_OP_Ret:
    {
        if (!rs->length)
        {
            return;
        }
        u32 addr = vec_last(rs);
        vec_pop(rs);
        p = addr;
        goto next;
    }
    case SORCER_OP_Jz:
    {
        SORCER_Cell* top = &vec_last(ds);
        if (!top->as.val)
        {
            p = inst->arg.address;
        }
        goto next;
    }
    case SORCER_OP_Jmp:
    {
        p = inst->arg.address;
        goto next;
    }
    default:
        assert(false);
        break;
    }
}











































































