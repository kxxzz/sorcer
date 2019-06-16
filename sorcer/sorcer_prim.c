#include "sorcer_a.h"




const char** SORCER_KeyNameTable(void)
{
    static const char* a[TXN_NumEvalKeys] =
    {
        "def",
        "var",
        "if",
        "apply",
    };
    return a;
}






static bool TXN_evalBoolByStr(SORCER_Value* pVal, const char* str, u32 len, SORCER_Context* ctx)
{
    if (0 == strncmp(str, "true", len))
    {
        if (pVal) pVal->b = true;
        return true;
    }
    if (0 == strncmp(str, "false", len))
    {
        if (pVal) pVal->b = true;
        return true;
    }
    return false;
}

static bool TXN_evalNumByStr(SORCER_Value* pVal, const char* str, u32 len, SORCER_Context* ctx)
{
    u32 r = false;
    APNUM_pool_t pool = ctx->numPool;
    APNUM_rat* n = APNUM_ratNew(pool);
    r = APNUM_ratFromStrWithBaseFmt(pool, n, str);
    if (len == r)
    {
        if (pVal)
        {
            pVal->a = n;
        }
        else
        {
            APNUM_ratFree(pool, n);
        }
    }
    return len == r;
}




static bool TXN_evalStringByStr(SORCER_Value* pVal, const char* str, u32 len, SORCER_Context* ctx)
{
    if (pVal)
    {
        vec_char* s = pVal->a;
        vec_pusharr(s, str, len);
        vec_push(s, 0);
    }
    return true;
}

static void TXN_evalStringDtor(void* ptr)
{
    vec_char* s = ptr;
    vec_free(s);
}






const SORCER_AtypeInfo* SORCER_PrimTypeInfoTable(void)
{
    static const SORCER_AtypeInfo a[TXN_NumEvalPrimTypes] =
    {
        { "bool", TXN_evalBoolByStr, true },
        { "num", TXN_evalNumByStr, true },
        { "string", TXN_evalStringByStr, false, sizeof(vec_char), TXN_evalStringDtor },
    };
    return a;
}

















void TXN_evalAfunCall_Array(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx);
void TXN_evalAfunCall_AtLoad(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx);
void TXN_evalAfunCall_AtSave(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx);
void TXN_evalAfunCall_Size(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx);
void TXN_evalAfunCall_Map(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx);
void TXN_evalAfunCall_Filter(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx);
void TXN_evalAfunCall_Reduce(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx);














static void TXN_evalAfunCall_Not(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    bool a = ins[0].b;
    outs[0].b = !a;
}



static void TXN_evalAfunCall_NumAdd(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    APNUM_rat* a = ins[0].a;
    APNUM_rat* b = ins[1].a;
    APNUM_rat* c = APNUM_ratNew(ctx->numPool);
    APNUM_ratAdd(ctx->numPool, c, a, b);
    outs[0].a = c;
}

static void TXN_evalAfunCall_NumSub(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    APNUM_rat* a = ins[0].a;
    APNUM_rat* b = ins[1].a;
    APNUM_rat* c = APNUM_ratNew(ctx->numPool);
    APNUM_ratSub(ctx->numPool, c, a, b);
    outs[0].a = c;
}

static void TXN_evalAfunCall_NumMul(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    APNUM_rat* a = ins[0].a;
    APNUM_rat* b = ins[1].a;
    APNUM_rat* c = APNUM_ratNew(ctx->numPool);
    APNUM_ratMul(ctx->numPool, c, a, b);
    outs[0].a = c;
}

static void TXN_evalAfunCall_NumDiv(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    APNUM_rat* a = ins[0].a;
    APNUM_rat* b = ins[1].a;
    APNUM_rat* c = APNUM_ratNew(ctx->numPool);
    APNUM_ratDiv(ctx->numPool, c, a, b);
    outs[0].a = c;
}



static void TXN_evalAfunCall_NumNeg(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    APNUM_rat* a = ins[0].a;
    APNUM_rat* b = APNUM_ratNew(ctx->numPool);
    APNUM_ratDup(b, a);
    APNUM_ratNeg(b);
    outs[0].a = b;
}




static void TXN_evalAfunCall_NumEQ(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    APNUM_rat* a = ins[0].a;
    APNUM_rat* b = ins[1].a;
    outs[0].b = APNUM_ratEq(a, b);
}

static void TXN_evalAfunCall_NumINEQ(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    APNUM_rat* a = ins[0].a;
    APNUM_rat* b = ins[1].a;
    outs[0].b = !APNUM_ratEq(a, b);
}

static void TXN_evalAfunCall_NumGT(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    APNUM_rat* a = ins[0].a;
    APNUM_rat* b = ins[1].a;
    outs[0].b = APNUM_ratCmp(ctx->numPool, a, b) > 0;
}

static void TXN_evalAfunCall_NumLT(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    APNUM_rat* a = ins[0].a;
    APNUM_rat* b = ins[1].a;
    outs[0].b = APNUM_ratCmp(ctx->numPool, a, b) < 0;
}

static void TXN_evalAfunCall_NumGE(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    APNUM_rat* a = ins[0].a;
    APNUM_rat* b = ins[1].a;
    outs[0].b = APNUM_ratCmp(ctx->numPool, a, b) >= 0;
}

static void TXN_evalAfunCall_NumLE(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    APNUM_rat* a = ins[0].a;
    APNUM_rat* b = ins[1].a;
    outs[0].b = APNUM_ratCmp(ctx->numPool, a, b) <= 0;
}



























const SORCER_AfunInfo* SORCER_PrimFunInfoTable(void)
{
    static const SORCER_AfunInfo a[TXN_NumEvalPrimFuns] =
    {
        {
            "&",
            "num -> [num]",
            TXN_evalAfunCall_Array,
            SORCER_AfunMode_ContextDepend,
        },
        {
            "&>",
            "{A*} [A*] num -> A*",
            TXN_evalAfunCall_AtLoad,
            SORCER_AfunMode_ContextDepend,
        },
        {
            "&<",
            "{A*} [A*] num A* ->",
            TXN_evalAfunCall_AtSave,
            SORCER_AfunMode_ContextDepend,
        },
        {
            "size",
            "{A*} [A*] -> num",
            TXN_evalAfunCall_Size,
            SORCER_AfunMode_ContextDepend,
        },

        {
            "map",
            "{A* B*} [A*] (A* -> B*) -> [B*]",
            TXN_evalAfunCall_Map,
            SORCER_AfunMode_HighOrder,
        },
        {
            "filter",
            "{A*} [A*] (A* -> bool) -> [A*]",
            TXN_evalAfunCall_Filter,
            SORCER_AfunMode_HighOrder,
        },
        {
            "reduce",
            "{A*} [A*] (A* A* -> A*) -> A*",
            TXN_evalAfunCall_Reduce,
            SORCER_AfunMode_HighOrder,
        },

        {
            "not",
            "bool -> bool",
            TXN_evalAfunCall_Not,
        },

        {
            "+",
            "num num -> num",
            TXN_evalAfunCall_NumAdd,
            SORCER_AfunMode_ContextDepend
        },
        {
            "-",
            "num num -> num",
            TXN_evalAfunCall_NumSub,
            SORCER_AfunMode_ContextDepend
        },
        {
            "*",
            "num num -> num",
            TXN_evalAfunCall_NumMul,
            SORCER_AfunMode_ContextDepend
        },
        {
            "/",
            "num num -> num",
            TXN_evalAfunCall_NumDiv,
            SORCER_AfunMode_ContextDepend
        },

        {
            "neg",
            "num -> num",
            TXN_evalAfunCall_NumNeg,
            SORCER_AfunMode_ContextDepend
        },

        {
            "eq",
            "num num -> bool",
            TXN_evalAfunCall_NumEQ,
            SORCER_AfunMode_ContextDepend
        },

        {
            "gt",
            "num num -> bool",
            TXN_evalAfunCall_NumGT,
            SORCER_AfunMode_ContextDepend
        },
        {
            "lt",
            "num num -> bool",
            TXN_evalAfunCall_NumLT,
            SORCER_AfunMode_ContextDepend
        },
        {
            "ge",
            "num num -> bool",
            TXN_evalAfunCall_NumGE,
            SORCER_AfunMode_ContextDepend
        },
        {
            "le",
            "num num -> bool",
            TXN_evalAfunCall_NumLE,
            SORCER_AfunMode_ContextDepend
        },
    };
    return a;
}



















































































































