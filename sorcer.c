#include "sorcer_a.h"





typedef enum SR_OP
{
    SR_OP_Nop = 0,

    SR_OP_PopNull,
    SR_OP_PopVar,
    SR_OP_PushImm,
    SR_OP_PushVar,
    SR_OP_PushBlock,
    SR_OP_Call,
    SR_OP_Apply,
    SR_OP_Opr,

    SR_OP_Ret,
    SR_OP_TailCall,
    SR_OP_TailApply,

    SR_NumOPs
} SR_OP;


typedef struct SR_InstImm
{
    SR_Type type;
    u32 src;
} SR_InstImm;

typedef struct SR_Inst
{
    SR_OP op;
    union
    {
        SR_InstImm imm;
        SR_Var var;
        SR_Block block;
        SR_Opr opr;
        u32 address;
    } arg;
} SR_Inst;

typedef vec_t(SR_Inst) SR_InstVec;




typedef struct SR_BlockInfo
{
    u32 varCount;
    SR_InstVec code[1];
    u32 calleeCount;
    u32 baseAddress;
} SR_BlockInfo;

static void SR_blockInfoFree(SR_BlockInfo* a)
{
    vec_free(a->code);
}




typedef struct SR_Ret
{
    u32 address;
    u32 varBase;
} SR_Ret;



typedef vec_t(SR_TypeInfo) SR_TypeInfoVec;
typedef vec_t(SR_Cell) SR_CellVec;
typedef vec_t(SR_OprInfo) SR_OprInfoVec;
typedef vec_t(SR_BlockInfo) SR_BlockInfoVec;
typedef vec_t(SR_Ret) SR_RetVec;


typedef struct SR_Context
{
    SR_TypeInfoVec typeTable[1];
    SR_OprInfoVec oprTable[1];
    SR_BlockInfoVec blockTable[1];
    vec_ptr typePoolTable[1];
    upool_t dataPool;

    SR_CellVec dataStack[1];

    bool codeUpdated;
    SR_InstVec code[1];
    SR_RetVec retStack[1];

    SR_CellVec inBuf[1];
    SR_CellVec varTable[1];
} SR_Context;




static u32 SR_cellToStr_Block(char* buf, u32 bufSize, void* pool, const SR_CellValue* x)
{
    return snprintf(buf, bufSize, "<block 0x%08llx>", x->val);
}


SR_Context* SR_ctxNew(void)
{
    SR_Context* ctx = zalloc(sizeof(SR_Context));
    ctx->dataPool = upool_new(256);

    SR_TypeInfo blkTypeInfo[1] =
    {
        {
            "block",
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            SR_cellToStr_Block,
        }
    };
    SR_Type blkType = SR_typeNew(ctx, blkTypeInfo);
    assert(SR_Type_Block.id == blkType.id);
    return ctx;
}

void SR_ctxFree(SR_Context* ctx)
{
    assert(0 == ctx->varTable->length);
    vec_free(ctx->varTable);
    vec_free(ctx->inBuf);

    vec_free(ctx->retStack);
    vec_free(ctx->code);

    for (u32 i = 0; i < ctx->dataStack->length; ++i)
    {
        SR_cellFree(ctx, ctx->dataStack->data + i);
    }
    vec_free(ctx->dataStack);

    upool_free(ctx->dataPool);

    assert(ctx->typePoolTable->length == ctx->typeTable->length);
    for (u32 i = 0; i < ctx->typeTable->length; ++i)
    {
        SR_TypeInfo* info = ctx->typeTable->data + i;
        if (info->poolDtor)
        {
            info->poolDtor(ctx->typePoolTable->data[i]);
        }
    }
    vec_free(ctx->typePoolTable);

    for (u32 i = 0; i < ctx->blockTable->length; ++i)
    {
        SR_blockInfoFree(ctx->blockTable->data + i);
    }
    vec_free(ctx->blockTable);
    vec_free(ctx->oprTable);
    vec_free(ctx->typeTable);
    free(ctx);
}









static void SR_ctxCellVecResize(SR_Context* ctx, SR_CellVec* a, u32 n)
{
    for (u32 i = n; i < a->length; ++i)
    {
        SR_cellFree(ctx, a->data + i);
    }
    vec_resize(a, n);
}












SR_Type SR_typeNew(SR_Context* ctx, const SR_TypeInfo* info)
{
    SR_TypeInfoVec* tt = ctx->typeTable;
    vec_ptr* pt = ctx->typePoolTable;
    assert(tt->length == pt->length);
    void* pool = NULL;
    if (info->poolCtor)
    {
        pool = info->poolCtor();
    }
    vec_push(pt, pool);
    vec_push(tt, *info);
    SR_Type a = { tt->length - 1 };
    return a;
}


SR_Type SR_typeByName(SR_Context* ctx, const char* name)
{
    SR_TypeInfoVec* tt = ctx->typeTable;
    for (u32 i = 0; i < tt->length; ++i)
    {
        u32 idx = tt->length - 1 - i;
        const char* s = tt->data[idx].name;
        if (0 == strcmp(s, name))
        {
            SR_Type type = { idx };
            return type;
        }
    }
    return SR_Type_Invalid;
}


SR_Type SR_typeByIndex(SR_Context* ctx, u32 idx)
{
    SR_TypeInfoVec* tt = ctx->typeTable;
    assert(idx < tt->length);
    SR_Type type = { idx };
    return type;
}


const SR_TypeInfo* SR_typeInfo(SR_Context* ctx, SR_Type type)
{
    SR_TypeInfoVec* tt = ctx->typeTable;
    assert(type.id < tt->length);
    return tt->data + type.id;
}




u32 SR_ctxTypesTotal(SR_Context* ctx)
{
    SR_TypeInfoVec* tt = ctx->typeTable;
    return tt->length;
}



void* SR_pool(SR_Context* ctx, SR_Type type)
{
    vec_ptr* pt = ctx->typePoolTable;
    assert(type.id < pt->length);
    return pt->data[type.id];
}




bool SR_cellNew(SR_Context* ctx, SR_Type type, const char* str, SR_Cell* out)
{
    SR_TypeInfoVec* tt = ctx->typeTable;
    vec_ptr* pt = ctx->typePoolTable;
    assert(tt->length == pt->length);
    assert(type.id < tt->length);
    SR_TypeInfo* info = tt->data + type.id;
    void* pool = pt->data[type.id];
    out->type = type;
    return info->ctor ? info->ctor(pool, str, out->value) : false;
}

void SR_cellFree(SR_Context* ctx, SR_Cell* x)
{
    SR_TypeInfoVec* tt = ctx->typeTable;
    vec_ptr* pt = ctx->typePoolTable;
    assert(tt->length == pt->length);
    assert(x->type.id < tt->length);
    SR_TypeInfo* info = tt->data + x->type.id;
    if (info->dtor)
    {
        void* pool = pt->data[x->type.id];
        info->dtor(pool, x->value);
    }
}

void SR_cellDup(SR_Context* ctx, const SR_Cell* x, SR_Cell* out)
{
    SR_TypeInfoVec* tt = ctx->typeTable;
    vec_ptr* pt = ctx->typePoolTable;
    assert(tt->length == pt->length);
    assert(x->type.id < tt->length);
    SR_TypeInfo* info = tt->data + x->type.id;
    out->type = x->type;
    if (info->copier)
    {
        void* pool = pt->data[x->type.id];
        info->copier(pool, x->value, out->value);
    }
    else
    {
        *out->value = *x->value;
    }
}




u32 SR_cellToStr(char* buf, u32 bufSize, SR_Context* ctx, const SR_Cell* x)
{
    SR_TypeInfoVec* tt = ctx->typeTable;
    vec_ptr* pt = ctx->typePoolTable;
    assert(tt->length == pt->length);
    assert(x->type.id < tt->length);
    SR_TypeInfo* info = tt->data + x->type.id;
    assert(info->toStr);
    void* pool = pt->data[x->type.id];
    return info->toStr(buf, bufSize, pool, x->value);
}










SR_Opr SR_oprNew(SR_Context* ctx, const SR_OprInfo* info)
{
    SR_OprInfoVec* ot = ctx->oprTable;
    vec_push(ot, *info);
    SR_Opr a = { ot->length - 1 };
    return a;
}


SR_Opr SR_oprByName(SR_Context* ctx, const char* name)
{
    SR_OprInfoVec* ot = ctx->oprTable;
    for (u32 i = 0; i < ot->length; ++i)
    {
        u32 idx = ot->length - 1 - i;
        const char* s = ot->data[idx].name;
        if (0 == strcmp(s, name))
        {
            SR_Opr opr = { idx };
            return opr;
        }
    }
    return SR_Opr_Invalid;
}


SR_Opr SR_oprByIndex(SR_Context* ctx, u32 idx)
{
    SR_OprInfoVec* ot = ctx->oprTable;
    assert(idx < ot->length);
    SR_Opr opr = { idx };
    return opr;
}


const SR_OprInfo* SR_oprInfo(SR_Context* ctx, SR_Opr opr)
{
    SR_OprInfoVec* ot = ctx->oprTable;
    assert(opr.id < ot->length);
    return ot->data + opr.id;
}






u32 SR_ctxBlocksTotal(SR_Context* ctx)
{
    return ctx->blockTable->length;
}

void SR_ctxBlocksRollback(SR_Context* ctx, u32 n)
{
    for (u32 i = n; i < ctx->blockTable->length; ++i)
    {
        SR_blockInfoFree(ctx->blockTable->data + i);
    }
    vec_resize(ctx->blockTable, n);
}












static void SR_codeOutdate(SR_Context* ctx)
{
    if (ctx->codeUpdated)
    {
        ctx->codeUpdated = false;
    }
}




SR_Block SR_blockNew(SR_Context* ctx)
{
    SR_codeOutdate(ctx);
    SR_BlockInfoVec* bt = ctx->blockTable;
    SR_BlockInfo binfo = { 0 };
    vec_push(bt, binfo);
    SR_Block a = { bt->length - 1 };
    return a;
}










void SR_blockAddInstPopNull(SR_Context* ctx, SR_Block blk)
{
    SR_codeOutdate(ctx);
    SR_BlockInfoVec* bt = ctx->blockTable;
    SR_BlockInfo* binfo = bt->data + blk.id;
    SR_Inst inst = { SR_OP_PopNull };
    vec_push(binfo->code, inst);
}



SR_Var SR_blockAddInstPopVar(SR_Context* ctx, SR_Block blk)
{
    SR_codeOutdate(ctx);
    SR_BlockInfoVec* bt = ctx->blockTable;
    SR_BlockInfo* binfo = bt->data + blk.id;
    SR_Var var = { binfo->varCount++ };
    SR_Inst inst = { SR_OP_PopVar, .arg.var = var };
    vec_push(binfo->code, inst);
    return var;
}




void SR_blockAddInstPushImm(SR_Context* ctx, SR_Block blk, SR_Type type, const char* str)
{
    SR_codeOutdate(ctx);
    SR_BlockInfoVec* bt = ctx->blockTable;
    SR_BlockInfo* binfo = bt->data + blk.id;
    u32 off = upool_elm(ctx->dataPool, str, (u32)strlen(str) + 1, NULL);
    SR_InstImm imm = { type, off };
    SR_Inst inst = { SR_OP_PushImm, .arg.imm = imm };
    vec_push(binfo->code, inst);
}


void SR_blockAddInstPushVar(SR_Context* ctx, SR_Block blk, SR_Var v)
{
    SR_codeOutdate(ctx);
    SR_BlockInfoVec* bt = ctx->blockTable;
    SR_BlockInfo* binfo = bt->data + blk.id;
    SR_Inst inst = { SR_OP_PushVar, .arg.var = v };
    vec_push(binfo->code, inst);
}


void SR_blockAddInstPushBlock(SR_Context* ctx, SR_Block blk, SR_Block b)
{
    SR_codeOutdate(ctx);
    SR_BlockInfoVec* bt = ctx->blockTable;
    SR_BlockInfo* binfo = bt->data + blk.id;
    ++bt->data[b.id].calleeCount;
    SR_Inst inst = { SR_OP_PushBlock, .arg.block = b };
    vec_push(binfo->code, inst);
}


void SR_blockAddInstCall(SR_Context* ctx, SR_Block blk, SR_Block callee)
{
    SR_codeOutdate(ctx);
    SR_BlockInfoVec* bt = ctx->blockTable;
    SR_BlockInfo* binfo = bt->data + blk.id;
    ++bt->data[callee.id].calleeCount;
    SR_Inst inst = { SR_OP_Call, .arg.block = callee };
    vec_push(binfo->code, inst);
}


void SR_blockAddInstApply(SR_Context* ctx, SR_Block blk)
{
    SR_codeOutdate(ctx);
    SR_BlockInfoVec* bt = ctx->blockTable;
    SR_BlockInfo* binfo = bt->data + blk.id;
    SR_Inst inst = { SR_OP_Apply };
    vec_push(binfo->code, inst);
}


void SR_blockAddInstOpr(SR_Context* ctx, SR_Block blk, SR_Opr opr)
{
    SR_codeOutdate(ctx);
    SR_BlockInfoVec* bt = ctx->blockTable;
    SR_BlockInfo* binfo = bt->data + blk.id;
    SR_Inst inst = { SR_OP_Opr, .arg.opr = opr };
    vec_push(binfo->code, inst);
}













void SR_blockAddInlineBlock(SR_Context* ctx, SR_Block blk, SR_Block ib)
{
    SR_BlockInfoVec* bt = ctx->blockTable;
    SR_BlockInfo* bInfo = bt->data + blk.id;
    SR_BlockInfo* iInfo = bt->data + ib.id;
    for (u32 i = 0; i < iInfo->code->length; ++i)
    {
        SR_Inst inst = iInfo->code->data[i];
        switch (inst.op)
        {
        case SR_OP_PopVar:
        case SR_OP_PushVar:
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
























static void SR_codeUpdate(SR_Context* ctx, SR_Block blk)
{
    if (ctx->codeUpdated)
    {
        return;
    }
    ctx->codeUpdated = true;

    SR_InstVec* code = ctx->code;
    SR_BlockInfoVec* bt = ctx->blockTable;

    for (u32 bi = 0; bi < bt->length; ++bi)
    {
        SR_BlockInfo* blkInfo = bt->data + bi;
        if (!blkInfo->calleeCount && (bi != blk.id))
        {
            continue;
        }
        blkInfo->baseAddress = code->length;
        for (u32 i = 0; i < blkInfo->code->length; ++i)
        {
            SR_Inst inst = blkInfo->code->data[i];
            // Tail Call Elimination
            if (i + 1 == blkInfo->code->length)
            {
                switch (inst.op)
                {
                case SR_OP_Call:
                {
                    inst.op = SR_OP_TailCall;
                    break;
                }
                case SR_OP_Apply:
                {
                    inst.op = SR_OP_TailApply;
                    break;
                }
                default:
                    break;
                }
            }
            vec_push(code, inst);
        }
        SR_Inst ret = { SR_OP_Ret };
        vec_push(code, ret);
    }

    for (u32 i = 0; i < code->length; ++i)
    {
        SR_Inst* inst = code->data + i;
        switch (inst->op)
        {
        case SR_OP_PushBlock:
        case SR_OP_Call:
        case SR_OP_TailCall:
        {
            SR_BlockInfo* blkInfo = bt->data + inst->arg.block.id;
            inst->arg.address = blkInfo->baseAddress;
            break;
        }
        default:
            break;
        }
    }
}




















void SR_run(SR_Context* ctx, SR_Block blk, SR_RunErrorInfo* errInfo)
{
    memset(errInfo, 0, sizeof(*errInfo));
    SR_codeUpdate(ctx, blk);

    SR_InstVec* code = ctx->code;
    SR_BlockInfoVec* bt = ctx->blockTable;
    SR_CellVec* ds = ctx->dataStack;
    SR_RetVec* rs = ctx->retStack;
    SR_CellVec* vt = ctx->varTable;

    assert(blk.id < bt->length);
    u32 p = bt->data[blk.id].baseAddress;
    const SR_Inst* inst = NULL;
next:
    inst = code->data + p++;
    switch (inst->op)
    {
    case SR_OP_Nop:
    {
        goto next;
    }
    case SR_OP_PopNull:
    {
        SR_Cell top = vec_last(ds);
        vec_pop(ds);
        SR_cellFree(ctx, &top);
        goto next;
    }
    case SR_OP_PopVar:
    {
        SR_Cell top = vec_last(ds);
        vec_pop(ds);
        vec_push(vt, top);
        goto next;
    }
    case SR_OP_PushImm:
    {
        const SR_InstImm* imm = &inst->arg.imm;
        const char* str = upool_elmData(ctx->dataPool, imm->src);
        SR_Cell cell;
        bool r = SR_cellNew(ctx, imm->type, str, &cell);
        assert(r);
        vec_push(ds, cell);
        goto next;
    }
    case SR_OP_PushVar:
    {
        u32 varBase = 0;
        if (rs->length > 0)
        {
            SR_Ret ret = vec_last(rs);
            varBase = ret.varBase;
        }
        SR_Cell cell;
        SR_cellDup(ctx, &vt->data[varBase + inst->arg.var.id], &cell);
        vec_push(ds, cell);
        goto next;
    }
    case SR_OP_PushBlock:
    {
        SR_Cell cell = { SR_Type_Block.id, { { .blk = inst->arg.block } } };
        vec_push(ds, cell);
        goto next;
    }
    case SR_OP_Call:
    {
        SR_Ret ret = { p, vt->length };
        vec_push(rs, ret);
        p = inst->arg.address;
        goto next;
    }
    case SR_OP_Apply:
    {
        SR_Ret ret = { p, vt->length };
        vec_push(rs, ret);
        SR_Cell top = vec_last(ds);
        if (top.type.id != SR_Type_Block.id)
        {
            errInfo->error = SR_RunError_OprArgs;
            goto failed;
        }
        vec_pop(ds);
        p = (u32)top.value->val;
        goto next;
    }
    case SR_OP_Opr:
    {
        const SR_OprInfo* info = ctx->oprTable->data + inst->arg.opr.id;
        SR_CellVec* inBuf = ctx->inBuf;
        SR_CellVec* ds = ctx->dataStack;

        if (ds->length < info->numIns)
        {
            errInfo->error = SR_RunError_OprArgs;
            goto failed;
        }

        SR_Cell* ins = ds->data + ds->length - info->numIns;
        for (u32 i = 0; i < info->numIns; ++i)
        {
            if (info->ins[i].id != ins[i].type.id)
            {
                errInfo->error = SR_RunError_OprArgs;
                goto failed;
            }
        }

        vec_resize(inBuf, info->numIns);
        vec_resize(ds, ds->length + info->numOuts - info->numIns);

        memcpy(inBuf->data, ds->data + ds->length - info->numOuts, sizeof(SR_Cell)*info->numIns);

        SR_Cell* outs = ds->data + ds->length - info->numOuts;
        info->func(ctx, info->funcCtx, inBuf->data, outs);
        for (u32 i = 0; i < info->numOuts; ++i)
        {
            outs[i].type = info->outs[i];
        }
        goto next;
    }
    case SR_OP_Ret:
    {
        if (!rs->length)
        {
            SR_ctxCellVecResize(ctx, vt, 0);
            return;
        }
        SR_Ret ret = vec_last(rs);
        vec_pop(rs);
        p = ret.address;
        assert(vt->length >= ret.varBase);
        SR_ctxCellVecResize(ctx, vt, ret.varBase);
        goto next;
    }
    case SR_OP_TailCall:
    {
        if (rs->length > 0)
        {
            SR_Ret ret = vec_last(rs);
            assert(vt->length >= ret.varBase);
            SR_ctxCellVecResize(ctx, vt, ret.varBase);
        }
        else
        {
            SR_ctxCellVecResize(ctx, vt, 0);
        }
        p = inst->arg.address;
        goto next;
    }
    case SR_OP_TailApply:
    {
        if (rs->length > 0)
        {
            SR_Ret ret = vec_last(rs);
            assert(vt->length >= ret.varBase);
            SR_ctxCellVecResize(ctx, vt, ret.varBase);
        }
        else
        {
            SR_ctxCellVecResize(ctx, vt, 0);
        }
        SR_Cell top = vec_last(ds);
        if (top.type.id != SR_Type_Block.id)
        {
            errInfo->error = SR_RunError_OprArgs;
            goto failed;
        }
        vec_pop(ds);
        p = (u32)top.value->val;
        goto next;
    }
    default:
        assert(false);
        goto next;
    }
failed:
    SR_ctxCellVecResize(ctx, vt, 0);
    return;
}




















u32 SR_dsSize(SR_Context* ctx)
{
    return ctx->dataStack->length;
}

const SR_Cell* SR_dsBase(SR_Context* ctx)
{
    return ctx->dataStack->data;
}




void SR_dsPush(SR_Context* ctx, const SR_Cell* x)
{
    vec_push(ctx->dataStack, *x);
}

void SR_dsPop(SR_Context* ctx, u32 n, SR_Cell* out)
{
    SR_CellVec* ds = ctx->dataStack;
    assert(ds->length >= n);
    memcpy(out, ds->data + ds->length - n, sizeof(SR_Cell)*n);
    vec_resize(ds, ds->length - n);
}










































































