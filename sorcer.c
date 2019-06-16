#include "sorcer_a.h"




typedef enum SORCER_CellType
{
    SORCER_CellType_Atom,
    SORCER_CellType_Func,

    SORCER_NumCellTypes
} SORCER_CellType;




typedef struct SORCER_Context
{
    int x;
} SORCER_Context;




SORCER_Context* SORCER_ctxNew(void)
{
    SORCER_Context* ctx = zalloc(sizeof(SORCER_Context));
    return ctx;
}


void SORCER_ctxFree(SORCER_Context* ctx)
{
    free(ctx);
}






































































































