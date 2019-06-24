#include "sorcer_a.h"
#include "sorcer_utils.h"






void SORCER_dsFprint(FILE* f, SORCER_Context* ctx)
{
    const SORCER_Cell* base = SORCER_dsBase(ctx);
    vec_char buf[1] = { 0 };
    for (u32 i = 0; i < SORCER_dsSize(ctx); ++i)
    {
        u32 n = SORCER_cellToStr(NULL, 0, ctx, base + i);
        vec_resize(buf, n + 1);
        n = SORCER_cellToStr(buf->data, buf->length, ctx, base + i);
        buf->data[n] = 0;
        fprintf(f, "%s", buf->data);
        fprintf(f, "\n");
    }
    vec_free(buf);
}
























































































