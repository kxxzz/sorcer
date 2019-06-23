#include "sorcer_a.h"
#include <apnum.h>






static bool SORCER_cellCtor_Num(void* pool, const char* str, SORCER_Cell* out)
{
    APNUM_rat* a = APNUM_ratNew(pool);
    size_t n = APNUM_ratFromStrWithBaseFmt(pool, a, str);
    if (strlen(str) != n)
    {
        APNUM_ratFree(pool, a);
        return false;
    }
    out->as.ptr = a;
    return true;
}

static void SORCER_cellDtor_Num(void* pool, void* ptr)
{
    APNUM_ratFree(pool, ptr);
}

static void* SORCER_poolCtor_Num(void)
{
    APNUM_pool_t pool = APNUM_poolNew();
    return pool;
}

static void SORCER_poolDtor_Num(void* pool)
{
    APNUM_poolFree(pool);
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


































































































