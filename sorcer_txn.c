#include "sorcer_a.h"
#include "sorcer_txn.h"
#include <fileu.h>








SORCER_Block SORCER_loadTxnNode(SORCER_Context* ctx, TXN_Space* space, TXN_Node node)
{
    SORCER_Block block = SORCER_Block_Invalid;
    return block;
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

















































































