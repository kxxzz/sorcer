#include "sorcer_a.h"
#include "sorcer_arith.h"
#include <apnum.h>






static bool SR_cellCtor_Num(void* pool, const char* str, SR_CellValue* pOut)
{
    APNUM_rat* a = APNUM_ratNew(pool);
    size_t n = APNUM_ratFromStrWithBaseFmt(pool, a, str);
    if (strlen(str) != n)
    {
        APNUM_ratFree(pool, a);
        return false;
    }
    pOut->ptr = a;
    return true;
}

static void SR_cellDtor_Num(void* pool, SR_CellValue* x)
{
    APNUM_ratFree(pool, x->ptr);
}

static void SR_cellCopier_Num(void* pool, const SR_CellValue* x, SR_CellValue* pOut)
{
    pOut->ptr = APNUM_ratNew(pool);
    APNUM_ratDup(pOut->ptr, x->ptr);
}

static void* SR_poolCtor_Num(void)
{
    APNUM_pool_t pool = APNUM_poolNew();
    return pool;
}

static void SR_poolDtor_Num(void* pool)
{
    APNUM_poolFree(pool);
}

static u32 SR_cellToStr_Num(char* buf, u32 bufSize, void* pool, const SR_CellValue* x)
{
    return APNUM_ratToStrWithBaseFmt(pool, x->ptr, APNUM_int_StrBaseFmtType_DEC, buf, bufSize);
}

static u32 SR_cellDumper_Num(const SR_CellValue* x, char* buf, u32 bufSize)
{
    return -1;
}

static void SR_cellLoader_Num(SR_CellValue* x, const char* ptr, u32 size)
{

}




SR_Type SR_ArithTypeTable(SR_ArithContext* ctx, SR_ArithType at)
{
    assert(at < SR_NumArithTypes);
    return ctx->type[at];
}


SR_Opr SR_ArithOprTable(SR_ArithContext* ctx, SR_ArithOpr aop)
{
    assert(aop < SR_NumArithOprs);
    return ctx->opr[aop];
}







static void SR_oprFunc_Neg(SR_Context* ctx, void* funcCtx, const SR_Cell* ins, SR_Cell* outs)
{
    APNUM_pool_t pool = SR_pool(ctx, SR_ArithTypeTable(funcCtx, SR_ArithType_NUM));
    outs[0].value->ptr = ins[0].value->ptr;
    APNUM_ratNeg(outs[0].value->ptr);
}

static void SR_oprFunc_Add(SR_Context* ctx, void* funcCtx, const SR_Cell* ins, SR_Cell* outs)
{
    APNUM_pool_t pool = SR_pool(ctx, SR_ArithTypeTable(funcCtx, SR_ArithType_NUM));
    outs[0].value->ptr = APNUM_ratNew(pool);
    APNUM_ratAdd(pool, outs[0].value->ptr, ins[0].value->ptr, ins[1].value->ptr);
    APNUM_ratFree(pool, ins[0].value->ptr);
    APNUM_ratFree(pool, ins[1].value->ptr);
}

static void SR_oprFunc_Sub(SR_Context* ctx, void* funcCtx, const SR_Cell* ins, SR_Cell* outs)
{
    APNUM_pool_t pool = SR_pool(ctx, SR_ArithTypeTable(funcCtx, SR_ArithType_NUM));
    outs[0].value->ptr = APNUM_ratNew(pool);
    APNUM_ratSub(pool, outs[0].value->ptr, ins[0].value->ptr, ins[1].value->ptr);
    APNUM_ratFree(pool, ins[0].value->ptr);
    APNUM_ratFree(pool, ins[1].value->ptr);
}

static void SR_oprFunc_Mul(SR_Context* ctx, void* funcCtx, const SR_Cell* ins, SR_Cell* outs)
{
    APNUM_pool_t pool = SR_pool(ctx, SR_ArithTypeTable(funcCtx, SR_ArithType_NUM));
    outs[0].value->ptr = APNUM_ratNew(pool);
    APNUM_ratMul(pool, outs[0].value->ptr, ins[0].value->ptr, ins[1].value->ptr);
    APNUM_ratFree(pool, ins[0].value->ptr);
    APNUM_ratFree(pool, ins[1].value->ptr);
}

static void SR_oprFunc_Div(SR_Context* ctx, void* funcCtx, const SR_Cell* ins, SR_Cell* outs)
{
    APNUM_pool_t pool = SR_pool(ctx, SR_ArithTypeTable(funcCtx, SR_ArithType_NUM));
    outs[0].value->ptr = APNUM_ratNew(pool);
    APNUM_ratDiv(pool, outs[0].value->ptr, ins[0].value->ptr, ins[1].value->ptr);
    APNUM_ratFree(pool, ins[0].value->ptr);
    APNUM_ratFree(pool, ins[1].value->ptr);
}






void SR_arith(SR_Context* ctx, SR_ArithContext* arithCtx)
{
    SR_Type typeNum = SR_typeByName(ctx, "num");
    assert(SR_Type_Invalid.id == typeNum.id);

    SR_TypeInfo typeInfo[SR_NumArithTypes] =
    {
        {
            "num",
            SR_cellCtor_Num,
            SR_cellDtor_Num,
            SR_cellCopier_Num,
            SR_poolCtor_Num,
            SR_poolDtor_Num,
            SR_cellToStr_Num,
            SR_cellDumper_Num,
            SR_cellLoader_Num,
        }
    };
    for (u32 i = 0; i < ARYLEN(typeInfo); ++i)
    {
        arithCtx->type[i] = SR_typeNew(ctx, typeInfo + i);
    }
    typeNum = SR_ArithTypeTable(arithCtx, SR_ArithType_NUM);


    SR_OprInfo ops[] =
    {
        {
            "neg",
            1, { typeNum },
            1, { typeNum },
            SR_oprFunc_Neg,
            arithCtx,
        },
        {
            "+",
            2, { typeNum, typeNum },
            1, { typeNum },
            SR_oprFunc_Add,
            arithCtx,
        },
        {
            "-",
            2, { typeNum, typeNum },
            1, { typeNum },
            SR_oprFunc_Sub,
            arithCtx,
        },
        {
            "*",
            2, { typeNum, typeNum },
            1, { typeNum },
            SR_oprFunc_Mul,
            arithCtx,
        },
        {
            "/",
            2, { typeNum, typeNum },
            1, { typeNum },
            SR_oprFunc_Div,
            arithCtx,
        },
    };
    for (u32 i = 0; i < ARYLEN(ops); ++i)
    {
        arithCtx->opr[i] = SR_oprNew(ctx, ops + i);
    }

}


































































































