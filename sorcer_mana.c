#include "sorcer_a.h"
#include "sorcer_mana.h"
#include "sorcer_typeflag.h"
#include <fileu.h>




typedef enum SR_ManaKeyword
{
    SR_ManaKeyword_BlockBegin,
    SR_ManaKeyword_BlockEnd,
    SR_ManaKeyword_VarsBegin,
    SR_ManaKeyword_VarsEnd,
    SR_ManaKeyword_Apply,

    SR_NumManaKeywords
} SR_ManaKeyword;

const char* SR_ManaKeywordNameTable(SR_ManaKeyword w)
{
    assert(w < SR_NumManaKeywords);
    static const char* a[SR_NumManaKeywords] =
    {
        "{",
        "}",
        "[",
        "]",
        "#",
    };
    return a[w];
}





typedef struct SR_ManaLoadVarInfo
{
    u32 nameId;
    SR_Var var;
} SR_ManaLoadVarInfo;

typedef vec_t(SR_ManaLoadVarInfo) SR_ManaLoadVarInfoVec;



typedef struct SR_ManaLoadDefInfo
{
    u32 nameId;
    SR_Block block;
} SR_ManaLoadDefInfo;

typedef vec_t(SR_ManaLoadDefInfo) SR_ManaLoadDefInfoVec;




typedef struct SR_ManaLoadBlockInfo
{
    SR_Block scope;
    u32 begin;
    u32 end;
    SR_ManaLoadDefInfoVec defTable[1];
    SR_ManaLoadVarInfoVec varTable[1];
    bool loaded;
} SR_ManaLoadBlockInfo;

static void SR_manaLoadBlockInfoFree(SR_ManaLoadBlockInfo* b)
{
    vec_free(b->varTable);
    vec_free(b->defTable);
}


typedef vec_t(SR_ManaLoadBlockInfo) SR_ManaLoadBlockInfoVec;



typedef struct SR_ManaLoadCall
{
    SR_Block block;
    u32 defNameId;
    u32 retP;
} SR_ManaLoadCall;

typedef vec_t(SR_ManaLoadCall) SR_ManaLoadCallStack;




typedef struct SR_ManaLoadContext
{
    SR_Context* sorcer;
    MANA_Space* space;
    const MANA_SpaceSrcInfo* srcInfo;
    SR_ManaErrorInfo* errInfo;
    SR_ManaFileInfoVec* fileTable;
    u32 blockBaseId;
    u32 keyword[SR_NumManaKeywords];
    SR_ManaLoadBlockInfoVec blockTable[1];
    SR_ManaLoadCallStack callStack[1];
    u32 p;
    vec_u32 varNameBuf[1];
} SR_ManaLoadContext;

static SR_ManaLoadContext SR_manaLoadContextNew
(
    SR_Context* sorcer, MANA_Space* space,
    const MANA_SpaceSrcInfo* srcInfo, SR_ManaErrorInfo* errInfo, SR_ManaFileInfoVec* fileTable
)
{
    SR_ManaLoadContext ctx[1] = { 0 };
    ctx->sorcer = sorcer;
    ctx->space = space;
    ctx->srcInfo = srcInfo;
    ctx->errInfo = errInfo;
    ctx->fileTable = fileTable;
    ctx->blockBaseId = SR_ctxBlocksTotal(sorcer);

    for (u32 i = 0; i < SR_NumManaKeywords; ++i)
    {
        const char* s = SR_ManaKeywordNameTable(i);
        ctx->keyword[i] = MANA_spaceDataIdByCstr(space, s);
    }
    return *ctx;
}

static void SR_manaLoadContextFree(SR_ManaLoadContext* ctx)
{
    vec_free(ctx->varNameBuf);
    vec_free(ctx->callStack);
    for (u32 i = 0; i < ctx->blockTable->length; ++i)
    {
        SR_manaLoadBlockInfoFree(ctx->blockTable->data + i);
    }
    vec_free(ctx->blockTable);
}






static SR_ManaLoadBlockInfo* SR_manaLoadBlockInfo(SR_ManaLoadContext* ctx, SR_Block blk)
{
    SR_ManaLoadBlockInfoVec* blockTable = ctx->blockTable;
    assert(blk.id - ctx->blockBaseId < blockTable->length);
    return blockTable->data + blk.id - ctx->blockBaseId;
}







static void SR_manaLoadErrorAtTok(SR_ManaLoadContext* ctx, u32 p, SR_ManaError err)
{
    const MANA_SpaceSrcInfo* srcInfo = ctx->srcInfo;
    SR_ManaErrorInfo* errInfo = ctx->errInfo;
    if (errInfo && srcInfo)
    {
        errInfo->error = err;
        const MANA_TokSrcInfo* tokSrcInfo = MANA_tokSrcInfo(srcInfo, p);
        errInfo->file = tokSrcInfo->file;
        errInfo->line = tokSrcInfo->line;
        errInfo->column = tokSrcInfo->column;
    }
}














static SR_ManaLoadVarInfo* SR_manaLoadFindVar(SR_ManaLoadContext* ctx, u32 nameId, SR_Block scope)
{
    while (scope.id != SR_Block_Invalid.id)
    {
        SR_ManaLoadBlockInfo* blockInfo = SR_manaLoadBlockInfo(ctx, scope);
        for (u32 i = 0; i < blockInfo->varTable->length; ++i)
        {
            SR_ManaLoadVarInfo* info = blockInfo->varTable->data + i;
            if (info->nameId == nameId)
            {
                return info;
            }
        }
        scope = blockInfo->scope;
    }
    return NULL;
}

static SR_ManaLoadDefInfo* SR_manaLoadFindDef(SR_ManaLoadContext* ctx, u32 nameId, SR_Block scope)
{
    while (scope.id != SR_Block_Invalid.id)
    {
        SR_ManaLoadBlockInfo* blockInfo = SR_manaLoadBlockInfo(ctx, scope);
        for (u32 i = 0; i < blockInfo->defTable->length; ++i)
        {
            SR_ManaLoadDefInfo* info = blockInfo->defTable->data + i;
            if (info->nameId == nameId)
            {
                return info;
            }
        }
        scope = blockInfo->scope;
    }
    return NULL;
}

static SR_Opr SR_manaLoadFindOpr(SR_ManaLoadContext* ctx, const char* name)
{
    return SR_oprByName(ctx->sorcer, name);
}

static SR_Block SR_manaLoadFindBlock(SR_ManaLoadContext* ctx, u32 begin)
{
    SR_ManaLoadBlockInfoVec* blockTable = ctx->blockTable;
    for (u32 i = 0; i < blockTable->length; ++i)
    {
        SR_ManaLoadBlockInfo* info = blockTable->data + i;
        if (info->begin == begin)
        {
            SR_Block blk = { ctx->blockBaseId + i };
            return blk;
        }
    }
    return SR_Block_Invalid;
}










static SR_Block SR_manaLoadBlockNew(SR_ManaLoadContext* ctx, SR_Block scope, u32 begin)
{
    SR_Context* sorcer = ctx->sorcer;
    SR_ManaLoadBlockInfoVec* blockTable = ctx->blockTable;

    SR_ManaLoadBlockInfo info = { scope, begin, -1 };
    vec_push(blockTable, info);
    return SR_blockNew(sorcer);
}


static void SR_manaLoadBlocksRollback(SR_ManaLoadContext* ctx, u32 n)
{
    SR_Context* sorcer = ctx->sorcer;
    SR_ManaLoadBlockInfoVec* blockTable = ctx->blockTable;
    for (u32 i = n; i < blockTable->length; ++i)
    {
        SR_manaLoadBlockInfoFree(blockTable->data + i);
    }
    vec_resize(blockTable, n);
    SR_ctxBlocksRollback(sorcer, n);
}







static u32 SR_manaLoadDefNameIdHere(MANA_Space* space, u32 p)
{
    const char* str = MANA_tokDataPtr(space, p);
    u32 size = MANA_tokDataSize(space, p);
    if (':' == str[size - 1])
    {
        return MANA_spaceDataIdByBuf(space, str, size - 1);
    }
    else
    {
        return -1;
    }
}






static void SR_manaLoadBlockDefTreeCallback(SR_ManaLoadContext* ctx, u32 p)
{
    SR_ManaLoadCallStack* callStack = ctx->callStack;

    u32 defNameId = vec_last(callStack).defNameId;
    SR_Block curBlock = vec_last(callStack).block;
    SR_ManaLoadBlockInfo* blkInfo = SR_manaLoadBlockInfo(ctx, curBlock);
    assert(-1 == blkInfo->end);
    blkInfo->end = p;
    if (defNameId != -1)
    {
        SR_ManaLoadDefInfo def = { defNameId, curBlock };
        SR_ManaLoadBlockInfo* scopeInfo = SR_manaLoadBlockInfo(ctx, blkInfo->scope);
        vec_push(scopeInfo->defTable, def);
    }
}





static bool SR_manaLoadBlockDefTree(SR_ManaLoadContext* ctx, u32 end)
{
    SR_Context* sorcer = ctx->sorcer;
    MANA_Space* space = ctx->space;
    const u32* keyword = ctx->keyword;
    SR_ManaLoadCallStack* callStack = ctx->callStack;

    u32 callBase = callStack->length;
next:;
    u32 p = ctx->p++;
    if (p == end)
    {
        if (callStack->length == callBase)
        {
            SR_manaLoadBlockDefTreeCallback(ctx, p);
            return true;
        }
        else
        {
            SR_manaLoadErrorAtTok(ctx, p - 1, SR_ManaError_Syntax);
            return false;
        }
    }
    const char* pStr = MANA_tokDataPtr(space, p);
    u32 pStrId = MANA_tokDataId(space, p);
    if (keyword[SR_ManaKeyword_BlockEnd] == pStrId)
    {
        if (callStack->length > callBase)
        {
            SR_manaLoadBlockDefTreeCallback(ctx, p);
            vec_pop(callStack);
            goto next;
        }
        else
        {
            SR_manaLoadErrorAtTok(ctx, p, SR_ManaError_Syntax);
            return false;
        }
    }
    else if (keyword[SR_ManaKeyword_BlockBegin] == pStrId)
    {
        SR_Block scope = vec_last(callStack).block;
        SR_Block block = SR_manaLoadBlockNew(ctx, scope, p);
        SR_ManaLoadCall c = { block, -1 };
        vec_push(callStack, c);
    }
    else
    {
        u32 defNameId = SR_manaLoadDefNameIdHere(space, p);
        if (defNameId != -1)
        {
            u32 p = ctx->p++;
            const char* pStr = MANA_tokDataPtr(space, p);
            u32 pStrId = MANA_tokDataId(space, p);
            SR_Block scope = vec_last(callStack).block;
            SR_Block block;
            if (keyword[SR_ManaKeyword_BlockBegin] == pStrId)
            {
                block = SR_manaLoadBlockNew(ctx, scope, p);
                SR_ManaLoadCall c = { block, defNameId };
                vec_push(callStack, c);
            }
            else
            {
                SR_manaLoadErrorAtTok(ctx, p, SR_ManaError_Syntax);
                return false;
            }
        }
    }
    goto next;
}







static void SR_manaLoadBlockSetLoaded(SR_ManaLoadContext* ctx, SR_Block block)
{
    SR_ManaLoadBlockInfo* blkInfo = SR_manaLoadBlockInfo(ctx, block);
    assert(blkInfo->end != -1);
    assert(!blkInfo->loaded);
    blkInfo->loaded = true;
}










SR_Block SR_manaLoadBlock(SR_ManaLoadContext* ctx, u32 begin, u32 end)
{
    SR_Context* sorcer = ctx->sorcer;
    MANA_Space* space = ctx->space;
    const u32* keyword = ctx->keyword;
    SR_ManaLoadCallStack* callStack = ctx->callStack;
    vec_u32* varNameBuf = ctx->varNameBuf;
    u32 blocksTotal0 = SR_ctxBlocksTotal(sorcer);
    {
        SR_Block blockRoot = SR_manaLoadBlockNew(ctx, SR_Block_Invalid, begin);
        if (blockRoot.id == SR_Block_Invalid.id)
        {
            goto failed;
        }
        SR_ManaLoadCall c = { blockRoot, -1 };
        vec_push(callStack, c);
        ctx->p = begin;
        if (SR_manaLoadBlockDefTree(ctx, end))
        {
            ctx->p = begin;
            SR_manaLoadBlockSetLoaded(ctx, blockRoot);
        }
        else
        {
            goto failed;
        }
    }
    u32 callBase = callStack->length;
next:;
    u32 p = ctx->p++;
    if (p == end)
    {
        if (callStack->length == callBase)
        {
            SR_Block blk = vec_last(callStack).block;
            vec_pop(callStack);
            return blk;
        }
        else
        {
            SR_manaLoadErrorAtTok(ctx, p, SR_ManaError_Syntax);
            return SR_Block_Invalid;
        }
    }
    SR_Block curBlk = vec_last(callStack).block;
    SR_ManaLoadBlockInfo* curBlkInfo = SR_manaLoadBlockInfo(ctx, curBlk);
    const char* pStr = MANA_tokDataPtr(space, p);
    u32 pStrId = MANA_tokDataId(space, p);
    if (keyword[SR_ManaKeyword_BlockEnd] == pStrId)
    {
        assert(curBlkInfo->end == p);
        if (callStack->length > callBase)
        {
            ctx->p = vec_last(callStack).retP;
            vec_pop(callStack);
            goto next;
        }
        else
        {
            SR_manaLoadErrorAtTok(ctx, p, SR_ManaError_Syntax);
            return SR_Block_Invalid;
        }
    }
    else if (keyword[SR_ManaKeyword_BlockBegin] == pStrId)
    {
        SR_Block block = SR_manaLoadFindBlock(ctx, p);
        assert(block.id != SR_Block_Invalid.id);
        SR_ManaLoadBlockInfo* blkInfo = SR_manaLoadBlockInfo(ctx, block);
        if (!blkInfo->loaded)
        {
            SR_manaLoadBlockSetLoaded(ctx, block);
            SR_ManaLoadCall c = { block, -1, ctx->p - 1 };
            vec_push(callStack, c);
            ctx->p = blkInfo->begin + 1;
            goto next;
        }
        assert(curBlk.id == blkInfo->scope.id);
        SR_blockAddInstPushBlock(sorcer, curBlk, block);
        ctx->p = blkInfo->end + 1;
        goto next;
    }
    else if (keyword[SR_ManaKeyword_VarsBegin] == pStrId)
    {
        vec_resize(varNameBuf, 0);
        for (;;)
        {
            p = ctx->p++;
            pStr = MANA_tokDataPtr(space, p);
            pStrId = MANA_tokDataId(space, p);
            if (keyword[SR_ManaKeyword_VarsEnd] == pStrId)
            {
                break;
            }
            vec_push(varNameBuf, pStrId);
        }
        for (u32 i = 0; i < varNameBuf->length; ++i)
        {
            u32 j = varNameBuf->length - 1 - i;
            SR_Var var = SR_blockAddInstPopVar(sorcer, curBlk);
            SR_ManaLoadVarInfo varInfo = { varNameBuf->data[j], var };
            vec_push(curBlkInfo->varTable, varInfo);
        }
        goto next;
    }
    else
    {
        u32 defNameId = SR_manaLoadDefNameIdHere(space, p);
        if (defNameId != -1)
        {
            SR_ManaLoadDefInfo* defInfo = SR_manaLoadFindDef(ctx, defNameId, curBlk);
            assert(defInfo);
            SR_ManaLoadBlockInfo* defBlkInfo = SR_manaLoadBlockInfo(ctx, defInfo->block);
            assert(defBlkInfo);
            assert(defBlkInfo->begin == p + 1);
            ctx->p = defBlkInfo->end;
            if (keyword[SR_ManaKeyword_BlockEnd] == MANA_tokDataId(space, ctx->p))
            {
                ctx->p += 1;
            }
            goto next;
        }
        else
        {
            u32 numTypes = SR_ctxTypesTotal(sorcer);
            if (MANA_tokFlags(space, p) & MANA_TokFlag_Quoted)
            {
                for (u32 i = 0; i < numTypes; ++i)
                {
                    SR_Type type = SR_typeByIndex(sorcer, i);
                    const SR_TypeInfo* typeInfo = SR_typeInfo(sorcer, type);
                    if (!(typeInfo->flags & SR_TypeFlag_String))
                    {
                        continue;
                    }
                    SR_Cell cell[1] = { 0 };
                    const char* str = MANA_tokDataPtr(space, p);
                    bool r = SR_cellNew(sorcer, type, str, cell);
                    if (r)
                    {
                        SR_cellFree(sorcer, cell);
                        SR_blockAddInstPushImm(sorcer, curBlk, type, str);
                        goto next;
                    }
                }
                SR_manaLoadErrorAtTok(ctx, p, SR_ManaError_UnkWord);
                goto failed;
            }
            else
            {
                if (keyword[SR_ManaKeyword_Apply] == pStrId)
                {
                    SR_blockAddInstApply(sorcer, curBlk);
                    goto next;
                }
                SR_ManaLoadVarInfo* varInfo = SR_manaLoadFindVar(ctx, pStrId, curBlk);
                if (varInfo)
                {
                    SR_blockAddInstPushVar(sorcer, curBlk, varInfo->var);
                    goto next;
                }
                SR_ManaLoadDefInfo* defInfo = SR_manaLoadFindDef(ctx, pStrId, curBlk);
                if (defInfo)
                {
                    SR_ManaLoadBlockInfo* defBlkInfo = SR_manaLoadBlockInfo(ctx, defInfo->block);
                    if (!defBlkInfo->loaded)
                    {
                        SR_manaLoadBlockSetLoaded(ctx, defInfo->block);
                        SR_ManaLoadCall c = { defInfo->block, -1, ctx->p - 1 };
                        vec_push(callStack, c);
                        ctx->p = defBlkInfo->begin + 1;
                        assert(keyword[SR_ManaKeyword_BlockBegin] == MANA_tokDataId(space, defBlkInfo->begin));
                        goto next;
                    }
                    SR_blockAddInstCall(sorcer, curBlk, defInfo->block);
                    goto next;
                }
                SR_Opr opr = SR_manaLoadFindOpr(ctx, pStr);
                if (opr.id != SR_Opr_Invalid.id)
                {
                    SR_blockAddInstOpr(sorcer, curBlk, opr);
                    goto next;
                }

                for (u32 i = 0; i < numTypes; ++i)
                {
                    SR_Type type = SR_typeByIndex(sorcer, i);
                    const SR_TypeInfo* typeInfo = SR_typeInfo(sorcer, type);
                    if (typeInfo->flags & SR_TypeFlag_String)
                    {
                        continue;
                    }
                    SR_Cell cell[1] = { 0 };
                    const char* str = MANA_tokDataPtr(space, p);
                    bool r = SR_cellNew(sorcer, type, str, cell);
                    if (r)
                    {
                        SR_cellFree(sorcer, cell);
                        SR_blockAddInstPushImm(sorcer, curBlk, type, str);
                        goto next;
                    }
                }
                SR_manaLoadErrorAtTok(ctx, p, SR_ManaError_UnkWord);
                goto failed;
            }
        }
        SR_manaLoadErrorAtTok(ctx, p, SR_ManaError_Syntax);
    }
failed:
    SR_manaLoadBlocksRollback(ctx, blocksTotal0);
    return SR_Block_Invalid;
}






SR_Block SR_blockFromManaToks
(
    SR_Context* ctx, MANA_Space* space, u32 begin, u32 end,
    const MANA_SpaceSrcInfo* srcInfo, SR_ManaErrorInfo* errInfo, SR_ManaFileInfoVec* fileTable
)
{
    if (fileTable && !fileTable->length)
    {
        SR_ManaFileInfo info = { 0 };
        vec_push(fileTable, info);
    }
    SR_ManaLoadContext tctx[1] = { SR_manaLoadContextNew(ctx, space, srcInfo, errInfo, fileTable) };
    SR_Block r = SR_manaLoadBlock(tctx, begin, end);
    SR_manaLoadContextFree(tctx);
    return r;
}




SR_Block SR_blockFromManaStr
(
    SR_Context* ctx, const char* str, SR_ManaErrorInfo* errInfo, SR_ManaFileInfoVec* fileTable
)
{
    SR_Block block = SR_Block_Invalid;
    MANA_Space* space = MANA_spaceNew();
    MANA_SpaceSrcInfo* srcInfo = MANA_spaceSrcInfoNew();
    MANA_LexingOpt opt[1] = { { "()[]{};," } };
    MANA_lexing(space, str, opt, srcInfo);
    block = SR_blockFromManaToks(ctx, space, 0, MANA_spaceToksTotal(space), srcInfo, errInfo, fileTable);
    MANA_spaceSrcInfoFree(srcInfo);
    MANA_spaceFree(space);
    return block;
}




SR_Block SR_blockFromManaFile
(
    SR_Context* ctx, const char* path, SR_ManaErrorInfo* errInfo, SR_ManaFileInfoVec* fileTable
)
{
    if (fileTable && !fileTable->length)
    {
        SR_ManaFileInfo info = { 0 };
        stzncpy(info.path, path, SR_ManaFilePath_MAX);
        vec_push(fileTable, info);
    }
    char* str;
    u32 strSize = FILEU_readFile(path, &str);
    if (-1 == strSize)
    {
        errInfo->error = SR_ManaError_SrcFile;
        errInfo->file = 0;
        errInfo->line = 0;
        errInfo->column = 0;
        return SR_Block_Invalid;
    }
    SR_Block blk = SR_blockFromManaStr(ctx, str, errInfo, fileTable);
    free(str);
    return blk;
}

















































































