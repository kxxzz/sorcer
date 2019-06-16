#include "sorcer_a.h"



typedef struct SORCER_Context
{
    TXN_Space* space;
} SORCER_Context;




SORCER_Context* SORCER_ctxNew(TXN_Space* space)
{
    SORCER_Context* ctx = zalloc(sizeof(SORCER_Context));
    ctx->space = space;
    return ctx;
}


void SORCER_ctxFree(SORCER_Context* ctx)
{
    free(ctx);
}






































































































