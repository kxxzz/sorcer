#include "sorcer_a.h"
#include "sorcer_txn.h"
#include "sorcer_typeflag.h"
#include <fileu.h>




typedef enum SORCER_TxnPrimWord
{
    SORCER_TxnPrimWord_Invalid = -1,

    SORCER_TxnPrimWord_Apply,

    SORCER_NumTxnPrimWords
} SORCER_TxnPrimWord;

const char* SORCER_TxnPrimWordNameTable(SORCER_TxnPrimWord w)
{
    assert(w < SORCER_NumTxnPrimWords);
    static const char* a[SORCER_NumTxnPrimWords] =
    {
        "apply",
    };
    return a[w];
}




typedef enum SORCER_TxnKeyExpr
{
    SORCER_TxnKeyExpr_Invalid = -1,

    SORCER_TxnKeyExpr_Def,
    SORCER_TxnKeyExpr_Var,

    SORCER_NumTxnKeyExprs
} SORCER_TxnKeyExpr;

const char* SORCER_TxnKeyExprHeadNameTable(SORCER_TxnKeyExpr expr)
{
    assert(expr < SORCER_NumTxnKeyExprs);
    static const char* a[SORCER_NumTxnKeyExprs] =
    {
        "def",
        "var",
    };
    return a[expr];
}




typedef struct SORCER_TxnLoadVarInfo
{
    const char* name;
    SORCER_Var var;
    u32 cell;
} SORCER_TxnLoadVarInfo;

typedef vec_t(SORCER_TxnLoadVarInfo) SORCER_TxnLoadVarInfoVec;



typedef struct SORCER_TxnLoadDefInfo
{
    const char* name;
    SORCER_Block block;
} SORCER_TxnLoadDefInfo;

typedef vec_t(SORCER_TxnLoadDefInfo) SORCER_TxnLoadDefInfoVec;




typedef struct SORCER_TxnLoadBlockInfo
{
    SORCER_Block scope;
    const TXN_Node* body;
    u32 bodyLen;
    SORCER_TxnLoadDefInfoVec defTable[1];
    SORCER_TxnLoadVarInfoVec varTable[1];
    bool loaded;
    u32 cellCount;
    vec_u32 dataStack[1];
    u32 numIns;
} SORCER_TxnLoadBlockInfo;

static void SORCER_txnLoadBlockInfoFree(SORCER_TxnLoadBlockInfo* b)
{
    vec_free(b->dataStack);
    vec_free(b->varTable);
    vec_free(b->defTable);
}


typedef vec_t(SORCER_TxnLoadBlockInfo) SORCER_TxnLoadBlockInfoVec;





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
    SORCER_TxnFileInfoVec* fileTable;
    u32 blockBaseId;
    SORCER_TxnLoadBlockInfoVec blockTable[1];
    SORCER_TxnLoadCallStack callStack[1];
} SORCER_TxnLoadContext;

static SORCER_TxnLoadContext SORCER_txnLoadContextNew
(
    SORCER_Context* sorcer, TXN_Space* space,
    const TXN_SpaceSrcInfo* srcInfo, SORCER_TxnErrorInfo* errInfo, SORCER_TxnFileInfoVec* fileTable
)
{
    SORCER_TxnLoadContext ctx[1] = { 0 };
    ctx->sorcer = sorcer;
    ctx->space = space;
    ctx->srcInfo = srcInfo;
    ctx->errInfo = errInfo;
    ctx->fileTable = fileTable;
    ctx->blockBaseId = SORCER_ctxBlocksTotal(sorcer);
    return *ctx;
}

static void SORCER_txnLoadContextFree(SORCER_TxnLoadContext* ctx)
{
    vec_free(ctx->callStack);
    for (u32 i = 0; i < ctx->blockTable->length; ++i)
    {
        SORCER_txnLoadBlockInfoFree(ctx->blockTable->data + i);
    }
    vec_free(ctx->blockTable);
}






static SORCER_TxnLoadBlockInfo* SORCER_txnLoadBlockInfo(SORCER_TxnLoadContext* ctx, SORCER_Block blk)
{
    SORCER_TxnLoadBlockInfoVec* blockTable = ctx->blockTable;
    assert(blk.id - ctx->blockBaseId < blockTable->length);
    return blockTable->data + blk.id - ctx->blockBaseId;
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
        const char* s = SORCER_TxnPrimWordNameTable(k);
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
        const char* s = SORCER_TxnKeyExprHeadNameTable(k);
        if (0 == strcmp(s, name))
        {
            return k;
        }
    }
    return SORCER_TxnKeyExpr_Invalid;
}




static SORCER_TxnLoadVarInfo* SORCER_txnLoadFindVar(SORCER_TxnLoadContext* ctx, const char* name, SORCER_Block scope)
{
    while (scope.id != SORCER_Block_Invalid.id)
    {
        SORCER_TxnLoadBlockInfo* blockInfo = SORCER_txnLoadBlockInfo(ctx, scope);
        for (u32 i = 0; i < blockInfo->varTable->length; ++i)
        {
            SORCER_TxnLoadVarInfo* info = blockInfo->varTable->data + i;
            if (info->name == name)
            {
                return info;
            }
        }
        scope = blockInfo->scope;
    }
    return NULL;
}

static SORCER_TxnLoadDefInfo* SORCER_txnLoadFindDef(SORCER_TxnLoadContext* ctx, const char* name, SORCER_Block scope)
{
    while (scope.id != SORCER_Block_Invalid.id)
    {
        SORCER_TxnLoadBlockInfo* blockInfo = SORCER_txnLoadBlockInfo(ctx, scope);
        for (u32 i = 0; i < blockInfo->defTable->length; ++i)
        {
            SORCER_TxnLoadDefInfo* info = blockInfo->defTable->data + i;
            if (info->name == name)
            {
                return info;
            }
        }
        scope = blockInfo->scope;
    }
    return NULL;
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
    SORCER_TxnLoadBlockInfoVec* blockTable = ctx->blockTable;

    SORCER_TxnLoadBlockInfo info = { scope, body, bodyLen };
    vec_push(blockTable, info);
    return SORCER_blockNew(sorcer);
}


static void SORCER_txnLoadBlocksRollback(SORCER_TxnLoadContext* ctx, u32 n)
{
    SORCER_Context* sorcer = ctx->sorcer;
    SORCER_TxnLoadBlockInfoVec* blockTable = ctx->blockTable;
    for (u32 i = n; i < blockTable->length; ++i)
    {
        SORCER_txnLoadBlockInfoFree(blockTable->data + i);
    }
    vec_resize(blockTable, n);
    SORCER_ctxBlocksRollback(sorcer, n);
}











static SORCER_Block SORCER_txnLoadBlockDefTree(SORCER_TxnLoadContext* ctx)
{
    SORCER_Context* sorcer = ctx->sorcer;
    TXN_Space* space = ctx->space;
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
        const char* name = TXN_tokData(space, elms[0]);
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
                const char* defName = TXN_tokData(space, elms[1]);
                SORCER_Block block;
                {
                    block = SORCER_txnLoadBlockNew(ctx, cur->block, elms + 2, len - 2);
                    SORCER_TxnLoadCallLevel level = { block, elms + 2, len - 2 };
                    vec_push(callStack, level);
                }
                SORCER_TxnLoadDefInfo def = { defName, block };
                SORCER_TxnLoadBlockInfo* info = SORCER_txnLoadBlockInfo(ctx, cur->block);
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
    SORCER_TxnLoadBlockInfo* blkInfo = SORCER_txnLoadBlockInfo(ctx, block);
    assert(!blkInfo->loaded);
    assert(!blkInfo->cellCount);
    assert(!blkInfo->dataStack->length);
    assert(!blkInfo->numIns);
    blkInfo->loaded = true;
}




static void SORCER_txnLoadBlockCellNew(SORCER_TxnLoadContext* ctx, SORCER_Block block)
{
    SORCER_TxnLoadBlockInfo* blkInfo = SORCER_txnLoadBlockInfo(ctx, block);
    u32 cellId = blkInfo->cellCount++;
    vec_push(blkInfo->dataStack, cellId);
}




static SORCER_TxnLoadVarInfo* SORCER_txnLoadFindVarWithCell(SORCER_TxnLoadContext* ctx, SORCER_Block block, u32 cell)
{
    SORCER_TxnLoadBlockInfo* blkInfo = SORCER_txnLoadBlockInfo(ctx, block);
    assert(blkInfo->loaded);
    for (u32 i = 0; i < blkInfo->varTable->length; ++i)
    {
        SORCER_TxnLoadVarInfo* info = blkInfo->varTable->data + i;
        if (info->cell == cell)
        {
            return info;
        }
    }
    return NULL;
}











SORCER_Block SORCER_txnLoadBlock(SORCER_TxnLoadContext* ctx, const TXN_Node* rootBody, u32 rootBodyLen)
{
    SORCER_Context* sorcer = ctx->sorcer;
    TXN_Space* space = ctx->space;
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
                const SORCER_TypeInfo* typeInfo = SORCER_typeInfo(sorcer, type);
                if (!(typeInfo->flags & SORCER_TypeFlag_String))
                {
                    continue;
                }
                SORCER_Cell cell[1] = { 0 };
                const char* str = TXN_tokData(space, node);
                bool r = SORCER_cellNew(sorcer, type, str, cell);
                if (r)
                {
                    SORCER_cellFree(sorcer, cell);
                    SORCER_blockAddInstPushImm(sorcer, cur->block, type, str);
                    SORCER_txnLoadBlockCellNew(ctx, cur->block);
                    goto next;
                }
            }
            SORCER_txnLoadErrorAtNode(ctx, node, SORCER_TxnError_UnkWord);
            goto failed;
        }
        else
        {
            const char* name = TXN_tokData(space, node);
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
            SORCER_TxnLoadVarInfo* varInfo = SORCER_txnLoadFindVar(ctx, name, cur->block);
            if (varInfo)
            {
                SORCER_blockAddInstPushVar(sorcer, cur->block, varInfo->var);
                SORCER_TxnLoadBlockInfo* curBlkInfo = SORCER_txnLoadBlockInfo(ctx, cur->block);
                vec_push(curBlkInfo->dataStack, varInfo->cell);
                goto next;
            }
            SORCER_TxnLoadDefInfo* defInfo = SORCER_txnLoadFindDef(ctx, name, cur->block);
            if (defInfo)
            {
                SORCER_TxnLoadBlockInfo* defBlkInfo = SORCER_txnLoadBlockInfo(ctx, defInfo->block);
                if (!defBlkInfo->loaded)
                {
                    SORCER_txnLoadBlockSetLoaded(ctx, defInfo->block);
                    SORCER_TxnLoadCallLevel level = { defInfo->block, defBlkInfo->body, defBlkInfo->bodyLen };
                    vec_push(callStack, level);
                    cur->p -= 1;
                    goto next;
                }
                SORCER_TxnLoadBlockInfo* curBlkInfo = SORCER_txnLoadBlockInfo(ctx, cur->block);
                u32 dsReduce = min(defBlkInfo->numIns, curBlkInfo->dataStack->length);
                u32 curBlkIns = defBlkInfo->numIns - dsReduce;
                curBlkInfo->numIns += curBlkIns;
                for (u32 i = 0; i < defBlkInfo->numIns; ++i)
                {
                    if (i >= curBlkIns)
                    {
                        u32 cellId = curBlkInfo->dataStack->data[i - curBlkIns];
                        SORCER_TxnLoadVarInfo* var = SORCER_txnLoadFindVarWithCell(ctx, cur->block, cellId);
                        if (var)
                        {
                            u32 rdp = defBlkInfo->numIns - 1 - i;
                            SORCER_blockAddInstDrop(sorcer, cur->block, rdp);
                        }
                    }
                }
                SORCER_blockAddInstCall(sorcer, cur->block, defInfo->block);
                vec_resize(curBlkInfo->dataStack, curBlkInfo->dataStack->length - dsReduce);
                for (u32 i = 0; i < defBlkInfo->dataStack->length; ++i)
                {
                    SORCER_txnLoadBlockCellNew(ctx, cur->block);
                }
                goto next;
            }
            SORCER_Opr opr = SORCER_txnLoadFindOpr(ctx, name);
            if (opr.id != SORCER_Opr_Invalid.id)
            {
                SORCER_blockAddInstOpr(sorcer, cur->block, opr);
                const SORCER_OprInfo* oprInfo = SORCER_oprInfo(sorcer, opr);
                SORCER_TxnLoadBlockInfo* curBlkInfo = SORCER_txnLoadBlockInfo(ctx, cur->block);
                u32 dsReduce = min(oprInfo->numIns, curBlkInfo->dataStack->length);
                u32 curBlkIns = oprInfo->numIns - dsReduce;
                curBlkInfo->numIns += curBlkIns;
                u32 freeMask = 0;
                for (u32 i = 0; i < oprInfo->numIns; ++i)
                {
                    if (i < curBlkIns)
                    {
                        freeMask |= (1U << i);
                    }
                    else
                    {
                        u32 cellId = curBlkInfo->dataStack->data[i - curBlkIns];
                        SORCER_TxnLoadVarInfo* var = SORCER_txnLoadFindVarWithCell(ctx, cur->block, cellId);
                        if (!var)
                        {
                            freeMask |= (1U << i);
                        }
                    }
                }
                SORCER_blockAddInstClean(sorcer, cur->block, freeMask);
                vec_resize(curBlkInfo->dataStack, curBlkInfo->dataStack->length - dsReduce);
                for (u32 i = 0; i < oprInfo->numOuts; ++i)
                {
                    SORCER_txnLoadBlockCellNew(ctx, cur->block);
                }
                goto next;
            }

            for (u32 i = 0; i < numTypes; ++i)
            {
                SORCER_Type type = SORCER_typeByIndex(sorcer, i);
                const SORCER_TypeInfo* typeInfo = SORCER_typeInfo(sorcer, type);
                if (typeInfo->flags & SORCER_TypeFlag_String)
                {
                    continue;
                }
                SORCER_Cell cell[1] = { 0 };
                const char* str = TXN_tokData(space, node);
                bool r = SORCER_cellNew(sorcer, type, str, cell);
                if (r)
                {
                    SORCER_cellFree(sorcer, cell);
                    SORCER_blockAddInstPushImm(sorcer, cur->block, type, str);
                    SORCER_txnLoadBlockCellNew(ctx, cur->block);
                    goto next;
                }
            }
            SORCER_txnLoadErrorAtNode(ctx, node, SORCER_TxnError_UnkWord);
            goto failed;
        }
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
        const char* name = TXN_tokData(space, elms[0]);
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
                    const char* name = TXN_tokData(space, elms[j]);
                    // todo
                    SORCER_Var var = SORCER_blockAddInstPopVar(sorcer, cur->block);

                    SORCER_TxnLoadBlockInfo* curBlkInfo = SORCER_txnLoadBlockInfo(ctx, cur->block);
                    u32 cell = -1;
                    if (curBlkInfo->dataStack->length > 0)
                    {
                        cell = vec_last(curBlkInfo->dataStack);
                        vec_pop(curBlkInfo->dataStack);
                    }
                    else
                    {
                        curBlkInfo->numIns += 1;
                    }

                    SORCER_TxnLoadVarInfo varInfo = { name, var, cell };
                    vec_push(curBlkInfo->varTable, varInfo);
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
    SORCER_Context* ctx, TXN_Space* space, TXN_Node node,
    const TXN_SpaceSrcInfo* srcInfo, SORCER_TxnErrorInfo* errInfo, SORCER_TxnFileInfoVec* fileTable
)
{
    if (fileTable && !fileTable->length)
    {
        SORCER_TxnFileInfo info = { 0 };
        vec_push(fileTable, info);
    }
    const TXN_Node* seq = TXN_seqElm(space, node);
    u32 len = TXN_seqLen(space, node);
    SORCER_TxnLoadContext tctx[1] = { SORCER_txnLoadContextNew(ctx, space, srcInfo, errInfo, fileTable) };
    SORCER_Block r = SORCER_txnLoadBlock(tctx, seq, len);
    SORCER_txnLoadContextFree(tctx);
    return r;
}




SORCER_Block SORCER_blockFromTxnCode
(
    SORCER_Context* ctx, const char* code, SORCER_TxnErrorInfo* errInfo, SORCER_TxnFileInfoVec* fileTable
)
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
    block = SORCER_blockFromTxnNode(ctx, space, root, srcInfo, errInfo, fileTable);
out:
    TXN_spaceSrcInfoFree(srcInfo);
    TXN_spaceFree(space);
    return block;
}




SORCER_Block SORCER_blockFromTxnFile
(
    SORCER_Context* ctx, const char* path, SORCER_TxnErrorInfo* errInfo, SORCER_TxnFileInfoVec* fileTable
)
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
    if (fileTable && !fileTable->length)
    {
        SORCER_TxnFileInfo info = { 0 };
        stzncpy(info.path, path, SORCER_TxnFilePath_MAX);
        vec_push(fileTable, info);
    }
    SORCER_Block blk = SORCER_blockFromTxnCode(ctx, str, errInfo, fileTable);
    free(str);
    return blk;
}

















































































