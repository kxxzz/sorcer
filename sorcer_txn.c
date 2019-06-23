#include "sorcer_a.h"
#include "sorcer_txn.h"
#include <fileu.h>




typedef enum SORCER_TxnPrimWord
{
    SORCER_TxnPrimWord_Invalid = -1,

    SORCER_TxnPrimWord_Apply,

    SORCER_NumTxnPrimWords
} SORCER_TxnPrimWord;

const char** SORCER_TxnPrimWordNameTable(void)
{
    static const char* a[SORCER_NumTxnPrimWords] =
    {
        "apply",
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
    const TXN_Node* body;
    u32 bodyLen;
    SORCER_TxnLoadDefVec defTable[1];
    SORCER_TxnLoadVarVec varTable[1];
    bool loaded;
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





typedef struct SORCER_TxnLoadContext
{
    SORCER_Context* sorcer;
    TXN_Space* space;
    const TXN_SpaceSrcInfo* srcInfo;
    SORCER_TxnErrorInfo* errInfo;
    u32 blockBaseId;
    SORCER_TxnLoadBlockVec blockTable[1];
    SORCER_TxnLoadCallStack callStack[1];
} SORCER_TxnLoadContext;

static SORCER_TxnLoadContext SORCER_txnLoadContextNew
(
    SORCER_Context* sorcer, TXN_Space* space, const TXN_SpaceSrcInfo* srcInfo, SORCER_TxnErrorInfo* errInfo
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
    vec_free(ctx->blockTable);
}






static void SORCER_txnLoadErrorAtNode(SORCER_TxnLoadContext* ctx, TXN_Node node, SORCER_TxnError err)
{
    const TXN_SpaceSrcInfo* srcInfo = ctx->srcInfo;
    SORCER_TxnErrorInfo* errInfo = ctx->errInfo;
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

static SORCER_Opr SORCER_txnLoadFindOpr(SORCER_TxnLoadContext* ctx, const char* name)
{
    return SORCER_oprByName(ctx->sorcer, name);
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













static SORCER_Block SORCER_txnLoadBlockNew(SORCER_TxnLoadContext* ctx, SORCER_Block scope, const TXN_Node* body, u32 bodyLen)
{
    SORCER_Context* sorcer = ctx->sorcer;
    SORCER_TxnLoadBlockVec* blockTable = ctx->blockTable;

    SORCER_TxnLoadBlock info = { scope, body, bodyLen };
    vec_push(blockTable, info);
    return SORCER_blockNew(sorcer);
}


static void SORCER_txnLoadBlocksRollback(SORCER_TxnLoadContext* ctx, u32 n)
{
    SORCER_Context* sorcer = ctx->sorcer;
    SORCER_TxnLoadBlockVec* blockTable = ctx->blockTable;
    for (u32 i = n; i < blockTable->length; ++i)
    {
        SORCER_txnLoadBlockFree(blockTable->data + i);
    }
    vec_resize(blockTable, n);
    SORCER_ctxBlocksRollback(sorcer, n);
}











static SORCER_Block SORCER_txnLoadBlockDefTree(SORCER_TxnLoadContext* ctx)
{
    SORCER_Context* sorcer = ctx->sorcer;
    TXN_Space* space = ctx->space;
    SORCER_TxnLoadBlockVec* blockTable = ctx->blockTable;
    SORCER_TxnLoadCallStack* callStack = ctx->callStack;

    u32 begin = callStack->length;
    SORCER_TxnLoadCallLevel* cur = NULL;
next:
    cur = &vec_last(callStack);
    if (cur->p == cur->len)
    {
        if (callStack->length > begin)
        {
            vec_pop(callStack);
            goto next;
        }
        else
        {
            cur->p = 0;
            return cur->block;
        }
    }
    TXN_Node node = cur->seq[cur->p++];
    if (SORCER_txnLoadCheckCall(space, node))
    {
        const TXN_Node* elms = TXN_seqElm(space, node);
        u32 len = TXN_seqLen(space, node);
        if (len < 2)
        {
            SORCER_txnLoadErrorAtNode(ctx, node, SORCER_TxnError_Syntax);
            return SORCER_Block_Invalid;
        }
        const char* name = TXN_tokCstr(space, elms[0]);
        SORCER_TxnKeyExpr expr = SORCER_txnKeyExprFromHeadName(name);
        if (expr != SORCER_TxnKeyExpr_Invalid)
        {
            switch (expr)
            {
            case SORCER_TxnKeyExpr_Def:
            {
                if (!TXN_isTok(space, elms[1]))
                {
                    SORCER_txnLoadErrorAtNode(ctx, elms[1], SORCER_TxnError_Syntax);
                    return SORCER_Block_Invalid;
                }
                const char* defName = TXN_tokCstr(space, elms[1]);
                SORCER_Block block;
                {
                    block = SORCER_txnLoadBlockNew(ctx, cur->block, elms + 2, len - 2);
                    SORCER_TxnLoadCallLevel level = { block, elms + 2, len - 2 };
                    vec_push(callStack, level);
                }
                SORCER_TxnLoadDef def = { defName, block };
                SORCER_TxnLoadBlock* info = blockTable->data + cur->block.id;
                vec_push(info->defTable, def);
                break;
            }
            default:
                break;
            }
        }
    }
    goto next;
}







static void SORCER_txnLoadBlockSetLoaded(SORCER_TxnLoadContext* ctx, SORCER_Block block)
{
    SORCER_TxnLoadBlockVec* blockTable = ctx->blockTable;
    SORCER_TxnLoadBlock* blkInfo = blockTable->data + block.id - ctx->blockBaseId;
    assert(!blkInfo->loaded);
    blkInfo->loaded = true;
}







SORCER_Block SORCER_txnLoadBlock(SORCER_TxnLoadContext* ctx, const TXN_Node* rootBody, u32 rootBodyLen)
{
    SORCER_Context* sorcer = ctx->sorcer;
    TXN_Space* space = ctx->space;
    SORCER_TxnLoadBlockVec* blockTable = ctx->blockTable;
    SORCER_TxnLoadCallStack* callStack = ctx->callStack;
    u32 blocksTotal0 = SORCER_ctxBlocksTotal(sorcer);
    {
        SORCER_Block blockRoot = SORCER_txnLoadBlockNew(ctx, SORCER_Block_Invalid, rootBody, rootBodyLen);
        if (blockRoot.id == SORCER_Block_Invalid.id)
        {
            goto failed;
        }
        SORCER_TxnLoadCallLevel root = { blockRoot, rootBody, rootBodyLen };
        vec_push(callStack, root);
        SORCER_txnLoadBlockDefTree(ctx);
        SORCER_txnLoadBlockSetLoaded(ctx, blockRoot);
    }
    u32 begin = callStack->length;
    SORCER_TxnLoadCallLevel* cur = NULL;
next:
    cur = &vec_last(callStack);
    if (cur->p == cur->len)
    {
        if (callStack->length > begin)
        {
            vec_pop(callStack);
            goto next;
        }
        else
        {
            SORCER_Block blk = cur->block;
            vec_pop(callStack);
            return blk;
        }
    }
    TXN_Node node = cur->seq[cur->p++];
    if (TXN_isTok(space, node))
    {
        u32 numTypes = SORCER_ctxTypesTotal(sorcer);
        if (TXN_tokQuoted(space, node))
        {
            for (u32 i = 0; i < numTypes; ++i)
            {
                SORCER_Type type = SORCER_typeByIndex(sorcer, i);
                SORCER_Cell cell[1] = { 0 };
                if (SORCER_cellNew(sorcer, type, TXN_tokCstr(space, node), true, cell))
                {
                    SORCER_blockAddInstPushCell(sorcer, cur->block, cell);
                    goto next;
                }
            }
            SORCER_txnLoadErrorAtNode(ctx, node, SORCER_TxnError_UnkWord);
            goto failed;
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
                SORCER_blockAddInstCall(sorcer, cur->block, def);
                SORCER_TxnLoadBlock* blkInfo = blockTable->data + def.id - ctx->blockBaseId;
                if (!blkInfo->loaded)
                {
                    SORCER_txnLoadBlockSetLoaded(ctx, def);
                    SORCER_TxnLoadCallLevel level = { def, blkInfo->body, blkInfo->bodyLen };
                    vec_push(callStack, level);
                }
                goto next;
            }
            SORCER_Opr opr = SORCER_txnLoadFindOpr(ctx, name);
            if (opr.id != SORCER_Opr_Invalid.id)
            {
                SORCER_blockAddInstOpr(sorcer, cur->block, opr);
                goto next;
            }

            for (u32 i = 0; i < numTypes; ++i)
            {
                SORCER_Type type = SORCER_typeByIndex(sorcer, i);
                SORCER_Cell cell[1] = { 0 };
                if (SORCER_cellNew(sorcer, type, TXN_tokCstr(space, node), false, cell))
                {
                    SORCER_blockAddInstPushCell(sorcer, cur->block, cell);
                    goto next;
                }
            }
            SORCER_txnLoadErrorAtNode(ctx, node, SORCER_TxnError_UnkWord);
            goto failed;
        }
    }
    else if (TXN_isSeqCurly(space, node))
    {
        const TXN_Node* body = TXN_seqElm(space, node);
        u32 bodyLen = TXN_seqLen(space, node);
        SORCER_Block block = SORCER_txnLoadBlockNew(ctx, cur->block, body, bodyLen);
        if (block.id == SORCER_Block_Invalid.id)
        {
            goto failed;
        }
        SORCER_blockAddInstCall(sorcer, cur->block, block);
        SORCER_TxnLoadCallLevel level = { block, body, bodyLen, };
        vec_push(callStack, level);
        SORCER_txnLoadBlockDefTree(ctx);
        SORCER_txnLoadBlockSetLoaded(ctx, block);
        goto next;
    }
    else if (TXN_isSeqSquare(space, node))
    {
        const TXN_Node* body = TXN_seqElm(space, node);
        u32 bodyLen = TXN_seqLen(space, node);
        SORCER_Block block = SORCER_txnLoadBlockNew(ctx, cur->block, body, bodyLen);
        if (block.id == SORCER_Block_Invalid.id)
        {
            goto failed;
        }
        SORCER_blockAddInstPushBlock(sorcer, cur->block, block);
        SORCER_TxnLoadCallLevel level = { block, body, bodyLen };
        vec_push(callStack, level);
        SORCER_txnLoadBlockDefTree(ctx);
        SORCER_txnLoadBlockSetLoaded(ctx, block);
        goto next;
    }
    else if (SORCER_txnLoadCheckCall(space, node))
    {
        const TXN_Node* elms = TXN_seqElm(space, node);
        u32 len = TXN_seqLen(space, node);
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
        SORCER_txnLoadErrorAtNode(ctx, node, SORCER_TxnError_UnkCall);
        goto failed;
    }
    SORCER_txnLoadErrorAtNode(ctx, node, SORCER_TxnError_Syntax);
failed:
    SORCER_txnLoadBlocksRollback(ctx, blocksTotal0);
    return SORCER_Block_Invalid;
}






SORCER_Block SORCER_blockFromTxnNode
(
    SORCER_Context* ctx, TXN_Space* space, TXN_Node node, const TXN_SpaceSrcInfo* srcInfo, SORCER_TxnErrorInfo* errInfo
)
{
    const TXN_Node* seq = TXN_seqElm(space, node);
    u32 len = TXN_seqLen(space, node);
    SORCER_TxnLoadContext tctx[1] = { SORCER_txnLoadContextNew(ctx, space, srcInfo, errInfo) };
    SORCER_Block r = SORCER_txnLoadBlock(tctx, seq, len);
    SORCER_txnLoadContextFree(tctx);
    return r;
}




SORCER_Block SORCER_blockFromTxnCode(SORCER_Context* ctx, const char* code, SORCER_TxnErrorInfo* errInfo)
{
    SORCER_Block block = SORCER_Block_Invalid;
    TXN_Space* space = TXN_spaceNew();
    TXN_SpaceSrcInfo srcInfo[1] = { 0 };
    TXN_Node root = TXN_parseAsList(space, code, srcInfo);
    if (TXN_Node_Invalid.id == root.id)
    {
        errInfo->error = SORCER_TxnError_TxnSyntex;
        const TXN_NodeSrcInfo* nodeSrcInfo = &vec_last(srcInfo->nodes);
        errInfo->file = nodeSrcInfo->file;
        errInfo->line = nodeSrcInfo->line;
        errInfo->column = nodeSrcInfo->column;
        goto out;
    }
    block = SORCER_blockFromTxnNode(ctx, space, root, srcInfo, errInfo);
out:
    TXN_spaceSrcInfoFree(srcInfo);
    TXN_spaceFree(space);
    return block;
}




SORCER_Block SORCER_blockFromTxnFile(SORCER_Context* ctx, const char* path, SORCER_TxnErrorInfo* errInfo)
{
    char* str;
    u32 strSize = FILEU_readFile(path, &str);
    if (-1 == strSize)
    {
        errInfo->error = SORCER_TxnError_TxnFile;
        errInfo->file = 0;
        errInfo->line = 0;
        errInfo->column = 0;
        return SORCER_Block_Invalid;
    }
    SORCER_Block blk = SORCER_blockFromTxnCode(ctx, str, errInfo);
    free(str);
    return blk;
}

















































































