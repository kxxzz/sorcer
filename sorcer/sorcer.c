/*
#include "sorcer_a.h"





SORCER_Context* SORCER_ctxNew(const SORCER_AtomTable* addAtomTable)
{
    SORCER_Context* ctx = zalloc(sizeof(*ctx));
    TXN_Space* space = ctx->space = TXN_spaceNew();
    SORCER_AtypeInfoVec* atypeTable = &ctx->atypeTable;
    SORCER_AfunInfoVec* afunTable = &ctx->afunTable;
    vec_u32* afunTypeTable = &ctx->afunTypeTable;
    SORCER_TypeContext* typeContext = ctx->typeContext = TXN_newEvalTypeContext();
    SORCER_ObjectTable* objectTable = &ctx->objectTable;
    SORCER_TypeDeclContext* typeDeclContext =  ctx->typeDeclContext = TXN_newEvalTypeDeclContext(space, atypeTable, typeContext);

    for (u32 i = 0; i < TXN_NumEvalPrimTypes; ++i)
    {
        vec_push(atypeTable, SORCER_PrimTypeInfoTable()[i]);
    }
    for (u32 i = 0; i < TXN_NumEvalPrimFuns; ++i)
    {
        const SORCER_AfunInfo* info = SORCER_PrimFunInfoTable() + i;
        vec_push(afunTable, *info);
        TXN_Node node = TXN_parseAsList(space, info->typeDecl, NULL);
        assert(node.id != TXN_Node_Invalid.id);
        u32 t = TXN_evalTypeDecl(typeDeclContext, node, NULL, NULL);
        assert(t != -1);
        vec_push(afunTypeTable, t);
    }
    if (addAtomTable)
    {
        for (u32 i = 0; i < addAtomTable->numTypes; ++i)
        {
            vec_push(atypeTable, addAtomTable->types[i]);
        }
        for (u32 i = 0; i < addAtomTable->numFuns; ++i)
        {
            const SORCER_AfunInfo* info = addAtomTable->funs + i;
            vec_push(afunTable, *info);
            TXN_Node node = TXN_parseAsList(space, info->typeDecl, NULL);
            assert(node.id != TXN_Node_Invalid.id);
            u32 t = TXN_evalTypeDecl(typeDeclContext, node, NULL, NULL);
            assert(t != -1);
            vec_push(afunTypeTable, t);
        }
    }

    ctx->objectTable = TXN_newEvalObjectTable(atypeTable);
    ctx->numPool = APNUM_poolNew();
    return ctx;
}




void SORCER_ctxFree(SORCER_Context* ctx)
{
    APNUM_poolFree(ctx->numPool);
    vec_free(&ctx->fileInfoTable);

    vec_free(&ctx->aryStack);
    TXN_evalTypeDeclContextFree(ctx->typeDeclContext);

    vec_free(&ctx->dataStack);
    vec_free(&ctx->varStack);
    vec_free(&ctx->callStack);
    vec_free(&ctx->typeStack);

    TXN_evalObjectTableFree(&ctx->objectTable, &ctx->atypeTable);
    vec_free(&ctx->nodeTable);

    vec_free(&ctx->afunTypeTable);
    vec_free(&ctx->afunTable);
    vec_free(&ctx->atypeTable);

    TXN_evalTypeContextFree(ctx->typeContext);
    TXN_spaceSrcInfoFree(&ctx->srcInfo);
    TXN_spaceFree(ctx->space);
    free(ctx);
}








SORCER_Error SORCER_ctxLastError(SORCER_Context* ctx)
{
    return ctx->error;
}

SORCER_TypeContext* TXN_evalDataTypeContext(SORCER_Context* ctx)
{
    return ctx->typeContext;
}

vec_u32* TXN_evalDataTypeStack(SORCER_Context* ctx)
{
    return &ctx->typeStack;
}

SORCER_ValueVec* SORCER_ctxDataStack(SORCER_Context* ctx)
{
    return &ctx->dataStack;
}




















void TXN_evalPushValue(SORCER_Context* ctx, u32 type, SORCER_Value val)
{
    vec_push(&ctx->typeStack, type);
    vec_push(&ctx->dataStack, val);
}

void TXN_evalDrop(SORCER_Context* ctx)
{
    assert(ctx->typeStack.length > 0);
    assert(ctx->dataStack.length > 0);
    assert(ctx->typeStack.length == ctx->dataStack.length);
    vec_pop(&ctx->typeStack);
    vec_pop(&ctx->dataStack);
}


















static void TXN_evalWordDefGetBody(SORCER_Context* ctx, TXN_Node node, u32* pLen, const TXN_Node** pSeq)
{
    TXN_Space* space = ctx->space;
    assert(TXN_seqLen(space, node) >= 2);
    *pLen = TXN_seqLen(space, node) - 2;
    const TXN_Node* exp = TXN_seqElm(space, node);
    *pSeq = exp + 2;
}













static SORCER_Value* TXN_evalGetVar(SORCER_Context* ctx, const SORCER_NodeVar* evar)
{
    TXN_Space* space = ctx->space;
    SORCER_NodeTable* nodeTable = &ctx->nodeTable;
    SORCER_CallStack* callStack = &ctx->callStack;
    SORCER_ValueVec* varStack = &ctx->varStack;
    for (u32 i = 0; i < callStack->length; ++i)
    {
        SORCER_Call* call = callStack->data + callStack->length - 1 - i;
        SORCER_Node* blkEnode = nodeTable->data + call->srcNode.id;
        if (evar->block.id == call->srcNode.id)
        {
            u32 offset = call->varBase + evar->id;
            return &varStack->data[offset];
        }
    }
    return NULL;
}













static void TXN_evalEnterBlock(SORCER_Context* ctx, const TXN_Node* seq, u32 len, TXN_Node srcNode)
{
    SORCER_BlockCallback nocb = { SORCER_BlockCallbackType_NONE };
    SORCER_Call call = { srcNode, seq, seq + len, ctx->varStack.length, nocb };
    vec_push(&ctx->callStack, call);
}

static void TXN_evalEnterBlockWithCB
(
    SORCER_Context* ctx, const TXN_Node* seq, u32 len, TXN_Node srcNode, SORCER_BlockCallback cb
)
{
    assert(cb.type != SORCER_BlockCallbackType_NONE);
    SORCER_Call call = { srcNode, seq, seq + len, ctx->varStack.length, cb };
    vec_push(&ctx->callStack, call);
}

static bool TXN_evalLeaveBlock(SORCER_Context* ctx)
{
    SORCER_Call* curCall = &vec_last(&ctx->callStack);
    SORCER_Node* enode = ctx->nodeTable.data + curCall->srcNode.id;
    assert(ctx->varStack.length >= enode->varsCount);
    vec_resize(&ctx->varStack, curCall->varBase);
    vec_pop(&ctx->callStack);
    return ctx->callStack.length > 0;
}





static bool TXN_evalAtTail(SORCER_Context* ctx)
{
    SORCER_Call* curCall = &vec_last(&ctx->callStack);
    if (curCall->cb.type != SORCER_BlockCallbackType_NONE)
    {
        return false;
    }
    return curCall->p == curCall->end;
}












static void TXN_evalAfunCall
(
    SORCER_Context* ctx, SORCER_AfunInfo* afunInfo, TXN_Node srcNode, SORCER_Node* srcEnode
)
{
    TXN_Space* space = ctx->space;
    SORCER_ValueVec* dataStack = &ctx->dataStack;
    SORCER_TypeContext* typeContext = ctx->typeContext;

    u32 argsOffset = dataStack->length - srcEnode->numIns;
    memset(ctx->ncallOutBuf, 0, sizeof(ctx->ncallOutBuf[0])*srcEnode->numOuts);
    afunInfo->call
    (
        space, dataStack->data + argsOffset, ctx->ncallOutBuf,
        afunInfo->mode >= SORCER_AfunMode_ContextDepend ? ctx : NULL
    );
    if (afunInfo->mode < SORCER_AfunMode_HighOrder)
    {
        vec_resize(dataStack, argsOffset);
        for (u32 i = 0; i < srcEnode->numOuts; ++i)
        {
            SORCER_Value v = ctx->ncallOutBuf[i];
            vec_push(dataStack, v);
        }
    }
}









static void TXN_evalTailCallElimination(SORCER_Context* ctx)
{
    while (TXN_evalAtTail(ctx))
    {
        if (!TXN_evalLeaveBlock(ctx))
        {
            break;
        }
    }
}












static void TXN_evalCall(SORCER_Context* ctx)
{
    TXN_Space* space = ctx->space;
    SORCER_Call* curCall;
    SORCER_NodeTable* nodeTable = &ctx->nodeTable;
    SORCER_ValueVec* dataStack = &ctx->dataStack;
    SORCER_TypeContext* typeContext = ctx->typeContext;
    SORCER_AtypeInfoVec* atypeTable = &ctx->atypeTable;
    SORCER_ArrayPtrVec* aryStack = &ctx->aryStack;
    bool r;
next:
    if (ctx->error.code)
    {
        return;
    }
    curCall = &vec_last(&ctx->callStack);
    if (curCall->p == curCall->end)
    {
        SORCER_BlockCallback* cb = &curCall->cb;
        switch (cb->type)
        {
        case SORCER_BlockCallbackType_NONE:
        {
            break;
        }
        case SORCER_BlockCallbackType_Ncall:
        {
            SORCER_AfunInfo* afunInfo = ctx->afunTable.data + cb->afun;
            SORCER_Node* srcEnode = nodeTable->data + curCall->srcNode.id;
            TXN_evalAfunCall(ctx, afunInfo, curCall->srcNode, srcEnode);
            break;
        }
        case SORCER_BlockCallbackType_CallWord:
        {
            TXN_Node blkSrc = cb->blkSrc;
            u32 bodyLen = 0;
            TXN_Node* body = NULL;
            TXN_evalWordDefGetBody(ctx, blkSrc, &bodyLen, &body);
            if (!TXN_evalLeaveBlock(ctx))
            {
                goto next;
            }
            TXN_evalTailCallElimination(ctx);
            TXN_evalEnterBlock(ctx, body, bodyLen, blkSrc);
            goto next;
        }
        case SORCER_BlockCallbackType_CallVar:
        {
            TXN_Node blkSrc = cb->blkSrc;
            u32 bodyLen = TXN_seqLen(space, blkSrc);
            const TXN_Node* body = TXN_seqElm(space, blkSrc);
            if (!TXN_evalLeaveBlock(ctx))
            {
                goto next;
            }
            TXN_evalTailCallElimination(ctx);
            TXN_evalEnterBlock(ctx, body, bodyLen, blkSrc);
            goto next;
        }
        case SORCER_BlockCallbackType_Cond:
        {
            SORCER_Value v = vec_last(dataStack);
            vec_pop(dataStack);
            if (!TXN_evalLeaveBlock(ctx))
            {
                goto next;
            }
            TXN_Node srcNode = curCall->srcNode;
            if (v.b)
            {
                TXN_evalEnterBlock(ctx, TXN_evalIfBranch0(space, srcNode), 1, srcNode);
                goto next;
            }
            TXN_evalEnterBlock(ctx, TXN_evalIfBranch1(space, srcNode), 1, srcNode);
            goto next;
        }
        case SORCER_BlockCallbackType_ArrayMap:
        {
            TXN_Node blkNode = curCall->srcNode;

            assert(aryStack->length >= 2);
            SORCER_Array* src = aryStack->data[aryStack->length - 2];
            SORCER_Array* dst = aryStack->data[aryStack->length - 1];

            u32 pos = cb->pos;
            assert(TXN_evalArraySize(dst) == TXN_evalArraySize(src));
            u32 size = TXN_evalArraySize(src);

            SORCER_Node* blkEnode = nodeTable->data + blkNode.id;
            assert(TXN_evalArrayElmSize(src) == blkEnode->numIns);
            u32 dstElmSize = blkEnode->numOuts;
            assert(dataStack->length >= dstElmSize);

            SORCER_Value* inBuf = dataStack->data + dataStack->length - dstElmSize;
            r = TXN_evalArraySetElm(dst, pos, inBuf);
            assert(r);
            vec_resize(dataStack, dataStack->length - dstElmSize);

            pos = ++cb->pos;
            if (pos < size)
            {
                u32 srcElmSize = TXN_evalArrayElmSize(src);
                SORCER_Value* outBuf = dataStack->data + dataStack->length;
                vec_resize(dataStack, dataStack->length + srcElmSize);
                r = TXN_evalArrayGetElm(src, pos, outBuf);
                assert(r);

                assert(TXN_isSeqSquare(space, blkNode));
                u32 bodyLen = TXN_seqLen(space, blkNode);
                curCall->p -= bodyLen;
            }
            else
            {
                SORCER_Value v = { .ary = dst, SORCER_ValueType_Object };
                vec_push(dataStack, v);

                vec_resize(aryStack, aryStack->length - 2);
                TXN_evalLeaveBlock(ctx);
            }
            goto next;
        }
        case SORCER_BlockCallbackType_ArrayFilter:
        {
            assert(aryStack->length >= 2);
            SORCER_Array* src = aryStack->data[aryStack->length - 2];
            SORCER_Array* dst = aryStack->data[aryStack->length - 1];

            u32 pos = cb->pos;
            u32 size = TXN_evalArraySize(src);
            assert(TXN_evalArrayElmSize(src) == TXN_evalArrayElmSize(dst));
            u32 elmSize = TXN_evalArrayElmSize(src);

            assert(dataStack->length >= 1);
            if (vec_last(dataStack).b)
            {
                SORCER_Value* elm = dataStack->data + dataStack->length;
                vec_resize(dataStack, dataStack->length + elmSize);
                bool r = TXN_evalArrayGetElm(src, pos, elm);
                assert(r);
                TXN_evalArrayPushElm(dst, elm);
                vec_resize(dataStack, dataStack->length - elmSize);
            }
            vec_pop(dataStack);
            pos = ++cb->pos;
            if (pos < size)
            {
                SORCER_Value* elm = dataStack->data + dataStack->length;
                vec_resize(dataStack, dataStack->length + elmSize);
                bool r = TXN_evalArrayGetElm(src, pos, elm);
                assert(r);

                TXN_Node srcNode = curCall->srcNode;
                assert(TXN_isSeqSquare(space, srcNode));
                u32 bodyLen = TXN_seqLen(space, srcNode);
                curCall->p -= bodyLen;
            }
            else
            {
                SORCER_Value v = { .ary = dst, SORCER_ValueType_Object };
                vec_push(dataStack, v);

                vec_resize(aryStack, aryStack->length - 2);
                TXN_evalLeaveBlock(ctx);
            }
            goto next;
        }
        case SORCER_BlockCallbackType_ArrayReduce:
        {
            assert(aryStack->length >= 1);
            SORCER_Array* src = aryStack->data[aryStack->length - 1];

            u32 pos = cb->pos;
            u32 size = TXN_evalArraySize(src);
            u32 elmSize = TXN_evalArrayElmSize(src);
            assert(dataStack->length >= elmSize);

            pos = ++cb->pos;
            if (pos < size)
            {
                SORCER_Value* elm = dataStack->data + dataStack->length;
                vec_resize(dataStack, dataStack->length + elmSize);
                bool r = TXN_evalArrayGetElm(src, pos, elm);
                assert(r);

                TXN_Node srcNode = curCall->srcNode;
                assert(TXN_isSeqSquare(space, srcNode));
                u32 bodyLen = TXN_seqLen(space, srcNode);
                curCall->p -= bodyLen;
            }
            else
            {
                vec_resize(aryStack, aryStack->length - 1);
                TXN_evalLeaveBlock(ctx);
            }
            goto next;
        }
        default:
            assert(false);
            goto next;
        }
        if (TXN_evalLeaveBlock(ctx))
        {
            goto next;
        }
        return;
    }
    TXN_Node node = *(curCall->p++);
#ifndef NDEBUG
    const TXN_NodeSrcInfo* nodeSrcInfo = TXN_nodeSrcInfo(&ctx->srcInfo, node);
#endif
    SORCER_Node* enode = nodeTable->data + node.id;
    switch (enode->type)
    {
    case SORCER_NodeType_VarDefBegin:
    {
        assert(TXN_isTok(space, node));
        const TXN_Node* begin = curCall->p;
        for (u32 n = 0;;)
        {
            assert(curCall->p < curCall->end);
            node = *(curCall->p++);
            if (TXN_isTok(space, node))
            {
                enode = nodeTable->data + node.id;
                if (SORCER_NodeType_VarDefEnd == enode->type)
                {
                    assert(n <= dataStack->length);
                    u32 off = dataStack->length - n;
                    for (u32 i = 0; i < n; ++i)
                    {
                        SORCER_Value val = dataStack->data[off + i];
                        vec_push(&ctx->varStack, val);
                    }
                    vec_resize(dataStack, off);
                    goto next;
                }
                else
                {
                    assert(SORCER_NodeType_None == enode->type);
                }
                ++n;
            }
        }
    }
    case SORCER_NodeType_GC:
    {
        TXN_evalGC(ctx);
        goto next;
    }
    case SORCER_NodeType_Apply:
    {
        SORCER_Value v = vec_last(dataStack);
        vec_pop(dataStack);
        TXN_Node blkSrc = v.src;
        assert(TXN_isSeqSquare(space, blkSrc));
        u32 bodyLen = TXN_seqLen(space, blkSrc);
        const TXN_Node* body = TXN_seqElm(space, blkSrc);
        TXN_evalEnterBlock(ctx, body, bodyLen, blkSrc);
        goto next;
    }
    case SORCER_NodeType_BlockExe:
    {
        assert(TXN_isSeqCurly(space, node));
        u32 bodyLen = TXN_seqLen(space, node);
        const TXN_Node* body = TXN_seqElm(space, node);
        TXN_evalEnterBlock(ctx, body, bodyLen, node);
        goto next;
    }
    case SORCER_NodeType_Afun:
    {
        assert(TXN_isTok(space, node));
        assert(dataStack->length >= enode->numIns);
        SORCER_AfunInfo* afunInfo = ctx->afunTable.data + enode->afun;
        assert(afunInfo->call);
        TXN_evalAfunCall(ctx, afunInfo, node, enode);
        goto next;
    }
    case SORCER_NodeType_Var:
    {
        assert(TXN_isTok(space, node));
        SORCER_Value* val = TXN_evalGetVar(ctx, &enode->var);
        vec_push(dataStack, *val);
        goto next;
    }
    case SORCER_NodeType_Word:
    {
        assert(TXN_isTok(space, node));
        TXN_Node blkSrc = enode->blkSrc;
        TXN_Node* body = NULL;
        u32 bodyLen = 0;
        TXN_evalWordDefGetBody(ctx, blkSrc, &bodyLen, &body);
        TXN_evalEnterBlock(ctx, body, bodyLen, blkSrc);
        goto next;
    }
    case SORCER_NodeType_Atom:
    {
        assert(TXN_isTok(space, node));
        u32 l = TXN_tokSize(space, node);
        const char* s = TXN_tokCstr(space, node);
        SORCER_Value v = TXN_evalNewAtom(ctx, s, l, enode->atype);
        vec_push(dataStack, v);
        goto next;
    }
    case SORCER_NodeType_Block:
    {
        assert(TXN_isSeqSquare(space, node));
        SORCER_Value v = { 0 };
        v.src = enode->blkSrc;
        vec_push(dataStack, v);
        goto next;
    }
    case SORCER_NodeType_CallAfun:
    {
        assert(TXN_evalCheckCall(space, node));
        const TXN_Node* elms = TXN_seqElm(space, node);
        u32 len = TXN_seqLen(space, node);
        u32 afun = enode->afun;
        assert(afun != -1);
        SORCER_AfunInfo* afunInfo = ctx->afunTable.data + afun;
        assert(afunInfo->call);
        SORCER_BlockCallback cb = { SORCER_BlockCallbackType_Ncall, .afun = afun };
        TXN_evalEnterBlockWithCB(ctx, elms + 1, len - 1, node, cb);
        goto next;
    }
    case SORCER_NodeType_CallWord:
    {
        assert(TXN_evalCheckCall(space, node));
        const TXN_Node* elms = TXN_seqElm(space, node);
        u32 len = TXN_seqLen(space, node);
        SORCER_BlockCallback cb = { SORCER_BlockCallbackType_CallWord, .blkSrc = enode->blkSrc };
        TXN_evalEnterBlockWithCB(ctx, elms + 1, len - 1, node, cb);
        goto next;
    }
    case SORCER_NodeType_CallVar:
    {
        assert(TXN_evalCheckCall(space, node));
        SORCER_Value* val = TXN_evalGetVar(ctx, &enode->var);
        TXN_Node blkSrc = val->src;
        assert(TXN_isSeqSquare(space, blkSrc));
        const TXN_Node* elms = TXN_seqElm(space, node);
        u32 len = TXN_seqLen(space, node);
        SORCER_BlockCallback cb = { SORCER_BlockCallbackType_CallVar, .blkSrc = blkSrc };
        TXN_evalEnterBlockWithCB(ctx, elms + 1, len - 1, node, cb);
        goto next;
    }
    case SORCER_NodeType_Def:
    {
        assert(TXN_evalCheckCall(space, node));
        assert(SORCER_BlockCallbackType_NONE == curCall->cb.type);
        goto next;
    }
    case SORCER_NodeType_If:
    {
        assert(TXN_evalCheckCall(space, node));
        const TXN_Node* elms = TXN_seqElm(space, node);
        u32 len = TXN_seqLen(space, node);
        assert(4 == len);
        SORCER_BlockCallback cb = { SORCER_BlockCallbackType_Cond };
        TXN_evalEnterBlockWithCB(ctx, elms + 1, 1, node, cb);
        goto next;
    }
    default:
        assert(false);
    }
}































void TXN_evalAfunCall_Array(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    u32 size;
    bool r = APNUM_ratToU32(ctx->numPool, ins[0].a, &size);
    if (!r) size = 0;
    outs[0] = TXN_evalNewArray(ctx, size);
}



void TXN_evalAfunCall_AtLoad(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    bool r;
    SORCER_Array* ary = ins[0].ary;
    u32 pos;
    r = APNUM_ratToU32(ctx->numPool, ins[1].a, &pos);
    if (r)
    {
        r = TXN_evalArrayGetElm(ary, pos, outs);
    }
    if (!r)
    {
        u32 elmSize = TXN_evalArrayElmSize(ary);
        memset(outs, 0, elmSize * sizeof(SORCER_Value));
    }
}



void TXN_evalAfunCall_AtSave(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    bool r;
    SORCER_Array* ary = ins[0].ary;
    u32 pos;
    r = APNUM_ratToU32(ctx->numPool, ins[1].a, &pos);
    if (r)
    {
        TXN_evalArraySetElm(ary, pos, ins + 2);
    }
}



void TXN_evalAfunCall_Size(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    SORCER_Array* ary = ins[0].ary;
    u32 size = TXN_evalArraySize(ary);
    APNUM_rat* n = APNUM_ratNew(ctx->numPool);
    APNUM_ratFromU32(ctx->numPool, n, size, 1, false);
    outs[0].a = n;
    // todo memory management
    // assert(false);
}



void TXN_evalAfunCall_Map(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    SORCER_ValueVec* dataStack = &ctx->dataStack;
    SORCER_ArrayPtrVec* aryStack = &ctx->aryStack;

    assert(SORCER_ValueType_Object == ins[0].type);
    assert(SORCER_ValueType_Inline == ins[1].type);

    SORCER_Array* src = ins[0].ary;
    vec_push(aryStack, src);
    u32 size = TXN_evalArraySize(src);
    SORCER_Array* dst = TXN_evalNewArray(ctx, size).ary;
    vec_push(aryStack, dst);

    TXN_Node blkSrc = ins[1].src;
    assert(TXN_isSeqSquare(space, blkSrc));
    u32 bodyLen = TXN_seqLen(space, blkSrc);
    const TXN_Node* body = TXN_seqElm(space, blkSrc);
    SORCER_Node* blkEnode = ctx->nodeTable.data + blkSrc.id;
    TXN_evalArraySetElmSize(dst, blkEnode->numOuts);

    u32 elmSize = TXN_evalArrayElmSize(src);
    SORCER_Value* outBuf = dataStack->data + dataStack->length - 2;
    vec_resize(dataStack, dataStack->length - 2 + elmSize);
    bool r = TXN_evalArrayGetElm(src, 0, outBuf);
    assert(r);

    SORCER_BlockCallback cb = { SORCER_BlockCallbackType_ArrayMap, .pos = 0 };
    TXN_evalEnterBlockWithCB(ctx, body, bodyLen, blkSrc, cb);
}




void TXN_evalAfunCall_Filter(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    SORCER_ValueVec* dataStack = &ctx->dataStack;
    SORCER_ArrayPtrVec* aryStack = &ctx->aryStack;

    assert(SORCER_ValueType_Object == ins[0].type);
    assert(SORCER_ValueType_Inline == ins[1].type);

    SORCER_Array* src = ins[0].ary;
    vec_push(aryStack, src);
    SORCER_Array* dst = TXN_evalNewArray(ctx, 0).ary;
    vec_push(aryStack, dst);

    TXN_Node blkSrc = ins[1].src;
    assert(TXN_isSeqSquare(space, blkSrc));
    u32 bodyLen = TXN_seqLen(space, blkSrc);
    const TXN_Node* body = TXN_seqElm(space, blkSrc);
    SORCER_Node* blkEnode = ctx->nodeTable.data + blkSrc.id;
    assert(blkEnode->numIns == blkEnode->numOuts);

    u32 elmSize = TXN_evalArrayElmSize(src);
    SORCER_Value* outBuf = dataStack->data + dataStack->length - 2;
    vec_resize(dataStack, dataStack->length - 2 + elmSize);
    bool r = TXN_evalArrayGetElm(src, 0, outBuf);
    assert(r);
    TXN_evalArraySetElmSize(dst, elmSize);
    assert(elmSize == blkEnode->numOuts);

    SORCER_BlockCallback cb = { SORCER_BlockCallbackType_ArrayFilter, .pos = 0 };
    TXN_evalEnterBlockWithCB(ctx, body, bodyLen, blkSrc, cb);
}




void TXN_evalAfunCall_Reduce(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx)
{
    SORCER_ValueVec* dataStack = &ctx->dataStack;
    SORCER_ArrayPtrVec* aryStack = &ctx->aryStack;

    assert(SORCER_ValueType_Object == ins[0].type);
    assert(SORCER_ValueType_Inline == ins[1].type);

    SORCER_Array* src = ins[0].ary;
    vec_push(aryStack, src);

    TXN_Node blkSrc = ins[1].src;
    assert(TXN_isSeqSquare(space, blkSrc));
    u32 bodyLen = TXN_seqLen(space, blkSrc);
    const TXN_Node* body = TXN_seqElm(space, blkSrc);
    SORCER_Node* blkEnode = ctx->nodeTable.data + blkSrc.id;
    assert(blkEnode->numIns == blkEnode->numOuts * 2);

    u32 elmSize = TXN_evalArrayElmSize(src);
    SORCER_Value* outBuf = dataStack->data + dataStack->length - 2;
    vec_resize(dataStack, dataStack->length - 2 + elmSize * 2);
    bool r;
    r = TXN_evalArrayGetElm(src, 0, outBuf);
    assert(r);
    r = TXN_evalArrayGetElm(src, 1, outBuf + elmSize);
    assert(r);

    SORCER_BlockCallback cb = { SORCER_BlockCallbackType_ArrayReduce, .pos = 1 };
    TXN_evalEnterBlockWithCB(ctx, body, bodyLen, blkSrc, cb);
}































void SORCER_execBlock(SORCER_Context* ctx, TXN_Node root)
{
    TXN_Space* space = ctx->space;
    u32 len = TXN_seqLen(space, root);
    const TXN_Node* seq = TXN_seqElm(space, root);
    TXN_evalEnterBlock(ctx, seq, len, root);
    TXN_evalCall(ctx);
}









SORCER_Error TXN_evalCompile
(
    TXN_Space* space, TXN_Node root, TXN_SpaceSrcInfo* srcInfo,
    SORCER_AtypeInfoVec* atypeTable, SORCER_AfunInfoVec* afunTable, vec_u32* afunTypeTable,
    SORCER_NodeTable* nodeTable,
    SORCER_TypeContext* typeContext, SORCER_TypeDeclContext* typeDeclContext, vec_u32* typeStack,
    SORCER_Context* evalContext
);



bool SORCER_execCode(SORCER_Context* ctx, const char* filename, const char* code, bool enableSrcInfo)
{
    if (enableSrcInfo)
    {
        SORCER_SrcFileInfo fileInfo = { 0 };
        stzncpy(fileInfo.name, filename, SORCER_FileName_MAX);
        vec_push(&ctx->fileInfoTable, fileInfo);
    }
    TXN_SpaceSrcInfo* srcInfo = NULL;
    if (enableSrcInfo)
    {
        srcInfo = &ctx->srcInfo;
    }
    TXN_Space* space = ctx->space;
    TXN_Node root = TXN_parseAsList(space, code, srcInfo);
    if (TXN_Node_Invalid.id == root.id)
    {
        ctx->error.code = SORCER_ErrCode_ExpSyntax;
        ctx->error.file = 0;
        if (srcInfo)
        {
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable : 6011)
#endif
            ctx->error.line = vec_last(&srcInfo->nodes).line;
            ctx->error.column = vec_last(&srcInfo->nodes).column;
#ifdef _MSC_VER
# pragma warning(pop)
#endif
        }
        else
        {
            ctx->error.line = -1;
            ctx->error.column = -1;
        }
        return false;
    }


    if (!TXN_isSeq(space, root))
    {
        ctx->error.code = SORCER_ErrCode_EvalSyntax;
        ctx->error.file = srcInfo->nodes.data[root.id].file;
        ctx->error.line = srcInfo->nodes.data[root.id].line;
        ctx->error.column = srcInfo->nodes.data[root.id].column;
        return false;
    }
    SORCER_Error error = TXN_evalCompile
    (
        space, root, &ctx->srcInfo,
        &ctx->atypeTable, &ctx->afunTable, &ctx->afunTypeTable,
        &ctx->nodeTable,
        ctx->typeContext, ctx->typeDeclContext, &ctx->typeStack,
        ctx
    );
    if (error.code)
    {
        ctx->error = error;
        return false;
    }
    SORCER_execBlock(ctx, root);
    if (ctx->error.code)
    {
        return false;
    }
    return true;
}



bool SORCER_execFile(SORCER_Context* ctx, const char* fileName, bool enableSrcInfo)
{
    char* code = NULL;
    u32 codeSize = FILEU_readFile(fileName, &code);
    if (-1 == codeSize)
    {
        ctx->error.code = SORCER_ErrCode_SrcFile;
        ctx->error.file = 0;
        return false;
    }
    if (0 == codeSize)
    {
        return false;
    }
    bool r = SORCER_execCode(ctx, fileName, code, enableSrcInfo);
    free(code);
    return r;
}

























const SORCER_FileInfoTable* SORCER_ctxSrcFileInfoTable(SORCER_Context* ctx)
{
    return &ctx->fileInfoTable;
}













const char** SORCER_ErrCodeNameTable(void)
{
    static const char* a[TXN_NumEvalErrorCodes] =
    {
        "NONE",
        "SrcFile",
        "ExpSyntax",
        "EvalSyntax",
        "EvalUnkWord",
        "EvalUnkCall",
        "EvalUnkFunType",
        "EvalUnkTypeDecl",
        "EvalArgs",
        "EvalBranchUneq",
        "EvalRecurNoBaseCase",
        "EvalUnification",
        "EvalAtomCtorByStr",
        "EvalTypeUnsolvable",
    };
    return a;
}


*/









































































































