#include "sorcer_a.h"
#include "sorcer_mana.h"
#include "sorcer_typeflag.h"
#include <fileu.h>




typedef enum SORCER_ManaPrimWord
{
    SORCER_ManaPrimWord_Invalid = -1,

    SORCER_ManaPrimWord_Apply,

    SORCER_NumManaPrimWords
} SORCER_ManaPrimWord;

const char* SORCER_ManaPrimWordNameTable(SORCER_ManaPrimWord w)
{
    assert(w < SORCER_NumManaPrimWords);
    static const char* a[SORCER_NumManaPrimWords] =
    {
        "#",
    };
    return a[w];
}




typedef enum SORCER_ManaKeyExpr
{
    SORCER_ManaKeyExpr_Invalid = -1,

    SORCER_ManaKeyExpr_Def,

    SORCER_NumManaKeyExprs
} SORCER_ManaKeyExpr;

const char* SORCER_ManaKeyExprHeadNameTable(SORCER_ManaKeyExpr expr)
{
    assert(expr < SORCER_NumManaKeyExprs);
    static const char* a[SORCER_NumManaKeyExprs] =
    {
        "def",
    };
    return a[expr];
}




typedef struct SORCER_ManaLoadVarInfo
{
    const char* name;
    SORCER_Var var;
} SORCER_ManaLoadVarInfo;

typedef vec_t(SORCER_ManaLoadVarInfo) SORCER_ManaLoadVarInfoVec;



typedef struct SORCER_ManaLoadDefInfo
{
    const char* name;
    SORCER_Block block;
} SORCER_ManaLoadDefInfo;

typedef vec_t(SORCER_ManaLoadDefInfo) SORCER_ManaLoadDefInfoVec;




typedef struct SORCER_ManaLoadBlockInfo
{
    SORCER_Block scope;
    const MANA_Node* body;
    u32 bodyLen;
    SORCER_ManaLoadDefInfoVec defTable[1];
    SORCER_ManaLoadVarInfoVec varTable[1];
    bool loaded;
} SORCER_ManaLoadBlockInfo;

static void SORCER_manaLoadBlockInfoFree(SORCER_ManaLoadBlockInfo* b)
{
    vec_free(b->varTable);
    vec_free(b->defTable);
}


typedef vec_t(SORCER_ManaLoadBlockInfo) SORCER_ManaLoadBlockInfoVec;





typedef struct SORCER_ManaLoadCallLevel
{
    SORCER_Block block;
    const MANA_Node* seq;
    u32 len;
    u32 p;
} SORCER_ManaLoadCallLevel;

typedef vec_t(SORCER_ManaLoadCallLevel) SORCER_ManaLoadCallStack;





typedef struct SORCER_ManaLoadContext
{
    SORCER_Context* sorcer;
    MANA_Space* space;
    const MANA_SpaceSrcInfo* srcInfo;
    SORCER_ManaErrorInfo* errInfo;
    SORCER_ManaFileInfoVec* fileTable;
    u32 blockBaseId;
    SORCER_ManaLoadBlockInfoVec blockTable[1];
    SORCER_ManaLoadCallStack callStack[1];
} SORCER_ManaLoadContext;

static SORCER_ManaLoadContext SORCER_manaLoadContextNew
(
    SORCER_Context* sorcer, MANA_Space* space,
    const MANA_SpaceSrcInfo* srcInfo, SORCER_ManaErrorInfo* errInfo, SORCER_ManaFileInfoVec* fileTable
)
{
    SORCER_ManaLoadContext ctx[1] = { 0 };
    ctx->sorcer = sorcer;
    ctx->space = space;
    ctx->srcInfo = srcInfo;
    ctx->errInfo = errInfo;
    ctx->fileTable = fileTable;
    ctx->blockBaseId = SORCER_ctxBlocksTotal(sorcer);
    return *ctx;
}

static void SORCER_manaLoadContextFree(SORCER_ManaLoadContext* ctx)
{
    vec_free(ctx->callStack);
    for (u32 i = 0; i < ctx->blockTable->length; ++i)
    {
        SORCER_manaLoadBlockInfoFree(ctx->blockTable->data + i);
    }
    vec_free(ctx->blockTable);
}






static SORCER_ManaLoadBlockInfo* SORCER_manaLoadBlockInfo(SORCER_ManaLoadContext* ctx, SORCER_Block blk)
{
    SORCER_ManaLoadBlockInfoVec* blockTable = ctx->blockTable;
    assert(blk.id - ctx->blockBaseId < blockTable->length);
    return blockTable->data + blk.id - ctx->blockBaseId;
}







static void SORCER_manaLoadErrorAtNode(SORCER_ManaLoadContext* ctx, MANA_Node node, SORCER_ManaError err)
{
    const MANA_SpaceSrcInfo* srcInfo = ctx->srcInfo;
    SORCER_ManaErrorInfo* errInfo = ctx->errInfo;
    if (errInfo && srcInfo)
    {
        errInfo->error = err;
        const MANA_NodeSrcInfo* nodeSrcInfo = MANA_nodeSrcInfo(srcInfo, node);
        errInfo->file = nodeSrcInfo->file;
        errInfo->line = nodeSrcInfo->line;
        errInfo->column = nodeSrcInfo->column;
    }
}









static SORCER_ManaPrimWord SORCER_manaPrimWordFromName(const char* name)
{
    for (SORCER_ManaPrimWord i = 0; i < SORCER_NumManaPrimWords; ++i)
    {
        SORCER_ManaPrimWord k = SORCER_NumManaPrimWords - 1 - i;
        const char* s = SORCER_ManaPrimWordNameTable(k);
        if (0 == strcmp(s, name))
        {
            return k;
        }
    }
    return SORCER_ManaPrimWord_Invalid;
}


static SORCER_ManaKeyExpr SORCER_manaKeyExprFromHeadName(const char* name)
{
    for (SORCER_ManaKeyExpr i = 0; i < SORCER_NumManaKeyExprs; ++i)
    {
        SORCER_ManaKeyExpr k = SORCER_NumManaKeyExprs - 1 - i;
        const char* s = SORCER_ManaKeyExprHeadNameTable(k);
        if (0 == strcmp(s, name))
        {
            return k;
        }
    }
    return SORCER_ManaKeyExpr_Invalid;
}




static SORCER_ManaLoadVarInfo* SORCER_manaLoadFindVar(SORCER_ManaLoadContext* ctx, const char* name, SORCER_Block scope)
{
    while (scope.id != SORCER_Block_Invalid.id)
    {
        SORCER_ManaLoadBlockInfo* blockInfo = SORCER_manaLoadBlockInfo(ctx, scope);
        for (u32 i = 0; i < blockInfo->varTable->length; ++i)
        {
            SORCER_ManaLoadVarInfo* info = blockInfo->varTable->data + i;
            if (info->name == name)
            {
                return info;
            }
        }
        scope = blockInfo->scope;
    }
    return NULL;
}

static SORCER_ManaLoadDefInfo* SORCER_manaLoadFindDef(SORCER_ManaLoadContext* ctx, const char* name, SORCER_Block scope)
{
    while (scope.id != SORCER_Block_Invalid.id)
    {
        SORCER_ManaLoadBlockInfo* blockInfo = SORCER_manaLoadBlockInfo(ctx, scope);
        for (u32 i = 0; i < blockInfo->defTable->length; ++i)
        {
            SORCER_ManaLoadDefInfo* info = blockInfo->defTable->data + i;
            if (info->name == name)
            {
                return info;
            }
        }
        scope = blockInfo->scope;
    }
    return NULL;
}

static SORCER_Opr SORCER_manaLoadFindOpr(SORCER_ManaLoadContext* ctx, const char* name)
{
    return SORCER_oprByName(ctx->sorcer, name);
}





static bool SORCER_manaLoadCheckCall(MANA_Space* space, MANA_Node node)
{
    if (!MANA_nodeIsSeqRound(space, node))
    {
        return false;
    }
    u32 len = MANA_seqLen(space, node);
    if (!len)
    {
        return false;
    }
    const MANA_Node* elms = MANA_seqElm(space, node);
    if (!MANA_nodeIsTok(space, elms[0]))
    {
        return false;
    }
    if (MANA_tokQuoted(space, elms[0]))
    {
        return false;
    }
    return true;
}













static SORCER_Block SORCER_manaLoadBlockNew(SORCER_ManaLoadContext* ctx, SORCER_Block scope, const MANA_Node* body, u32 bodyLen)
{
    SORCER_Context* sorcer = ctx->sorcer;
    SORCER_ManaLoadBlockInfoVec* blockTable = ctx->blockTable;

    SORCER_ManaLoadBlockInfo info = { scope, body, bodyLen };
    vec_push(blockTable, info);
    return SORCER_blockNew(sorcer);
}


static void SORCER_manaLoadBlocksRollback(SORCER_ManaLoadContext* ctx, u32 n)
{
    SORCER_Context* sorcer = ctx->sorcer;
    SORCER_ManaLoadBlockInfoVec* blockTable = ctx->blockTable;
    for (u32 i = n; i < blockTable->length; ++i)
    {
        SORCER_manaLoadBlockInfoFree(blockTable->data + i);
    }
    vec_resize(blockTable, n);
    SORCER_ctxBlocksRollback(sorcer, n);
}











static SORCER_Block SORCER_manaLoadBlockDefTree(SORCER_ManaLoadContext* ctx)
{
    SORCER_Context* sorcer = ctx->sorcer;
    MANA_Space* space = ctx->space;
    SORCER_ManaLoadCallStack* callStack = ctx->callStack;

    u32 begin = callStack->length;
    SORCER_ManaLoadCallLevel* cur = NULL;
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
    MANA_Node node = cur->seq[cur->p++];
    if (SORCER_manaLoadCheckCall(space, node))
    {
        const MANA_Node* elms = MANA_seqElm(space, node);
        u32 len = MANA_seqLen(space, node);
        if (len < 1)
        {
            SORCER_manaLoadErrorAtNode(ctx, node, SORCER_ManaError_Syntax);
            return SORCER_Block_Invalid;
        }
        if (!MANA_nodeIsTok(space, elms[0]))
        {
            SORCER_manaLoadErrorAtNode(ctx, elms[0], SORCER_ManaError_Syntax);
            return SORCER_Block_Invalid;
        }
        const char* defName = MANA_tokData(space, elms[0]);
        SORCER_Block block;
        {
            block = SORCER_manaLoadBlockNew(ctx, cur->block, elms + 1, len - 1);
            SORCER_ManaLoadCallLevel level = { block, elms + 1, len - 1 };
            vec_push(callStack, level);
        }
        SORCER_ManaLoadDefInfo def = { defName, block };
        SORCER_ManaLoadBlockInfo* info = SORCER_manaLoadBlockInfo(ctx, cur->block);
        vec_push(info->defTable, def);
    }
    goto next;
}







static void SORCER_manaLoadBlockSetLoaded(SORCER_ManaLoadContext* ctx, SORCER_Block block)
{
    SORCER_ManaLoadBlockInfo* blkInfo = SORCER_manaLoadBlockInfo(ctx, block);
    assert(!blkInfo->loaded);
    blkInfo->loaded = true;
}










SORCER_Block SORCER_manaLoadBlock(SORCER_ManaLoadContext* ctx, const MANA_Node* rootBody, u32 rootBodyLen)
{
    SORCER_Context* sorcer = ctx->sorcer;
    MANA_Space* space = ctx->space;
    SORCER_ManaLoadCallStack* callStack = ctx->callStack;
    u32 blocksTotal0 = SORCER_ctxBlocksTotal(sorcer);
    {
        SORCER_Block blockRoot = SORCER_manaLoadBlockNew(ctx, SORCER_Block_Invalid, rootBody, rootBodyLen);
        if (blockRoot.id == SORCER_Block_Invalid.id)
        {
            goto failed;
        }
        SORCER_ManaLoadCallLevel root = { blockRoot, rootBody, rootBodyLen };
        vec_push(callStack, root);
        SORCER_manaLoadBlockDefTree(ctx);
        SORCER_manaLoadBlockSetLoaded(ctx, blockRoot);
    }
    u32 begin = callStack->length;
    SORCER_ManaLoadCallLevel* cur = NULL;
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
    MANA_Node node = cur->seq[cur->p++];
    if (MANA_nodeIsTok(space, node))
    {
        u32 numTypes = SORCER_ctxTypesTotal(sorcer);
        if (MANA_tokQuoted(space, node))
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
                const char* str = MANA_tokData(space, node);
                bool r = SORCER_cellNew(sorcer, type, str, cell);
                if (r)
                {
                    SORCER_cellFree(sorcer, cell);
                    SORCER_blockAddInstPushImm(sorcer, cur->block, type, str);
                    goto next;
                }
            }
            SORCER_manaLoadErrorAtNode(ctx, node, SORCER_ManaError_UnkWord);
            goto failed;
        }
        else
        {
            const char* name = MANA_tokData(space, node);
            SORCER_ManaPrimWord primWord = SORCER_manaPrimWordFromName(name);
            if (primWord != SORCER_ManaPrimWord_Invalid)
            {
                switch (primWord)
                {
                case SORCER_ManaPrimWord_Apply:
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
            SORCER_ManaLoadVarInfo* varInfo = SORCER_manaLoadFindVar(ctx, name, cur->block);
            if (varInfo)
            {
                SORCER_blockAddInstPushVar(sorcer, cur->block, varInfo->var);
                goto next;
            }
            SORCER_ManaLoadDefInfo* defInfo = SORCER_manaLoadFindDef(ctx, name, cur->block);
            if (defInfo)
            {
                SORCER_ManaLoadBlockInfo* defBlkInfo = SORCER_manaLoadBlockInfo(ctx, defInfo->block);
                if (!defBlkInfo->loaded)
                {
                    SORCER_manaLoadBlockSetLoaded(ctx, defInfo->block);
                    SORCER_ManaLoadCallLevel level = { defInfo->block, defBlkInfo->body, defBlkInfo->bodyLen };
                    vec_push(callStack, level);
                    cur->p -= 1;
                    goto next;
                }
                SORCER_blockAddInstCall(sorcer, cur->block, defInfo->block);
                goto next;
            }
            SORCER_Opr opr = SORCER_manaLoadFindOpr(ctx, name);
            if (opr.id != SORCER_Opr_Invalid.id)
            {
                SORCER_blockAddInstOpr(sorcer, cur->block, opr);
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
                const char* str = MANA_tokData(space, node);
                bool r = SORCER_cellNew(sorcer, type, str, cell);
                if (r)
                {
                    SORCER_cellFree(sorcer, cell);
                    SORCER_blockAddInstPushImm(sorcer, cur->block, type, str);
                    goto next;
                }
            }
            SORCER_manaLoadErrorAtNode(ctx, node, SORCER_ManaError_UnkWord);
            goto failed;
        }
    }
    else if (MANA_nodeIsSeqSquare(space, node))
    {
        const MANA_Node* body = MANA_seqElm(space, node);
        u32 bodyLen = MANA_seqLen(space, node);
        SORCER_Block block = SORCER_manaLoadBlockNew(ctx, cur->block, body, bodyLen);
        if (block.id == SORCER_Block_Invalid.id)
        {
            goto failed;
        }
        SORCER_blockAddInstPushBlock(sorcer, cur->block, block);
        SORCER_ManaLoadCallLevel level = { block, body, bodyLen };
        vec_push(callStack, level);
        SORCER_manaLoadBlockDefTree(ctx);
        SORCER_manaLoadBlockSetLoaded(ctx, block);
        goto next;
    }
    else if (MANA_nodeIsSeqCurly(space, node))
    {
        const MANA_Node* elms = MANA_seqElm(space, node);
        u32 len = MANA_seqLen(space, node);
        SORCER_ManaLoadBlockInfo* curBlkInfo = SORCER_manaLoadBlockInfo(ctx, cur->block);
        for (u32 i = 0; i < len; ++i)
        {
            u32 j = len - 1 - i;
            const char* name = MANA_tokData(space, elms[j]);
            SORCER_Var var = SORCER_blockAddInstPopVar(sorcer, cur->block);
            SORCER_ManaLoadVarInfo varInfo = { name, var };
            vec_push(curBlkInfo->varTable, varInfo);
        }
        goto next;
    }
    else if (SORCER_manaLoadCheckCall(space, node))
    {
        goto next;
    }
    SORCER_manaLoadErrorAtNode(ctx, node, SORCER_ManaError_Syntax);
failed:
    SORCER_manaLoadBlocksRollback(ctx, blocksTotal0);
    return SORCER_Block_Invalid;
}






SORCER_Block SORCER_blockFromManaNode
(
    SORCER_Context* ctx, MANA_Space* space, MANA_Node node,
    const MANA_SpaceSrcInfo* srcInfo, SORCER_ManaErrorInfo* errInfo, SORCER_ManaFileInfoVec* fileTable
)
{
    if (fileTable && !fileTable->length)
    {
        SORCER_ManaFileInfo info = { 0 };
        vec_push(fileTable, info);
    }
    const MANA_Node* seq = MANA_seqElm(space, node);
    u32 len = MANA_seqLen(space, node);
    SORCER_ManaLoadContext tctx[1] = { SORCER_manaLoadContextNew(ctx, space, srcInfo, errInfo, fileTable) };
    SORCER_Block r = SORCER_manaLoadBlock(tctx, seq, len);
    SORCER_manaLoadContextFree(tctx);
    return r;
}




SORCER_Block SORCER_blockFromManaCode
(
    SORCER_Context* ctx, const char* code, SORCER_ManaErrorInfo* errInfo, SORCER_ManaFileInfoVec* fileTable
)
{
    SORCER_Block block = SORCER_Block_Invalid;
    MANA_Space* space = MANA_spaceNew();
    MANA_SpaceSrcInfo srcInfo[1] = { 0 };
    MANA_Node root = MANA_parseAsList(space, code, srcInfo);
    if (MANA_Node_Invalid.id == root.id)
    {
        errInfo->error = SORCER_ManaError_TxnSyntex;
        const MANA_NodeSrcInfo* nodeSrcInfo = &vec_last(srcInfo->nodes);
        errInfo->file = nodeSrcInfo->file;
        errInfo->line = nodeSrcInfo->line;
        errInfo->column = nodeSrcInfo->column;
        goto out;
    }
    block = SORCER_blockFromManaNode(ctx, space, root, srcInfo, errInfo, fileTable);
out:
    MANA_spaceSrcInfoFree(srcInfo);
    MANA_spaceFree(space);
    return block;
}




SORCER_Block SORCER_blockFromManaFile
(
    SORCER_Context* ctx, const char* path, SORCER_ManaErrorInfo* errInfo, SORCER_ManaFileInfoVec* fileTable
)
{
    char* str;
    u32 strSize = FILEU_readFile(path, &str);
    if (-1 == strSize)
    {
        errInfo->error = SORCER_ManaError_SrcFile;
        errInfo->file = 0;
        errInfo->line = 0;
        errInfo->column = 0;
        return SORCER_Block_Invalid;
    }
    if (fileTable && !fileTable->length)
    {
        SORCER_ManaFileInfo info = { 0 };
        stzncpy(info.path, path, SORCER_ManaFilePath_MAX);
        vec_push(fileTable, info);
    }
    SORCER_Block blk = SORCER_blockFromManaCode(ctx, str, errInfo, fileTable);
    free(str);
    return blk;
}

















































































