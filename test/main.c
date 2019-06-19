#pragma warning(disable: 4101)

#include <stdlib.h>
#ifdef _WIN32
# include <crtdbg.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <fileu.h>


#include <sorcer.h>
#include <sorcer_txn.h>



static int mainReturn(int r)
{
#if !defined(NDEBUG) && defined(_WIN32)
    system("pause");
#endif
    return r;
}






static void test(void)
{
    TXN_Node root;
    char* text;

    u32 textSize = FILEU_readFile("../1.txn", &text);
    assert(textSize != -1);

    TXN_Space* space = TXN_spaceNew();
    TXN_SpaceSrcInfo srcInfo[1] = { 0 };
    root = TXN_parseAsList(space, text, srcInfo);
    assert(root.id != TXN_Node_Invalid.id);
    free(text);

    SORCER_Context* ctx = SORCER_ctxNew();
    SORCER_Block blockRoot = SORCER_loadTXN(ctx, space, root);
    //SORCER_blockCall(ctx, blockRoot);

    SORCER_ctxFree(ctx);
    TXN_spaceSrcInfoFree(srcInfo);
    TXN_spaceFree(space);
}







int main(int argc, char* argv[])
{
#if !defined(NDEBUG) && defined(_WIN32)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    test();
    return mainReturn(EXIT_SUCCESS);
}






























































