#include "sorcer_a.h"
#include "sorcer_utils.h"






void SORCER_dsFprint(FILE* f, SORCER_Context* ctx)
{
    const SORCER_Cell* base = SORCER_dsBase(ctx);
    char buf[1024] = { 0 };
    for (u32 i = 0; i < SORCER_dsSize(ctx); ++i)
    {
        u32 n = SORCER_cellToStr(buf, sizeof(buf), ctx, base + i);
        buf[n] = 0;
        fprintf(f, "%s", buf);
        fprintf(f, "\n");
    }
}
























































































