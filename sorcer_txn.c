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
    SORCER_Block block;
    SORCER_Var var;
} SORCER_TxnLoadVar;

typedef vec_t(SORCER_TxnLoadVar) SORCER_TxnLoadVarVec;




typedef struct SORCER_TxnLoadContext
{
    SORCER_Context* sorcer;
    const SORCER_StepTxnInfoVec* stiTable;
    TXN_Space* space;
    SORCER_TxnLoadVarVec varTable[1];
} SORCER_TxnLoadContext;

static SORCER_TxnLoadContext SORCER_txnLoadContextNew(SORCER_Context* sorcer, const SORCER_StepTxnInfoVec* stiTable, TXN_Space* space)
{
    SORCER_TxnLoadContext ctx[1] = { 0 };
    ctx->sorcer = sorcer;
    ctx->stiTable = stiTable;
    ctx->space = space;
    return *ctx;
}

static void SORCER_txnLoadContextFree(SORCER_TxnLoadContext* ctx)
{
    vec_free(ctx->varTable);
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



SORCER_Block SORCER_loadFromTxnNode(SORCER_Context* ctx, const SORCER_StepTxnInfoVec* stiTable, TXN_Space* space, TXN_Node node)
{
    SORCER_Block block = SORCER_Block_Invalid;
    const TXN_Node* seq = TXN_seqElm(space, node);
    u32 len = TXN_seqLen(space, node);
    if (!len)
    {
        return block;
    }
    SORCER_TxnLoadContext tctx[1] = { SORCER_txnLoadContextNew(ctx, stiTable, space) };
    SORCER_Block r = SORCER_loadTxnBlock(tctx, seq, len);
    SORCER_txnLoadContextFree(tctx);
    return r;
}






SORCER_Block SORCER_loadFromTxnFile(SORCER_Context* ctx, const SORCER_StepTxnInfoVec* stiTable, const char* path)
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
    block = SORCER_loadFromTxnNode(ctx, stiTable, space, root);
out:
    TXN_spaceSrcInfoFree(srcInfo);
    TXN_spaceFree(space);
    return block;
}

















































































