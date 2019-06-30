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
#include <sorcer_mana.h>
#include <sorcer_arith.h>



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
    SORCER_ArithContext arithCtx[1] = { 0 };
    SORCER_arith(ctx, arithCtx);
    SORCER_ManaErrorInfo errInfo[1] = { 0 };
    SORCER_ManaFileInfoVec fileTable[1] = { 0 };

    SORCER_Block blk = SORCER_blockFromManaFile(ctx, "../1.mana", errInfo, fileTable);
    assert(blk.id != SORCER_Block_Invalid.id);
    assert(SORCER_ManaError_NONE == errInfo->error);
    SORCER_run(ctx, blk);

    vec_free(fileTable);
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





























































