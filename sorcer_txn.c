#include "sorcer_a.h"
#include "sorcer_txn.h"
#include <fileu.h>




typedef enum SORCER_TxnPrimWord
{
    SORCER_TxnPrimWord_Invalid = -1,

    SORCER_TxnPrimWord_Apply,
    SORCER_TxnPrimWord_Ifte,

    SORCER_NumTxnPrimWords
} SORCER_TxnPrimWord;

const char** SORCER_TxnPrimWordNameTable(void)
{
    static const char* a[SORCER_NumTxnPrimWords] =
    {
        "apply",
        "ifte",
    };
    return a;
}




typedef enum SORCER_TxnKeyExpr
{
    SORCER_TxnKeyExpr_Invalid = -1,

    SORCER_TxnKeyExpr_Def,
    SORCER_TxnKeyExpr_Var,

    SORCER_NumTxnKeyExprs
} SORCER_TxnKeyExpr;

const char** SORCER_TxnKeyExprHeadNameTable(void)
{
    static const char* a[SORCER_NumTxnKeyExprs] =
    {
        "def",
        "var",
    };
    return a;
}





typedef struct SORCER_TxnLoadStepInfo
{
    const char* name;
} SORCER_TxnLoadStepInfo;

typedef vec_t(SORCER_TxnLoadStepInfo) SORCER_TxnStepInfoVec;






static bool SORCER_txnLoadCellFromSym(SORCER_Cell* out, const char* name)
{
    return true;
}






typedef struct SORCER_TxnLoadVar
{
    const char* name;
    SORCER_Var var;
} SORCER_TxnLoadVar;

typedef vec_t(SORCER_TxnLoadVar) SORCER_TxnLoadVarVec;



typedef struct SORCER_TxnLoadDef
{
    const char* name;
    SORCER_Block block;
} SORCER_TxnLoadDef;

typedef vec_t(SORCER_TxnLoadDef) SORCER_TxnLoadDefVec;



typedef struct SORCER_TxnLoadBlock
{
    SORCER_Block scope;
    SORCER_TxnLoadDefVec defTable[1];
    SORCER_TxnLoadVarVec varTable[1];
} SORCER_TxnLoadBlock;

static void SORCER_txnLoadBlockFree(SORCER_TxnLoadBlock* b)
{
    vec_free(b->varTable);
    vec_free(b->defTable);
}


typedef vec_t(SORCER_TxnLoadBlock) SORCER_TxnLoadBlockVec;





typedef struct SORCER_TxnLoadCallLevel
{
    SORCER_Block block;
    const TXN_Node* seq;
    u32 len;
    u32 p;
} SORCER_TxnLoadCallLevel;

typedef vec_t(SORCER_TxnLoadCallLevel) SORCER_TxnLoadCallStack;





typedef struct SORCER_TxnLoadBlockReq
{
    SORCER_Block block;
    const TXN_Node* seq;
    u32 len;
} SORCER_TxnLoadBlockReq;

typedef vec_t(SORCER_TxnLoadBlockReq) SORCER_TxnLoadBlockReqVec;





typedef struct SORCER_TxnLoadContext
{
    SORCER_Context* sorcer;
    TXN_Space* space;
    const TXN_SpaceSrcInfo* srcInfo;
    SORCER_TxnErrInfo* errInfo;
    u32 blockBaseId;
    SORCER_TxnLoadBlockVec blockTable[1];
    SORCER_TxnLoadBlockReqVec blockReqs[1];
    SORCER_TxnLoadCallStack callStack[1];
} SORCER_TxnLoadContext;

static SORCER_TxnLoadContext SORCER_txnLoadContextNew
(
    SORCER_Context* sorcer, TXN_Space* space, const TXN_SpaceSrcInfo* srcInfo, SORCER_TxnErrInfo* errInfo
)
{
    SORCER_TxnLoadContext ctx[1] = { 0 };
    ctx->sorcer = sorcer;
    ctx->space = space;
    ctx->srcInfo = srcInfo;
    ctx->errInfo = errInfo;
    ctx->blockBaseId = SORCER_ctxBlocksTotal(sorcer);
    return *ctx;
}

static void SORCER_txnLoadContextFree(SORCER_TxnLoadContext* ctx)
{
    vec_free(ctx->callStack);
    for (u32 i = 0; i < ctx->blockTable->length; ++i)
    {
        SORCER_txnLoadBlockFree(ctx->blockTable->data + i);
    }
    vec_free(ctx->blockReqs);
    vec_free(ctx->blockTable);
}








static void SORCER_txnLoadErrorAtNode(SORCER_TxnLoadContext* ctx, TXN_Node node, SORCER_TxnErr err)
{
    const TXN_SpaceSrcInfo* srcInfo = ctx->srcInfo;
    SORCER_TxnErrInfo* errInfo = ctx->errInfo;
    if (errInfo && srcInfo)
    {
        errInfo->error = err;
        const TXN_NodeSrcInfo* nodeSrcInfo = TXN_nodeSrcInfo(srcInfo, node);
        errInfo->file = nodeSrcInfo->file;
        errInfo->line = nodeSrcInfo->line;
        errInfo->column = nodeSrcInfo->column;
    }
}









static SORCER_TxnPrimWord SORCER_txnPrimWordFromName(const char* name)
{
    for (SORCER_TxnPrimWord i = 0; i < SORCER_NumTxnPrimWords; ++i)
    {
        SORCER_TxnPrimWord k = SORCER_NumTxnPrimWords - 1 - i;
        const char* s = SORCER_TxnPrimWordNameTable()[k];
        if (0 == strcmp(s, name))
        {
            return k;
        }
    }
    return SORCER_TxnPrimWord_Invalid;
}


static SORCER_TxnKeyExpr SORCER_txnKeyExprFromHeadName(const char* name)
{
    for (SORCER_TxnKeyExpr i = 0; i < SORCER_NumTxnKeyExprs; ++i)
    {
        SORCER_TxnKeyExpr k = SORCER_NumTxnKeyExprs - 1 - i;
        const char* s = SORCER_TxnKeyExprHeadNameTable()[k];
        if (0 == strcmp(s, name))
        {
            return k;
        }
    }
    return SORCER_TxnKeyExpr_Invalid;
}




static SORCER_Var SORCER_txnLoadFindVar(SORCER_TxnLoadContext* ctx, const char* name, SORCER_Block scope)
{
    SORCER_TxnLoadBlockVec* blockTable = ctx->blockTable;
    while (scope.id != SORCER_Block_Invalid.id)
    {
        SORCER_TxnLoadBlock* blockInfo = blockTable->data + scope.id - ctx->blockBaseId;
        for (u32 i = 0; i < blockInfo->varTable->length; ++i)
        {
            SORCER_TxnLoadVar* info = blockInfo->varTable->data + i;
            if (info->name == name)
            {
                return info->var;
            }
        }
        scope = blockInfo->scope;
    }
    return SORCER_Var_Invalid;
}

static SORCER_Block SORCER_txnLoadFindDef(SORCER_TxnLoadContext* ctx, const char* name, SORCER_Block scope)
{
    SORCER_TxnLoadBlockVec* blockTable = ctx->blockTable;
    while (scope.id != SORCER_Block_Invalid.id)
    {
        SORCER_TxnLoadBlock* blockInfo = blockTable->data + scope.id - ctx->blockBaseId;
        for (u32 i = 0; i < blockInfo->defTable->length; ++i)
        {
            SORCER_TxnLoadDef* info = blockInfo->defTable->data + i;
            if (info->name == name)
            {
                return info->block;
            }
        }
        scope = blockInfo->scope;
    }
    return SORCER_Block_Invalid;
}

static SORCER_Step SORCER_txnLoadFindStep(const char* name)
{
    for (u32 i = 0; i < SORCER_NumSteps; ++i)
    {
        u32 idx = SORCER_NumSteps - 1 - i;
        const char* s = SORCER_StepInfoTable[idx].name;
        if (0 == strcmp(s, name))
        {
            SORCER_Step step = { idx };
            return step;
        }
    }
    return SORCER_Step_Invalid;
}





static bool SORCER_txnLoadCheckCall(TXN_Space* space, TXN_Node node)
{
    if (!TXN_isSeqRound(space, node))
    {
        return false;
    }
    u32 len = TXN_seqLen(space, node);
    if (!len)
    {
        return false;
    }
    const TXN_Node* elms = TXN_seqElm(space, node);
    if (!TXN_isTok(space, elms[0]))
    {
        return false;
    }
    if (TXN_tokQuoted(space, elms[0]))
    {
        return false;
    }
    return true;
}









SORCER_Block SORCER_txnLoadBlock(SORCER_TxnLoadContext* ctx, const TXN_Node* seq, u32 len)
{
    SORCER_Context* sorcer = ctx->sorcer;
    TXN_Space* space = ctx->space;
    SORCER_TxnLoadBlockVec* blockTable = ctx->blockTable;
    SORCER_TxnLoadBlockReqVec* blockReqs = ctx->blockReqs;
    SORCER_TxnLoadCallStack* callStack = ctx->callStack;
    u32 blocksTotal0 = SORCER_ctxBlocksTotal(sorcer);

    SORCER_TxnLoadCallLevel level = { SORCER_blockNew(sorcer), seq, len };
    vec_push(callStack, level);
    SORCER_TxnLoadCallLevel* cur = NULL;
next:
    cur = &vec_last(callStack);
    if (level.p == level.len)
    {
        if (callStack->length > 0)
        {
            SORCER_Block ib = cur->block;
            vec_pop(callStack);
            SORCER_Block blk = vec_last(callStack).block;
            assert(ib.id != blk.id);
            SORCER_blockAddInlineBlock(sorcer, blk, ib);
            goto next;
        }
        else
        {
            if (blockReqs->length > 0)
            {
                SORCER_TxnLoadBlockReq req = vec_last(blockReqs);
                SORCER_TxnLoadCallLevel level = { req.block, req.seq, req.len };
                vec_push(callStack, level);
                vec_pop(blockReqs);
                goto next;
            }
            else
            {
                SORCER_Block b = cur->block;
                vec_pop(callStack);
                return b;
            }
        }
    }
    TXN_Node node = seq[level.p++];
    if (TXN_isTok(space, node))
    {
        if (TXN_tokQuoted(space, node))
        {
            // todo
            SORCER_Cell str[1] = { 0 };
            SORCER_blockAddInstPushCell(sorcer, cur->block, str);
            goto next;
        }
        else
        {
            const char* name = TXN_tokCstr(space, node);
            SORCER_TxnPrimWord primWord = SORCER_txnPrimWordFromName(name);
            if (primWord != SORCER_TxnPrimWord_Invalid)
            {
                switch (primWord)
                {
                case SORCER_TxnPrimWord_Apply:
                {
                    SORCER_blockAddInstApply(sorcer, cur->block);
                    break;
                }
                case SORCER_TxnPrimWord_Ifte:
                {
                    SORCER_blockAddInstIfte(sorcer, cur->block);
                    break;
                }
                default:
                    assert(false);
                    break;
                }
                goto next;
            }
            SORCER_Var var = SORCER_txnLoadFindVar(ctx, name, cur->block);
            if (var.id != SORCER_Var_Invalid.id)
            {
                SORCER_blockAddInstPushVar(sorcer, cur->block, var);
                goto next;
            }
            SORCER_Block def = SORCER_txnLoadFindDef(ctx, name, cur->block);
            if (def.id != SORCER_Block_Invalid.id)
            {
                SORCER_blockAddInstPushBlock(sorcer, cur->block, def);
                goto next;
            }
            SORCER_Step step = SORCER_txnLoadFindStep(name);
            if (step != SORCER_Step_Invalid)
            {
                SORCER_blockAddInstStep(sorcer, cur->block, step);
                goto next;
            }
            SORCER_Cell cell[1];
            if (SORCER_txnLoadCellFromSym(cell, name))
            {
                SORCER_blockAddInstPushCell(sorcer, cur->block, cell);
                goto next;
            }
            else
            {
                SORCER_txnLoadErrorAtNode(ctx, node, SORCER_TxnErr_UnkWord);
                goto failed;
            }
        }
    }
    else if (TXN_isSeqCurly(space, node))
    {
        const TXN_Node* body = TXN_seqElm(space, node);
        u32 bodyLen = TXN_seqLen(space, node);
        SORCER_TxnLoadCallLevel level = { SORCER_blockNew(sorcer), body, bodyLen };
        vec_push(callStack, level);
        goto next;
    }
    else if (TXN_isSeqSquare(space, node))
    {
        SORCER_Block block = SORCER_blockNew(sorcer);
        SORCER_blockAddInstPushBlock(sorcer, cur->block, block);
        const TXN_Node* body = TXN_seqElm(space, node);
        u32 bodyLen = TXN_seqLen(space, node);
        SORCER_TxnLoadBlockReq req = { block, body, bodyLen };
        vec_push(blockReqs, req);
        goto next;
    }
    else if (SORCER_txnLoadCheckCall(space, node))
    {
        const TXN_Node* elms = TXN_seqElm(space, node);
        u32 len = TXN_seqLen(space, node);
        assert(len > 0);
        const char* name = TXN_tokCstr(space, elms[0]);
        SORCER_TxnKeyExpr expr = SORCER_txnKeyExprFromHeadName(name);
        if (expr != SORCER_TxnKeyExpr_Invalid)
        {
            switch (expr)
            {
            case SORCER_TxnKeyExpr_Def:
            {
                goto next;
            }
            case SORCER_TxnKeyExpr_Var:
            {
                for (u32 i = 1; i < len; ++i)
                {
                    u32 j = len - 1 - i;
                    const char* name = TXN_tokCstr(space, elms[j]);
                    SORCER_Var var = SORCER_blockAddInstPopVar(sorcer, cur->block);
                    SORCER_TxnLoadBlock* blkInfo = blockTable->data + cur->block.id;
                    SORCER_TxnLoadVar varInfo = { name, var };
                    vec_push(blkInfo->varTable, varInfo);
                }
                goto next;
            }
            default:
            {
                assert(false);
                break;
            }
            }
        }
        SORCER_txnLoadErrorAtNode(ctx, node, SORCER_TxnErr_UnkCall);
        goto failed;
    }
    SORCER_txnLoadErrorAtNode(ctx, node, SORCER_TxnErr_UnkFormat);
failed:
    SORCER_ctxBlocksRollback(sorcer, blocksTotal0);
    return SORCER_Block_Invalid;
}






SORCER_Block SORCER_blockFromTxnNode
(
    SORCER_Context* ctx, TXN_Space* space, TXN_Node node, const TXN_SpaceSrcInfo* srcInfo, SORCER_TxnErrInfo* errInfo
)
{
    SORCER_Block block = SORCER_Block_Invalid;
    const TXN_Node* seq = TXN_seqElm(space, node);
    u32 len = TXN_seqLen(space, node);
    if (!len)
    {
        return block;
    }
    SORCER_TxnLoadContext tctx[1] = { SORCER_txnLoadContextNew(ctx, space, srcInfo, errInfo) };
    SORCER_Block r = SORCER_txnLoadBlock(tctx, seq, len);
    SORCER_txnLoadContextFree(tctx);
    return r;
}






SORCER_Block SORCER_blockFromTxnFile(SORCER_Context* ctx, const char* path, SORCER_TxnErrInfo* errInfo)
{
    SORCER_Block block = SORCER_Block_Invalid;
    char* str;
    u32 strSize = FILEU_readFile(path, &str);
    if (-1 == strSize)
    {
        return block;
    }
    TXN_Space* space = TXN_spaceNew();
    TXN_SpaceSrcInfo srcInfo[1] = { 0 };
    TXN_Node root = TXN_parseAsList(space, str, srcInfo);
    free(str);
    if (TXN_Node_Invalid.id == root.id)
    {
        goto out;
    }
    block = SORCER_blockFromTxnNode(ctx, space, root, srcInfo, errInfo);
out:
    TXN_spaceSrcInfoFree(srcInfo);
    TXN_spaceFree(space);
    return block;
}

















































































