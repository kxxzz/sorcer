#include "sorcer_a.h"





typedef enum SORCER_OP
{
    SORCER_OP_PopVar = 0,
    SORCER_OP_PushCell,
    SORCER_OP_PushVar,
    SORCER_OP_PushBlock,
    SORCER_OP_Call,
    SORCER_OP_Apply,
    SORCER_OP_Ifte,
    SORCER_OP_Opr,

    SORCER_OP_Ret,
    SORCER_OP_Jmp,
    SORCER_OP_Jnz,
    SORCER_OP_ApplyJmp,

    SORCER_NumOPs
} SORCER_OP;



typedef struct SORCER_Inst
{
    SORCER_OP op;
    union
    {
        SORCER_Cell cell;
        SORCER_Var var;
        SORCER_Block block;
        SORCER_Opr opr;
        u32 address;
    } arg;
} SORCER_Inst;

typedef vec_t(SORCER_Inst) SORCER_InstVec;




typedef struct SORCER_BlockInfo
{
    u32 varCount;
    SORCER_InstVec code[1];
    u32 calleeCount;
    u32 baseAddress;
} SORCER_BlockInfo;

static void SORCER_blockInfoFree(SORCER_BlockInfo* a)
{
    vec_free(a->code);
}




typedef struct SORCER_Ret
{
    u32 address;
    u32 varBase;
} SORCER_Ret;



typedef vec_t(SORCER_CellTypeInfo) SORCER_CellTypeInfoVec;
typedef vec_t(SORCER_Cell) SORCER_CellVec;
typedef vec_t(SORCER_OprInfo) SORCER_OprInfoVec;
typedef vec_t(SORCER_BlockInfo) SORCER_BlockInfoVec;
typedef vec_t(SORCER_Ret) SORCER_RetVec;


typedef struct SORCER_Context
{
    SORCER_CellTypeInfoVec cellTypeTable[1];
    SORCER_OprInfoVec oprInfoTable[1];
    SORCER_BlockInfoVec blockInfoTable[1];

    SORCER_CellVec dataStack[1];

    bool codeUpdated;
    SORCER_InstVec code[1];
    SORCER_RetVec retStack[1];

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

    vec_free(ctx->dataStack);

    for (u32 i = 0; i < ctx->blockInfoTable->length; ++i)
    {
        SORCER_blockInfoFree(ctx->blockInfoTable->data + i);
    }
    vec_free(ctx->blockInfoTable);
    vec_free(ctx->oprInfoTable);
    vec_free(ctx->cellTypeTable);
    free(ctx);
}









SORCER_CellType SORCER_cellTypeNew(SORCER_Context* ctx, const SORCER_CellTypeInfo* info)
{
    SORCER_CellTypeInfoVec* ct = ctx->oprInfoTable;
    vec_push(ct, *info);
    SORCER_CellType a = { ct->length - 1 };
    return a;
}




bool SORCER_cellNew(SORCER_Context* ctx, SORCER_CellType type, const char* str, SORCER_Cell* out)
{
    SORCER_CellTypeInfoVec* ct = ctx->cellTypeTable;
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    assert(type.id < ct->length);
    return ct->data[type.id].ctor(str, out);
}









SORCER_Opr SORCER_oprNew(SORCER_Context* ctx, const SORCER_OprInfo* info)
{
    SORCER_OprInfoVec* st = ctx->oprInfoTable;
    vec_push(st, *info);
    SORCER_Opr a = { st->length - 1 };
    return a;
}

void SORCER_opr(SORCER_Context* ctx, SORCER_Opr opr)
{
    const SORCER_OprInfo* info = ctx->oprInfoTable->data + opr.id;
    SORCER_CellVec* inBuf = ctx->inBuf;
    SORCER_CellVec* ds = ctx->dataStack;

    vec_resize(inBuf, info->numIns);
    vec_resize(ds, ds->length + info->numOuts - info->numIns);

    SORCER_dsPop(ctx, info->numIns, ctx->inBuf->data);
    info->func(inBuf->data, ds->data + ds->length - info->numOuts);
}



















u32 SORCER_ctxBlocksTotal(SORCER_Context* ctx)
{
    return ctx->blockInfoTable->length;
}

void SORCER_ctxBlocksRollback(SORCER_Context* ctx, u32 n)
{
    for (u32 i = n; i < ctx->blockInfoTable->length; ++i)
    {
        SORCER_blockInfoFree(ctx->blockInfoTable->data + i);
    }
    vec_resize(ctx->blockInfoTable, n);
}












static void SORCER_codeOutdate(SORCER_Context* ctx)
{
    if (ctx->codeUpdated)
    {
        ctx->codeUpdated = false;
    }
}




SORCER_Block SORCER_blockNew(SORCER_Context* ctx)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo binfo = { 0 };
    vec_push(bt, binfo);
    SORCER_Block a = { bt->length - 1 };
    return a;
}










SORCER_Var SORCER_blockAddInstPopVar(SORCER_Context* ctx, SORCER_Block blk)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* binfo = bt->data + blk.id;
    SORCER_Var var = { binfo->varCount++ };
    SORCER_Inst inst = { SORCER_OP_PopVar, .arg.var = var };
    vec_push(binfo->code, inst);
    return var;
}




void SORCER_blockAddInstPushCell(SORCER_Context* ctx, SORCER_Block blk, const SORCER_Cell* x)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* binfo = bt->data + blk.id;
    SORCER_Inst inst = { SORCER_OP_PushCell, .arg.cell = *x };
    vec_push(binfo->code, inst);
}


void SORCER_blockAddInstPushVar(SORCER_Context* ctx, SORCER_Block blk, SORCER_Var v)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* binfo = bt->data + blk.id;
    SORCER_Inst inst = { SORCER_OP_PushVar, .arg.var = v };
    vec_push(binfo->code, inst);
}


void SORCER_blockAddInstPushBlock(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block b)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* binfo = bt->data + blk.id;
    ++bt->data[b.id].calleeCount;
    SORCER_Inst inst = { SORCER_OP_PushBlock, .arg.block = b };
    vec_push(binfo->code, inst);
}


void SORCER_blockAddInstCall(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block callee)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* binfo = bt->data + blk.id;
    ++bt->data[callee.id].calleeCount;
    SORCER_Inst inst = { SORCER_OP_Call, .arg.block = callee };
    vec_push(binfo->code, inst);
}


void SORCER_blockAddInstApply(SORCER_Context* ctx, SORCER_Block blk)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* binfo = bt->data + blk.id;
    SORCER_Inst inst = { SORCER_OP_Apply };
    vec_push(binfo->code, inst);
}


void SORCER_blockAddInstIfte(SORCER_Context* ctx, SORCER_Block blk)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* binfo = bt->data + blk.id;
    SORCER_Inst inst = { SORCER_OP_Apply };
    vec_push(binfo->code, inst);
}


void SORCER_blockAddInstOpr(SORCER_Context* ctx, SORCER_Block blk, SORCER_Opr opr)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* binfo = bt->data + blk.id;
    SORCER_Inst inst = { SORCER_OP_Opr,.arg.opr = opr };
    vec_push(binfo->code, inst);
}













void SORCER_blockAddInlineBlock(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block ib)
{
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* bInfo = bt->data + blk.id;
    SORCER_BlockInfo* iInfo = bt->data + ib.id;
    for (u32 i = 0; i < iInfo->code->length; ++i)
    {
        SORCER_Inst inst = iInfo->code->data[i];
        switch (inst.op)
        {
        case SORCER_OP_PopVar:
        case SORCER_OP_PushVar:
        {
            inst.arg.var.id += bInfo->varCount;
            break;
        }
        default:
            break;
        }
        vec_push(bInfo->code, inst);
    }
    bInfo->varCount += iInfo->varCount;
}





void SORCER_blockAddPatIfteCT(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block onTrue, SORCER_Block onFalse)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_BlockInfo* bInfo = bt->data + blk.id;
    SORCER_BlockInfo* onTrueInfo = bt->data + onTrue.id;
    SORCER_BlockInfo* onFalseInfo = bt->data + onFalse.id;
    u32 jnzOff = bInfo->code->length;
    {
        SORCER_Inst inst = { SORCER_OP_Jnz };
        vec_push(bInfo->code, inst);
    }
    SORCER_blockAddInlineBlock(ctx, blk, onFalse);
    u32 jmpOff = bInfo->code->length;
    {
        SORCER_Inst inst = { SORCER_OP_Jmp };
        vec_push(bInfo->code, inst);
    }
    bInfo->code->data[jnzOff].arg.address = bInfo->code->length;
    SORCER_blockAddInlineBlock(ctx, blk, onTrue);
    bInfo->code->data[jmpOff].arg.address = bInfo->code->length;
}






















static void SORCER_codeUpdate(SORCER_Context* ctx, SORCER_Block blk)
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
        if (!blkInfo->calleeCount && (bi != blk.id))
        {
            continue;
        }
        blkInfo->baseAddress = code->length;
        for (u32 i = 0; i < blkInfo->code->length; ++i)
        {
            SORCER_Inst inst = blkInfo->code->data[i];
            switch (inst.op)
            {
            case SORCER_OP_Jmp:
            case SORCER_OP_Jnz:
            {
                inst.arg.address += blkInfo->baseAddress;
                break;
            }
            default:
                break;
            }
            // Tail Call Elimination
            if (i + 1 == blkInfo->code->length)
            {
                switch (inst.op)
                {
                case SORCER_OP_Call:
                {
                    inst.op = SORCER_OP_Jmp;
                    break;
                }
                case SORCER_OP_Apply:
                {
                    inst.op = SORCER_OP_ApplyJmp;
                    break;
                }
                default:
                    break;
                }
            }
            vec_push(code, inst);
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










void SORCER_run(SORCER_Context* ctx, SORCER_Block blk)
{
    SORCER_codeUpdate(ctx, blk);

    SORCER_InstVec* code = ctx->code;
    SORCER_BlockInfoVec* bt = ctx->blockInfoTable;
    SORCER_CellVec* ds = ctx->dataStack;
    SORCER_RetVec* rs = ctx->retStack;
    SORCER_CellVec* vt = ctx->varTable;

    assert(blk.id < bt->length);
    u32 p = bt->data[blk.id].baseAddress;
    SORCER_Inst* inst = NULL;
next:
    inst = code->data + p++;
    switch (inst->op)
    {
    case SORCER_OP_PopVar:
    {
        SORCER_Cell top = vec_last(ds);
        vec_pop(ds);
        vec_push(vt, top);
        goto next;
    }
    case SORCER_OP_PushCell:
    {
        vec_push(ds, inst->arg.cell);
        goto next;
    }
    case SORCER_OP_PushVar:
    {
        u32 varBase = 0;
        if (rs->length > 0)
        {
            SORCER_Ret ret = vec_last(rs);
            varBase = ret.varBase;
        }
        SORCER_Cell cell = vt->data[varBase + inst->arg.var.id];
        vec_push(ds, cell);
        goto next;
    }
    case SORCER_OP_PushBlock:
    {
        SORCER_Cell cell = { .as.block = inst->arg.block };
        vec_push(ds, cell);
        goto next;
    }
    case SORCER_OP_Call:
    {
        SORCER_Ret ret = { p, vt->length };
        vec_push(rs, ret);
        p = inst->arg.address;
        goto next;
    }
    case SORCER_OP_Apply:
    {
        SORCER_Cell top = vec_last(ds);
        vec_pop(ds);
        SORCER_Ret ret = { p, vt->length };
        vec_push(rs, ret);
        p = top.as.address;
        goto next;
    }
    case SORCER_OP_Ifte:
    {
        SORCER_Cell onFalse = vec_last(ds);
        vec_pop(ds);
        SORCER_Cell onTure = vec_last(ds);
        vec_pop(ds);
        SORCER_Cell cond = vec_last(ds);
        vec_pop(ds);
        SORCER_Ret ret = { p, vt->length };
        vec_push(rs, ret);
        if (cond.as.val)
        {
            p = onTure.as.address;
        }
        else
        {
            p = onFalse.as.address;
        }
        goto next;
    }
    case SORCER_OP_Opr:
    {
        SORCER_opr(ctx, inst->arg.opr);
        goto next;
    }
    case SORCER_OP_Ret:
    {
        if (!rs->length)
        {
            return;
        }
        SORCER_Ret ret = vec_last(rs);
        vec_pop(rs);
        p = ret.address;
        assert(vt->length >= ret.varBase);
        vec_resize(vt, ret.varBase);
        goto next;
    }
    case SORCER_OP_Jmp:
    {
        p = inst->arg.address;
        goto next;
    }
    case SORCER_OP_Jnz:
    {
        SORCER_Cell top = vec_last(ds);
        vec_pop(ds);
        if (top.as.val)
        {
            p = inst->arg.address;
        }
        goto next;
    }
    case SORCER_OP_ApplyJmp:
    {
        SORCER_Cell top = vec_last(ds);
        vec_pop(ds);
        p = top.as.address;
        goto next;
    }
    default:
        assert(false);
        break;
    }
}




















u32 SORCER_dsSize(SORCER_Context* ctx)
{
    return ctx->dataStack->length;
}

const SORCER_Cell* SORCER_dsBase(SORCER_Context* ctx)
{
    return ctx->dataStack->data;
}




void SORCER_dsPush(SORCER_Context* ctx, const SORCER_Cell* x)
{
    vec_push(ctx->dataStack, *x);
}

void SORCER_dsPop(SORCER_Context* ctx, u32 n, SORCER_Cell* out)
{
    SORCER_CellVec* ds = ctx->dataStack;
    assert(ds->length >= n);
    memcpy(out, ds->data + ds->length - n, sizeof(SORCER_Cell)*n);
    vec_resize(ds, ds->length - n);
}










































































