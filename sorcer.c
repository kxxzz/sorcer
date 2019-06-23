#include "sorcer_a.h"





typedef enum SORCER_OP
{
    SORCER_OP_PopVar = 0,
    SORCER_OP_PushImm,
    SORCER_OP_PushVar,
    SORCER_OP_PushBlock,
    SORCER_OP_Call,
    SORCER_OP_Apply,
    SORCER_OP_Opr,

    SORCER_OP_Ret,
    SORCER_OP_Jmp,
    SORCER_OP_JmpDS,

    SORCER_NumOPs
} SORCER_OP;


typedef struct SORCER_InstImm
{
    SORCER_Type type;
    u32 src;
} SORCER_InstImm;

typedef struct SORCER_Inst
{
    SORCER_OP op;
    union
    {
        SORCER_InstImm imm;
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



typedef vec_t(SORCER_TypeInfo) SORCER_TypeInfoVec;
typedef vec_t(SORCER_Cell) SORCER_CellVec;
typedef vec_t(SORCER_OprInfo) SORCER_OprInfoVec;
typedef vec_t(SORCER_BlockInfo) SORCER_BlockInfoVec;
typedef vec_t(SORCER_Ret) SORCER_RetVec;


typedef struct SORCER_Context
{
    SORCER_TypeInfoVec typeTable[1];
    SORCER_OprInfoVec oprTable[1];
    SORCER_BlockInfoVec blockTable[1];
    vec_ptr typePoolTable[1];
    upool_t dataPool;

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
    ctx->dataPool = upool_new(256);
    return ctx;
}

void SORCER_ctxFree(SORCER_Context* ctx)
{
    vec_free(ctx->varTable);
    vec_free(ctx->inBuf);

    vec_free(ctx->retStack);
    vec_free(ctx->code);

    vec_free(ctx->dataStack);

    upool_free(ctx->dataPool);

    assert(ctx->typePoolTable->length == ctx->typeTable->length);
    for (u32 i = 0; i < ctx->typeTable->length; ++i)
    {
        SORCER_TypeInfo* info = ctx->typeTable->data + i;
        if (info->poolDtor)
        {
            info->poolDtor(ctx->typePoolTable->data[i]);
        }
    }
    vec_free(ctx->typePoolTable);

    for (u32 i = 0; i < ctx->blockTable->length; ++i)
    {
        SORCER_blockInfoFree(ctx->blockTable->data + i);
    }
    vec_free(ctx->blockTable);
    vec_free(ctx->oprTable);
    vec_free(ctx->typeTable);
    free(ctx);
}









SORCER_Type SORCER_typeNew(SORCER_Context* ctx, const SORCER_TypeInfo* info)
{
    SORCER_TypeInfoVec* tt = ctx->typeTable;
    vec_ptr* pt = ctx->typePoolTable;
    assert(tt->length == pt->length);
    void* pool = NULL;
    if (info->poolCtor)
    {
        pool = info->poolCtor();
    }
    vec_push(pt, pool);
    vec_push(tt, *info);
    SORCER_Type a = { tt->length - 1 };
    return a;
}


SORCER_Type SORCER_typeByName(SORCER_Context* ctx, const char* name)
{
    SORCER_TypeInfoVec* tt = ctx->typeTable;
    for (u32 i = 0; i < tt->length; ++i)
    {
        u32 idx = tt->length - 1 - i;
        const char* s = tt->data[idx].name;
        if (0 == strcmp(s, name))
        {
            SORCER_Type type = { idx };
            return type;
        }
    }
    return SORCER_Type_Invalid;
}


SORCER_Type SORCER_typeByIndex(SORCER_Context* ctx, u32 idx)
{
    SORCER_TypeInfoVec* tt = ctx->typeTable;
    assert(idx < tt->length);
    SORCER_Type type = { idx };
    return type;
}




u32 SORCER_ctxTypesTotal(SORCER_Context* ctx)
{
    SORCER_TypeInfoVec* tt = ctx->typeTable;
    return tt->length;
}



void* SORCER_pool(SORCER_Context* ctx, SORCER_Type type)
{
    vec_ptr* pt = ctx->typePoolTable;
    assert(type.id < pt->length);
    return pt->data[type.id];
}




bool SORCER_cellNew(SORCER_Context* ctx, SORCER_Type type, const char* str, SORCER_Cell* out)
{
    SORCER_TypeInfoVec* tt = ctx->typeTable;
    vec_ptr* pt = ctx->typePoolTable;
    assert(tt->length == pt->length);
    assert(type.id < tt->length);
    SORCER_TypeInfo* info = tt->data + type.id;
    void* pool = pt->data[type.id];
    out->type = type;
    return info->ctor(pool, str, &out->as.ptr);
}

void SORCER_cellFree(SORCER_Context* ctx, SORCER_Cell* x)
{
    SORCER_TypeInfoVec* tt = ctx->typeTable;
    vec_ptr* pt = ctx->typePoolTable;
    assert(tt->length == pt->length);
    assert(x->type.id < tt->length);
    SORCER_TypeInfo* info = tt->data + x->type.id;
    void* pool = pt->data[x->type.id];
    info->dtor(pool, x->as.ptr);
}




u32 SORCER_cellToStr(char* buf, u32 bufSize, SORCER_Context* ctx, const SORCER_Cell* x)
{
    SORCER_TypeInfoVec* tt = ctx->typeTable;
    vec_ptr* pt = ctx->typePoolTable;
    assert(tt->length == pt->length);
    assert(x->type.id < tt->length);
    SORCER_TypeInfo* info = tt->data + x->type.id;
    void* pool = pt->data[x->type.id];
    return info->toStr(buf, bufSize, pool, x);
}










SORCER_Opr SORCER_oprNew(SORCER_Context* ctx, const SORCER_OprInfo* info)
{
    SORCER_OprInfoVec* ot = ctx->oprTable;
    vec_push(ot, *info);
    SORCER_Opr a = { ot->length - 1 };
    return a;
}


SORCER_Opr SORCER_oprByName(SORCER_Context* ctx, const char* name)
{
    SORCER_OprInfoVec* ot = ctx->oprTable;
    for (u32 i = 0; i < ot->length; ++i)
    {
        u32 idx = ot->length - 1 - i;
        const char* s = ot->data[idx].name;
        if (0 == strcmp(s, name))
        {
            SORCER_Opr opr = { idx };
            return opr;
        }
    }
    return SORCER_Opr_Invalid;
}


SORCER_Opr SORCER_oprByIndex(SORCER_Context* ctx, u32 idx)
{
    SORCER_OprInfoVec* ot = ctx->oprTable;
    assert(idx < ot->length);
    SORCER_Opr opr = { idx };
    return opr;
}








u32 SORCER_ctxBlocksTotal(SORCER_Context* ctx)
{
    return ctx->blockTable->length;
}

void SORCER_ctxBlocksRollback(SORCER_Context* ctx, u32 n)
{
    for (u32 i = n; i < ctx->blockTable->length; ++i)
    {
        SORCER_blockInfoFree(ctx->blockTable->data + i);
    }
    vec_resize(ctx->blockTable, n);
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
    SORCER_BlockInfoVec* bt = ctx->blockTable;
    SORCER_BlockInfo binfo = { 0 };
    vec_push(bt, binfo);
    SORCER_Block a = { bt->length - 1 };
    return a;
}










SORCER_Var SORCER_blockAddInstPopVar(SORCER_Context* ctx, SORCER_Block blk)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockTable;
    SORCER_BlockInfo* binfo = bt->data + blk.id;
    SORCER_Var var = { binfo->varCount++ };
    SORCER_Inst inst = { SORCER_OP_PopVar, .arg.var = var };
    vec_push(binfo->code, inst);
    return var;
}




void SORCER_blockAddInstPushImm(SORCER_Context* ctx, SORCER_Block blk, SORCER_Type type, const char* str)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockTable;
    SORCER_BlockInfo* binfo = bt->data + blk.id;
    u32 off = upool_elm(ctx->dataPool, str, (u32)strlen(str) + 1, NULL);
    SORCER_InstImm imm = { type, off };
    SORCER_Inst inst = { SORCER_OP_PushImm, .arg.imm = imm };
    vec_push(binfo->code, inst);
}


void SORCER_blockAddInstPushVar(SORCER_Context* ctx, SORCER_Block blk, SORCER_Var v)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockTable;
    SORCER_BlockInfo* binfo = bt->data + blk.id;
    SORCER_Inst inst = { SORCER_OP_PushVar, .arg.var = v };
    vec_push(binfo->code, inst);
}


void SORCER_blockAddInstPushBlock(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block b)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockTable;
    SORCER_BlockInfo* binfo = bt->data + blk.id;
    ++bt->data[b.id].calleeCount;
    SORCER_Inst inst = { SORCER_OP_PushBlock, .arg.block = b };
    vec_push(binfo->code, inst);
}


void SORCER_blockAddInstCall(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block callee)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockTable;
    SORCER_BlockInfo* binfo = bt->data + blk.id;
    ++bt->data[callee.id].calleeCount;
    SORCER_Inst inst = { SORCER_OP_Call, .arg.block = callee };
    vec_push(binfo->code, inst);
}


void SORCER_blockAddInstApply(SORCER_Context* ctx, SORCER_Block blk)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockTable;
    SORCER_BlockInfo* binfo = bt->data + blk.id;
    SORCER_Inst inst = { SORCER_OP_Apply };
    vec_push(binfo->code, inst);
}


void SORCER_blockAddInstOpr(SORCER_Context* ctx, SORCER_Block blk, SORCER_Opr opr)
{
    SORCER_codeOutdate(ctx);
    SORCER_BlockInfoVec* bt = ctx->blockTable;
    SORCER_BlockInfo* binfo = bt->data + blk.id;
    SORCER_Inst inst = { SORCER_OP_Opr, .arg.opr = opr };
    vec_push(binfo->code, inst);
}













void SORCER_blockAddInlineBlock(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block ib)
{
    SORCER_BlockInfoVec* bt = ctx->blockTable;
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
























static void SORCER_codeUpdate(SORCER_Context* ctx, SORCER_Block blk)
{
    if (ctx->codeUpdated)
    {
        return;
    }
    ctx->codeUpdated = true;

    SORCER_InstVec* code = ctx->code;
    SORCER_BlockInfoVec* bt = ctx->blockTable;

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
                    inst.op = SORCER_OP_JmpDS;
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














static void SORCER_runOpr(SORCER_Context* ctx, SORCER_Opr opr)
{
    const SORCER_OprInfo* info = ctx->oprTable->data + opr.id;
    SORCER_CellVec* inBuf = ctx->inBuf;
    SORCER_CellVec* ds = ctx->dataStack;

    vec_resize(inBuf, info->numIns);
    vec_resize(ds, ds->length + info->numOuts - info->numIns);

    memcpy(inBuf->data, ds->data + ds->length - info->numOuts, sizeof(SORCER_Cell)*info->numIns);

    info->func(ctx, info->funcCtx, inBuf->data, ds->data + ds->length - info->numOuts);
}













void SORCER_run(SORCER_Context* ctx, SORCER_Block blk)
{
    SORCER_codeUpdate(ctx, blk);

    SORCER_InstVec* code = ctx->code;
    SORCER_BlockInfoVec* bt = ctx->blockTable;
    SORCER_CellVec* ds = ctx->dataStack;
    SORCER_RetVec* rs = ctx->retStack;
    SORCER_CellVec* vt = ctx->varTable;

    assert(blk.id < bt->length);
    u32 p = bt->data[blk.id].baseAddress;
    const SORCER_Inst* inst = NULL;
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
    case SORCER_OP_PushImm:
    {
        const SORCER_InstImm* imm = &inst->arg.imm;
        const char* str = upool_elmData(ctx->dataPool, imm->src);
        SORCER_Cell cell;
        bool r = SORCER_cellNew(ctx, imm->type, str, &cell);
        assert(r);
        vec_push(ds, cell);
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
    case SORCER_OP_Opr:
    {
        SORCER_runOpr(ctx, inst->arg.opr);
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
    case SORCER_OP_JmpDS:
    {
        SORCER_Cell top = vec_last(ds);
        vec_pop(ds);
        p = top.as.address;
        goto next;
    }
    default:
        assert(false);
        goto next;
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










































































