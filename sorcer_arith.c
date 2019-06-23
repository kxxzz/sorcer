#include "sorcer_a.h"
#include "sorcer_arith.h"
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



static bool s_ArithTypeTableInited = false;
static SORCER_Type s_ArithTypeTable[SORCER_NumArithTypes] = { 0 };

SORCER_Type SORCER_ArithTypeTable(SORCER_ArithType at)
{
    assert(s_ArithTypeTableInited);
    assert(at < SORCER_NumArithTypes);
    return s_ArithTypeTable[at];
}


static s_ArithOprTableInited = false;
static SORCER_Opr s_ArithOprTable[SORCER_NumArithOprs] = { 0 };

SORCER_Opr SORCER_ArithOprTable(SORCER_ArithOpr aop)
{
    assert(s_ArithOprTableInited);
    assert(aop < SORCER_NumArithOprs);
    return s_ArithOprTable[aop];
}




static void SORCER_oprFunc_Neg(SORCER_Context* ctx, const SORCER_Cell* ins, SORCER_Cell* outs)
{
    APNUM_pool_t pool = SORCER_pool(ctx, SORCER_ArithTypeTable(SORCER_ArithType_NUM));
    outs[0].as.ptr = APNUM_ratNew(pool);
    APNUM_ratDup(outs[0].as.ptr, ins[0].as.ptr);
    APNUM_ratNeg(outs[0].as.ptr);
}

static void SORCER_oprFunc_Add(SORCER_Context* ctx, const SORCER_Cell* ins, SORCER_Cell* outs)
{
    APNUM_pool_t pool = SORCER_pool(ctx, SORCER_ArithTypeTable(SORCER_ArithType_NUM));
    outs[0].as.ptr = APNUM_ratNew(pool);
    APNUM_ratAdd(pool, outs[0].as.ptr, ins[0].as.ptr, ins[1].as.ptr);
}

static void SORCER_oprFunc_Sub(SORCER_Context* ctx, const SORCER_Cell* ins, SORCER_Cell* outs)
{
    APNUM_pool_t pool = SORCER_pool(ctx, SORCER_ArithTypeTable(SORCER_ArithType_NUM));
    outs[0].as.ptr = APNUM_ratNew(pool);
    APNUM_ratSub(pool, outs[0].as.ptr, ins[0].as.ptr, ins[1].as.ptr);
}

static void SORCER_oprFunc_Mul(SORCER_Context* ctx, const SORCER_Cell* ins, SORCER_Cell* outs)
{
    APNUM_pool_t pool = SORCER_pool(ctx, SORCER_ArithTypeTable(SORCER_ArithType_NUM));
    outs[0].as.ptr = APNUM_ratNew(pool);
    APNUM_ratMul(pool, outs[0].as.ptr, ins[0].as.ptr, ins[1].as.ptr);
}

static void SORCER_oprFunc_Div(SORCER_Context* ctx, const SORCER_Cell* ins, SORCER_Cell* outs)
{
    APNUM_pool_t pool = SORCER_pool(ctx, SORCER_ArithTypeTable(SORCER_ArithType_NUM));
    outs[0].as.ptr = APNUM_ratNew(pool);
    APNUM_ratDiv(pool, outs[0].as.ptr, ins[0].as.ptr, ins[1].as.ptr);
}






void SORCER_arith(SORCER_Context* ctx)
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
        }
    };
    for (u32 i = 0; i < ARYLEN(typeInfo); ++i)
    {
        s_ArithTypeTable[i] = SORCER_typeNew(ctx, typeInfo + i);
    }
    s_ArithTypeTableInited = true;
    typeNum = SORCER_ArithTypeTable(SORCER_ArithType_NUM);


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
        s_ArithOprTable[i] = SORCER_oprNew(ctx, ops + i);
    }
    s_ArithOprTableInited = true;

}


































































































