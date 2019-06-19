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





typedef struct SORCER_TxnContext
{
    SORCER_Context* sorcer;
} SORCER_TxnContext;




static SORCER_TxnKeyExpr SORCER_txnKeyExprFromHeadName(SORCER_TxnContext* ctx, const char* name)
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




SORCER_Block SORCER_loadTxnBlock(SORCER_Context* ctx, TXN_Space* space, const TXN_Node* seq, u32 len)
{
    SORCER_Block block = SORCER_blockNew(ctx);

    for (u32 i = 0; i < len; ++i)
    {
        TXN_Node node = seq[i];
        if (TXN_isTok(space, node))
        {
            // todo
            SORCER_Cell str = { 0 };
            SORCER_blockAddInstPushCell(ctx, block, str);
        }
        else if (TXN_isSeqCurly(space, node))
        {
            const char* name = TXN_tokCstr(space, node);
            if (0 == strcmp(name, "apply"))
            {
                SORCER_blockAddInstApply(ctx, block);
                continue;
            }
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



SORCER_Block SORCER_loadTxnNode(SORCER_Context* ctx, TXN_Space* space, TXN_Node node)
{
    SORCER_Block block = SORCER_Block_Invalid;
    const TXN_Node* seq = TXN_seqElm(space, node);
    u32 len = TXN_seqLen(space, node);
    if (!len)
    {
        return block;
    }
    return SORCER_loadTxnBlock(ctx, space, seq, len);
}






SORCER_Block SORCER_loadTxnFile(SORCER_Context* ctx, const char* path)
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
    block = SORCER_loadTxnNode(ctx, space, root);
out:
    TXN_spaceSrcInfoFree(srcInfo);
    TXN_spaceFree(space);
    return block;
}

















































































