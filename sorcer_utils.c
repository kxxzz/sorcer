#include "sorcer_a.h"
#include "sorcer_utils.h"






void SR_dsFprint(FILE* f, SR_Context* ctx)
{
    const SR_Cell* base = SR_dsBase(ctx);
    vec_char buf[1] = { 0 };
    for (u32 i = 0; i < SR_dsSize(ctx); ++i)
    {
        u32 n = SR_cellToStr(NULL, 0, ctx, base + i);
        vec_resize(buf, n + 1);
        n = SR_cellToStr(buf->data, buf->length, ctx, base + i);
        buf->data[n] = 0;
        fprintf(f, "%s", buf->data);
        fprintf(f, "\n");
    }
    vec_free(buf);
}
























































































