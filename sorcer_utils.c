#include "sorcer_a.h"
#include "sorcer_utils.h"






void SORCER_dsFprint(FILE* f, SORCER_Context* ctx)
{
    const SORCER_Cell* base = SORCER_dsBase(ctx);
    for (u32 i = 0; i < SORCER_dsSize(ctx); ++i)
    {
        //SORCER_Cell v = base[i];
        fprintf(f, "\n");
    }
}
























































































