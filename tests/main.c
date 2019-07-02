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
#include <sorcer_utils.h>



static int mainReturn(int r)
{
#if !defined(NDEBUG) && defined(_WIN32)
    system("pause");
#endif
    return r;
}






static void test(void)
{
    SR_Context* ctx = SR_ctxNew();
    SR_ArithContext arithCtx[1] = { 0 };
    SR_arith(ctx, arithCtx);
    SR_ManaFileInfoVec fileTable[1] = { 0 };

    SR_ManaErrorInfo compErrInfo[1] = { 0 };
    SR_Block blk = SR_blockFromManaFile(ctx, "../1.mana", compErrInfo, fileTable);
    assert(blk.id != SR_Block_Invalid.id);
    assert(SR_ManaError_NONE == compErrInfo->error);

    SR_RunErrorInfo runErrInfo[1];
    SR_run(ctx, blk, runErrInfo);
    assert(SR_RunError_NONE == runErrInfo->error);

    SR_dsFprint(stdout, ctx);

    vec_free(fileTable);
    SR_ctxFree(ctx);
}







int main(int argc, char* argv[])
{
#if !defined(NDEBUG) && defined(_WIN32)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    test();
    return mainReturn(EXIT_SUCCESS);
}






























































