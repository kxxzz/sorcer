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
    SORCER_Context* ctx = SORCER_ctxNew();
    SORCER_StepTxnInfoVec stiTable[1] = { 0 };

    SORCER_Block blk = SORCER_loadFromTxnFile(ctx, stiTable, "../1.txn");
    assert(blk.id != SORCER_Block_Invalid.id);
    SORCER_run(ctx, blk);

    vec_free(stiTable);
    SORCER_ctxFree(ctx);
}







int main(int argc, char* argv[])
{
#if !defined(NDEBUG) && defined(_WIN32)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    test();
    return mainReturn(EXIT_SUCCESS);
}






























































