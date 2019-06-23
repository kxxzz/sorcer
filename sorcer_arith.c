#include "sorcer_a.h"
#include "sorcer_arith.h"
#include <apnum.h>






static bool SORCER_cellCtor_Num(void* pool, const char* str, void** pOut)
{
    APNUM_rat* a = APNUM_ratNew(pool);
    size_t n = APNUM_ratFromStrWithBaseFmt(pool, a, str);
    if (strlen(str) != n)
    {
        APNUM_ratFree(pool, a);
        return false;
    }
    *pOut = a;
    return true;
}

static void SORCER_cellDtor_Num(void* pool, void* x)
{
    APNUM_ratFree(pool, x);
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

static u32 SORCER_cellToStr_Num(char* buf, u32 bufSize, void* pool, const SORCER_Cell* x)
{
    return APNUM_ratToStrWithBaseFmt(pool, x->as.ptr, APNUM_int_StrBaseFmtType_DEC, buf, bufSize);
}





SORCER_Type SORCER_ArithTypeTable(SORCER_ArithContext* ctx, SORCER_ArithType at)
{
    assert(at < SORCER_NumArithTypes);
    return ctx->type[at];
}


SORCER_Opr SORCER_ArithOprTable(SORCER_ArithContext* ctx, SORCER_ArithOpr aop)
{
    assert(aop < SORCER_NumArithOprs);
    return ctx->opr[aop];
}







static void SORCER_oprFunc_Neg(SORCER_Context* ctx, void* funcCtx, const SORCER_Cell* ins, SORCER_Cell* outs)
{
    APNUM_pool_t pool = SORCER_pool(ctx, SORCER_ArithTypeTable(funcCtx, SORCER_ArithType_NUM));
    outs[0].as.ptr = APNUM_ratNew(pool);
    APNUM_ratDup(outs[0].as.ptr, ins[0].as.ptr);
    APNUM_ratNeg(outs[0].as.ptr);
}

static void SORCER_oprFunc_Add(SORCER_Context* ctx, void* funcCtx, const SORCER_Cell* ins, SORCER_Cell* outs)
{
    APNUM_pool_t pool = SORCER_pool(ctx, SORCER_ArithTypeTable(funcCtx, SORCER_ArithType_NUM));
    outs[0].as.ptr = APNUM_ratNew(pool);
    APNUM_ratAdd(pool, outs[0].as.ptr, ins[0].as.ptr, ins[1].as.ptr);
}

static void SORCER_oprFunc_Sub(SORCER_Context* ctx, void* funcCtx, const SORCER_Cell* ins, SORCER_Cell* outs)
{
    APNUM_pool_t pool = SORCER_pool(ctx, SORCER_ArithTypeTable(funcCtx, SORCER_ArithType_NUM));
    outs[0].as.ptr = APNUM_ratNew(pool);
    APNUM_ratSub(pool, outs[0].as.ptr, ins[0].as.ptr, ins[1].as.ptr);
}

static void SORCER_oprFunc_Mul(SORCER_Context* ctx, void* funcCtx, const SORCER_Cell* ins, SORCER_Cell* outs)
{
    APNUM_pool_t pool = SORCER_pool(ctx, SORCER_ArithTypeTable(funcCtx, SORCER_ArithType_NUM));
    outs[0].as.ptr = APNUM_ratNew(pool);
    APNUM_ratMul(pool, outs[0].as.ptr, ins[0].as.ptr, ins[1].as.ptr);
}

static void SORCER_oprFunc_Div(SORCER_Context* ctx, void* funcCtx, const SORCER_Cell* ins, SORCER_Cell* outs)
{
    APNUM_pool_t pool = SORCER_pool(ctx, SORCER_ArithTypeTable(funcCtx, SORCER_ArithType_NUM));
    outs[0].as.ptr = APNUM_ratNew(pool);
    APNUM_ratDiv(pool, outs[0].as.ptr, ins[0].as.ptr, ins[1].as.ptr);
}






void SORCER_arith(SORCER_Context* ctx, SORCER_ArithContext* arithCtx)
{
    SORCER_Type typeNum = SORCER_typeByName(ctx, "num");
    assert(SORCER_Type_Invalid.id == typeNum.id);

    SORCER_TypeInfo typeInfo[SORCER_NumArithTypes] =
    {
        {
            "num",
            SORCER_cellCtor_Num,
            SORCER_cellDtor_Num,
            SORCER_poolCtor_Num,
            SORCER_poolDtor_Num,
            SORCER_cellToStr_Num,
        }
    };
    for (u32 i = 0; i < ARYLEN(typeInfo); ++i)
    {
        arithCtx->type[i] = SORCER_typeNew(ctx, typeInfo + i);
    }
    typeNum = SORCER_ArithTypeTable(arithCtx, SORCER_ArithType_NUM);


    SORCER_OprInfo ops[] =
    {
        {
            "neg",
            1, { typeNum },
            1, { typeNum },
            SORCER_oprFunc_Neg,
            arithCtx,
        },
        {
            "+",
            2, { typeNum, typeNum },
            1, { typeNum },
            SORCER_oprFunc_Add,
            arithCtx,
        },
        {
            "-",
            2, { typeNum, typeNum },
            1, { typeNum },
            SORCER_oprFunc_Sub,
            arithCtx,
        },
        {
            "*",
            2, { typeNum, typeNum },
            1, { typeNum },
            SORCER_oprFunc_Mul,
            arithCtx,
        },
        {
            "/",
            2, { typeNum, typeNum },
            1, { typeNum },
            SORCER_oprFunc_Div,
            arithCtx,
        },
    };
    for (u32 i = 0; i < ARYLEN(ops); ++i)
    {
        arithCtx->opr[i] = SORCER_oprNew(ctx, ops + i);
    }

}


































































































