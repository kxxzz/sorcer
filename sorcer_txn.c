#include "sorcer_a.h"
#include "sorcer_txn.h"
#include <fileu.h>



typedef enum SORCER_TxnKeyExpr
{
    SORCER_TxnKeyExpr_Def,
    SORCER_TxnKeyExpr_Var,
    SORCER_TxnKeyExpr_Ifte,

    SORCER_NumTxnKeyExprs
} SORCER_TxnKeyExpr;

const char** SORCER_TxnKeyExprHeadNameTable(void)
{
    static const char* a[SORCER_NumTxnKeyExprs] =
    {
        "def",
        "var",
        "ifte",
    };
    return a;
}




typedef struct SORCER_TxnLoadVar
{
    const char* name;
    SORCER_Block scope;
    SORCER_Var var;
} SORCER_TxnLoadVar;

typedef vec_t(SORCER_TxnLoadVar) SORCER_TxnLoadVarVec;



typedef struct SORCER_TxnLoadDef
{
    const char* name;
    SORCER_Block scope;
    SORCER_Block block;
} SORCER_TxnLoadDef;

typedef vec_t(SORCER_TxnLoadDef) SORCER_TxnLoadDefVec;



typedef struct SORCER_TxnLoadBlock
{
    SORCER_Block scope;
    SORCER_TxnLoadDefVec defTable[1];
    SORCER_TxnLoadVarVec varTable[1];
} SORCER_TxnLoadBlock;

static void SORCER_txnLoadBlockFree(SORCER_TxnLoadBlock* b)
{
    vec_free(b->varTable);
    vec_free(b->defTable);
}


typedef vec_t(SORCER_TxnLoadBlock) SORCER_TxnLoadBlockVec;





typedef struct SORCER_TxnLoadContext
{
    SORCER_Context* sorcer;
    const SORCER_TxnStepInfoVec* tsiTable;
    TXN_Space* space;
    u32 blockBaseId;
    SORCER_TxnLoadBlockVec blockTable[1];
} SORCER_TxnLoadContext;

static SORCER_TxnLoadContext SORCER_txnLoadContextNew(SORCER_Context* sorcer, const SORCER_TxnStepInfoVec* tsiTable, TXN_Space* space)
{
    SORCER_TxnLoadContext ctx[1] = { 0 };
    ctx->sorcer = sorcer;
    ctx->tsiTable = tsiTable;
    ctx->space = space;
    ctx->blockBaseId = SORCER_ctxBlocksTotal(sorcer);
    return *ctx;
}

static void SORCER_txnLoadContextFree(SORCER_TxnLoadContext* ctx)
{
    for (u32 i = 0; i < ctx->blockTable->length; ++i)
    {
        SORCER_txnLoadBlockFree(ctx->blockTable->data + i);
    }
    vec_free(ctx->blockTable);
}





static SORCER_TxnKeyExpr SORCER_txnKeyExprFromHeadName(const char* name)
{
    for (SORCER_TxnKeyExpr i = 0; i < SORCER_NumTxnKeyExprs; ++i)
    {
        SORCER_TxnKeyExpr k = SORCER_NumTxnKeyExprs - 1 - i;
        const char* s = SORCER_TxnKeyExprHeadNameTable()[k];
        if (0 == strcmp(s, name))
        {
            return k;
        }
    }
    return -1;
}




static SORCER_Var SORCER_txnLoadFindVar(SORCER_TxnLoadContext* ctx, const char* name, SORCER_Block scope)
{
    SORCER_TxnLoadBlockVec* blockTable = ctx->blockTable;
    while (scope.id != SORCER_Block_Invalid.id)
    {
        SORCER_TxnLoadBlock* blockInfo = blockTable->data + scope.id - ctx->blockBaseId;
        for (u32 i = 0; i < blockInfo->varTable->length; ++i)
        {
            SORCER_TxnLoadVar* info = blockInfo->varTable->data + i;
            if (info->name == name)
            {
                return info->var;
            }
        }
        scope = blockInfo->scope;
    }
    return SORCER_Var_Invalid;
}

static SORCER_Block SORCER_txnLoadFindDef(SORCER_TxnLoadContext* ctx, const char* name, SORCER_Block scope)
{
    SORCER_TxnLoadBlockVec* blockTable = ctx->blockTable;
    while (scope.id != SORCER_Block_Invalid.id)
    {
        SORCER_TxnLoadBlock* blockInfo = blockTable->data + scope.id - ctx->blockBaseId;
        for (u32 i = 0; i < blockInfo->defTable->length; ++i)
        {
            SORCER_TxnLoadDef* info = blockInfo->defTable->data + i;
            if (info->name == name)
            {
                return info->block;
            }
        }
        scope = blockInfo->scope;
    }
    return SORCER_Block_Invalid;
}

static SORCER_Step SORCER_txnLoadFindStep(SORCER_TxnLoadContext* ctx, const char* name)
{
    for (u32 i = 0; i < ctx->tsiTable->length; ++i)
    {
        u32 idx = ctx->tsiTable->length - 1 - i;
        const char* s = ctx->tsiTable->data[idx].name;
        if (0 == strcmp(s, name))
        {
            SORCER_Step step = { idx };
            return step;
        }
    }
    return SORCER_Step_Invalid;
}











SORCER_Block SORCER_loadTxnBlock(SORCER_TxnLoadContext* ctx, const TXN_Node* seq, u32 len)
{
    SORCER_Context* sorcer = ctx->sorcer;
    TXN_Space* space = ctx->space;
    SORCER_Block block = SORCER_blockNew(sorcer);

    for (u32 i = 0; i < len; ++i)
    {
        TXN_Node node = seq[i];
        if (TXN_isTok(space, node))
        {
            if (TXN_tokQuoted(space, node))
            {
                // todo
                SORCER_Cell str = { 0 };
                SORCER_blockAddInstPushCell(sorcer, block, str);
            }
            else
            {
                const char* name = TXN_tokCstr(space, node);
                if (0 == strcmp(name, "apply"))
                {
                    SORCER_blockAddInstApply(sorcer, block);
                    continue;
                }
                SORCER_Var var = SORCER_txnLoadFindVar(ctx, name, block);
                if (var.id != SORCER_Var_Invalid.id)
                {
                    SORCER_blockAddInstPushVar(sorcer, block, var);
                    continue;
                }
                SORCER_Block def = SORCER_txnLoadFindDef(ctx, name, block);
                if (def.id != SORCER_Block_Invalid.id)
                {
                    SORCER_blockAddInstPushBlock(sorcer, block, def);
                    continue;
                }
                SORCER_Step step = SORCER_txnLoadFindStep(ctx, name);
                if (step.id != SORCER_Step_Invalid.id)
                {
                    SORCER_blockAddInstStep(sorcer, block, step);
                    continue;
                }
            }
        }
        else if (TXN_isSeqCurly(space, node))
        {

        }
        else if (TXN_isSeqSquare(space, node))
        {

        }
        else
        {

        }
    }
    return block;
}



SORCER_Block SORCER_blockFromTxnNode(SORCER_Context* ctx, const SORCER_TxnStepInfoVec* tsiTable, TXN_Space* space, TXN_Node node)
{
    SORCER_Block block = SORCER_Block_Invalid;
    const TXN_Node* seq = TXN_seqElm(space, node);
    u32 len = TXN_seqLen(space, node);
    if (!len)
    {
        return block;
    }
    SORCER_TxnLoadContext tctx[1] = { SORCER_txnLoadContextNew(ctx, tsiTable, space) };
    SORCER_Block r = SORCER_loadTxnBlock(tctx, seq, len);
    SORCER_txnLoadContextFree(tctx);
    return r;
}






SORCER_Block SORCER_blockFromTxnFile(SORCER_Context* ctx, const SORCER_TxnStepInfoVec* tsiTable, const char* path)
{
    SORCER_Block block = SORCER_Block_Invalid;
    char* str;
    u32 strSize = FILEU_readFile(path, &str);
    if (-1 == strSize)
    {
        return block;
    }
    TXN_Space* space = TXN_spaceNew();
    TXN_SpaceSrcInfo srcInfo[1] = { 0 };
    TXN_Node root = TXN_parseAsList(space, str, srcInfo);
    free(str);
    if (TXN_Node_Invalid.id == root.id)
    {
        goto out;
    }
    block = SORCER_blockFromTxnNode(ctx, tsiTable, space, root);
out:
    TXN_spaceSrcInfoFree(srcInfo);
    TXN_spaceFree(space);
    return block;
}

















































































