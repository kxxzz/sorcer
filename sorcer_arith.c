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




static SORCER_Type s_typeNum = { (u32)-1 };



static void SORCER_oprFunc_Neg(SORCER_Context* ctx, const SORCER_Cell* ins, SORCER_Cell* outs)
{
    APNUM_pool_t pool = SORCER_pool(ctx, s_typeNum);
    outs[0].as.ptr = APNUM_ratNew(pool);
    APNUM_ratDup(outs[0].as.ptr, ins[0].as.ptr);
    APNUM_ratNeg(outs[0].as.ptr);
}

static void SORCER_oprFunc_Add(SORCER_Context* ctx, const SORCER_Cell* ins, SORCER_Cell* outs)
{
    APNUM_pool_t pool = SORCER_pool(ctx, s_typeNum);
    outs[0].as.ptr = APNUM_ratNew(pool);
    APNUM_ratAdd(pool, outs[0].as.ptr, ins[0].as.ptr, ins[1].as.ptr);
}

static void SORCER_oprFunc_Sub(SORCER_Context* ctx, const SORCER_Cell* ins, SORCER_Cell* outs)
{
    APNUM_pool_t pool = SORCER_pool(ctx, s_typeNum);
    outs[0].as.ptr = APNUM_ratNew(pool);
    APNUM_ratSub(pool, outs[0].as.ptr, ins[0].as.ptr, ins[1].as.ptr);
}

static void SORCER_oprFunc_Mul(SORCER_Context* ctx, const SORCER_Cell* ins, SORCER_Cell* outs)
{
    APNUM_pool_t pool = SORCER_pool(ctx, s_typeNum);
    outs[0].as.ptr = APNUM_ratNew(pool);
    APNUM_ratMul(pool, outs[0].as.ptr, ins[0].as.ptr, ins[1].as.ptr);
}

static void SORCER_oprFunc_Div(SORCER_Context* ctx, const SORCER_Cell* ins, SORCER_Cell* outs)
{
    APNUM_pool_t pool = SORCER_pool(ctx, s_typeNum);
    outs[0].as.ptr = APNUM_ratNew(pool);
    APNUM_ratDiv(pool, outs[0].as.ptr, ins[0].as.ptr, ins[1].as.ptr);
}






void SORCER_arith(SORCER_Context* ctx)
{
    SORCER_Type typeNum = SORCER_typeByName(ctx, "num");
    assert(SORCER_Type_Invalid.id == typeNum.id);

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
    typeNum = SORCER_typeNew(ctx, typeInfo);
    s_typeNum = typeNum;


    SORCER_OprInfo ops[] =
    {
        {
            "neg",
            1, { typeNum },
            1, { typeNum },
            SORCER_oprFunc_Neg,
        },
        {
            "+",
            2, { typeNum, typeNum },
            1, { typeNum },
            SORCER_oprFunc_Add,
        },
        {
            "-",
            2, { typeNum, typeNum },
            1, { typeNum },
            SORCER_oprFunc_Sub,
        },
        {
            "*",
            2, { typeNum, typeNum },
            1, { typeNum },
            SORCER_oprFunc_Mul,
        },
        {
            "/",
            2, { typeNum, typeNum },
            1, { typeNum },
            SORCER_oprFunc_Div,
        },
    };
    for (u32 i = 0; i < ARYLEN(ops); ++i)
    {
        SORCER_oprNew(ctx, ops + i);
    }


}


































































































