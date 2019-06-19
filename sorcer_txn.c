#include "sorcer_a.h"
#include "sorcer_txn.h"
#include <fileu.h>





SORCER_Block SORCER_loadTxnBlock(SORCER_Context* ctx, TXN_Space* space, const TXN_Node* seq, u32 len)
{
    SORCER_Block block = SORCER_blockNew(ctx);

    for (u32 i = 0; i < len; ++i)
    {
        TXN_Node node = seq[i];
        if (TXN_isTok(space, node))
        {
            // todo
            SORCER_Cell x = { 0 };
            SORCER_blockAddInstPushCell(ctx, block, x);
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

















































































