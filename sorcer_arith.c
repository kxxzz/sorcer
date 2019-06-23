#include "sorcer_a.h"
#include <apnum.h>






static bool SORCER_cellCtor_Num(void* pool, const char* str, SORCER_Cell* out)
{
    return false;
}

static void SORCER_cellDtor_Num(void* pool, void* ptr)
{

}

static void* SORCER_poolCtor_Num(void)
{
    return NULL;
}

static void SORCER_poolDtor_Num(void* pool)
{

}



void SORCER_arith(SORCER_Context* ctx)
{
    SORCER_TypeInfo typeInfo[1] =
    {
        {
            "num",
            SORCER_cellCtor_Num,
            SORCER_cellDtor_Num,
            SORCER_poolCtor_Num,
            SORCER_poolDtor_Num,
        }
    };
    //SORCER_Type typeNum = SORCER_typeNew(ctx, typeInfo);

}


































































































