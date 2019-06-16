#include "sorcer_a.h"
#include "sorcer_utils.h"
#include <time.h>





typedef struct SORCER_ValueFprintArrayLevel
{
    SORCER_Value v;
    u32 arySize;
    u32 elmSize;
    const u32* elmTypes;
    bool hasAryMemb;
    bool multiLine;
    u32 indent;
    u32 elmBufBase;

    u32 aryIdx;
    u32 elmIdx;
} SORCER_ValueFprintArrayLevel;

typedef vec_t(SORCER_ValueFprintArrayLevel) SORCER_ValueFprintStack;




typedef struct SORCER_ValueFprintContext
{
    FILE* f;
    SORCER_Context* evalContext;
    SORCER_ValueVec elmBuf;
    SORCER_ValueFprintStack stack;
} SORCER_ValueFprintContext;

static SORCER_ValueFprintContext TXN_newEvalValueFprintContext(FILE* f, SORCER_Context* evalContext)
{
    SORCER_ValueFprintContext a = { f, evalContext };
    return a;
}
static void TXN_evalValueFprintContextFree(SORCER_ValueFprintContext* ctx)
{
    vec_free(&ctx->stack);
    vec_free(&ctx->elmBuf);
}





static bool TXN_evalValueFprintNonArray(SORCER_ValueFprintContext* ctx, SORCER_Value v, u32 t)
{
    FILE* f = ctx->f;
    SORCER_Context* evalContext = ctx->evalContext;
    SORCER_TypeContext* typeContext = TXN_evalDataTypeContext(evalContext);
    const SORCER_TypeDesc* desc = TXN_evalTypeDescById(typeContext, t);
    switch (desc->type)
    {
    case SORCER_TypeType_Atom:
    {
        switch (desc->atype)
        {
        case SORCER_PrimType_BOOL:
        {
            fprintf(f, "%s", v.b ? "true" : "false");
            break;
        }
        case SORCER_PrimType_NUM:
        {
            APNUM_rat* a = v.a;
            char buf[1024];
            u32 n = APNUM_ratToStrWithBaseFmt(evalContext->numPool, a, APNUM_int_StrBaseFmtType_DEC, buf, sizeof(buf));
            buf[n] = 0;
            fprintf(f, "%s", buf);
            break;
        }
        case SORCER_PrimType_STRING:
        {
            const char* s = ((vec_char*)(v.a))->data;
            fprintf(f, "\"%s\"", s);
            break;
        }
        default:
        {
            const char* s = SORCER_PrimTypeInfoTable()[t].name;
            fprintf(f, "<TYPE: %s>", s);
            break;
        }
        }
        break;
    }
    case SORCER_TypeType_Array:
    {
        return false;
    }
    default:
        assert(false);
        break;
    }
    return true;
}






static void TXN_evalValueFprintPushArrayLevel
(
    SORCER_ValueFprintContext* ctx,
    SORCER_Value v, u32 arySize, u32 elmSize, const u32* elmTypes,
    bool hasAryMemb, u32 indent
)
{
    FILE* f = ctx->f;
    SORCER_Context* evalContext = ctx->evalContext;
    SORCER_TypeContext* typeContext = TXN_evalDataTypeContext(evalContext);
    SORCER_ValueVec* elmBuf = &ctx->elmBuf;
    SORCER_ValueFprintStack* stack = &ctx->stack;

    u32 elmBufBase = elmBuf->length;
    vec_resize(elmBuf, elmBufBase + elmSize);

    bool multiLine = hasAryMemb || (elmSize > 1);
    if (multiLine)
    {
        fprintf(f, "[\n");
    }
    else
    {
        fprintf(f, "[ ");
    }
    SORCER_ValueFprintArrayLevel newLevel =
    {
        v, arySize, elmSize, elmTypes, hasAryMemb, multiLine, indent, elmBufBase
    };
    vec_push(stack, newLevel);
}





static void TXN_evalValueFprintPushArrayLevelByVT(SORCER_ValueFprintContext* ctx, SORCER_Value v, u32 t, u32 indent)
{
    SORCER_Context* evalContext = ctx->evalContext;
    SORCER_TypeContext* typeContext = TXN_evalDataTypeContext(evalContext);

    const SORCER_TypeDesc* desc = TXN_evalTypeDescById(typeContext, t);
    assert(SORCER_TypeType_Array == desc->type);

    assert(SORCER_ValueType_Object == v.type);
    const SORCER_TypeDesc* elmDesc = TXN_evalTypeDescById(typeContext, desc->ary.elm);
    assert(SORCER_TypeType_List == elmDesc->type);

    bool hasAryMemb = false;
    for (u32 i = 0; i < elmDesc->list.count; ++i)
    {
        u32 t = elmDesc->list.elms[i];
        const SORCER_TypeDesc* membDesc = TXN_evalTypeDescById(typeContext, t);
        if (SORCER_TypeType_Array == membDesc->type)
        {
            hasAryMemb = true;
        }
    }

    SORCER_Array* ary = v.ary;
    u32 arySize = TXN_evalArraySize(ary);
    u32 elmSize = TXN_evalArrayElmSize(ary);
    assert(elmDesc->list.count == elmSize);
    const u32* elmTypes = elmDesc->list.elms;

    TXN_evalValueFprintPushArrayLevel(ctx, v, arySize, elmSize, elmTypes, hasAryMemb, indent);
}





static void TXN_evalValueFprint(SORCER_ValueFprintContext* ctx, SORCER_Value v, u32 t, u32 indent)
{
    FILE* f = ctx->f;
    SORCER_Context* evalContext = ctx->evalContext;
    SORCER_TypeContext* typeContext = TXN_evalDataTypeContext(evalContext);
    SORCER_ValueVec* elmBuf = &ctx->elmBuf;
    SORCER_ValueFprintStack* stack = &ctx->stack;

    for (u32 i = 0; i < indent; ++i)
    {
        fprintf(f, "  ");
    }
    if (!TXN_evalValueFprintNonArray(ctx, v, t))
    {
        TXN_evalValueFprintPushArrayLevelByVT(ctx, v, t, indent);
    }

    SORCER_ValueFprintArrayLevel* top = NULL;
    u32 aryIdx = 0;
    u32 elmIdx = 0;

next:
    if (!stack->length)
    {
        return;
    }
    top = &vec_last(stack);
    aryIdx = top->aryIdx;
    elmIdx = top->elmIdx++;

    if (aryIdx == top->arySize)
    {
        vec_resize(elmBuf, top->elmBufBase);
        fprintf(f, "]");
        vec_pop(stack);
        goto next;
    }
    if (0 == elmIdx)
    {
        bool r = TXN_evalArrayGetElm(top->v.ary, aryIdx, elmBuf->data + top->elmBufBase);
        assert(r);
    }
    else
    {
        if (top->multiLine && ((elmIdx == top->elmSize) || top->hasAryMemb))
        {
            fprintf(f, "\n");
        }
        else
        {
            fprintf(f, " ");
        }
        if (elmIdx == top->elmSize)
        {
            if (top->hasAryMemb)
            {
                fprintf(f, "\n");
            }

            ++top->aryIdx;
            top->elmIdx = 0;
            goto next;
        }
    }
    v = elmBuf->data[top->elmBufBase + elmIdx];
    t = top->elmTypes[elmIdx];
    indent = (top->multiLine && ((0 == elmIdx) || top->hasAryMemb)) ? top->indent + 1 : 0;
    for (u32 i = 0; i < indent; ++i)
    {
        fprintf(f, "  ");
    }
    if (!TXN_evalValueFprintNonArray(ctx, v, t))
    {
        TXN_evalValueFprintPushArrayLevelByVT(ctx, v, t, indent);
    }
    goto next;
}







void TXN_evalDataStackFprint(FILE* f, SORCER_Context* evalContext)
{
    vec_u32* typeStack = TXN_evalDataTypeStack(evalContext);
    SORCER_ValueVec* dataStack = TXN_evalDataStack(evalContext);
    SORCER_ValueFprintContext ctx = TXN_newEvalValueFprintContext(f, evalContext);
    for (u32 i = 0; i < dataStack->length; ++i)
    {
        SORCER_Value v = dataStack->data[i];
        u32 t = typeStack->data[i];
        TXN_evalValueFprint(&ctx, v, t, 0);
        fprintf(f, "\n");
    }
    TXN_evalValueFprintContextFree(&ctx);
}








char* TXN_evalGetNowStr(char* timeBuf)
{
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    timeBuf[strftime(timeBuf, SORCER_TimeStrBuf_MAX, "%H:%M:%S", lt)] = '\0';
    return timeBuf;
}







void TXN_evalErrorFprint(FILE* f, const SORCER_FileInfoTable* fiTable, const SORCER_Error* err)
{
    const char* name = SORCER_ErrCodeNameTable()[err->code];
    const char* filename = fiTable->data[err->file].name;
    char timeBuf[SORCER_TimeStrBuf_MAX];
    fprintf(f, "[ERROR] %s: \"%s\": %u:%u [%s]\n", name, filename, err->line, err->column, TXN_evalGetNowStr(timeBuf));
}





































































