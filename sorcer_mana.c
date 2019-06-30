#include "sorcer_a.h"
#include "sorcer_mana.h"
#include "sorcer_typeflag.h"
#include <fileu.h>




typedef enum SORCER_ManaKeyword
{
    SORCER_ManaKeyword_Invalid = -1,

    SORCER_ManaKeyword_BlockBegin,
    SORCER_ManaKeyword_BlockEnd,
    SORCER_ManaKeyword_VarsBegin,
    SORCER_ManaKeyword_VarsEnd,
    SORCER_ManaKeyword_Apply,

    SORCER_NumManaKeywords
} SORCER_ManaKeyword;

const char* SORCER_ManaKeywordNameTable(SORCER_ManaKeyword w)
{
    assert(w < SORCER_NumManaKeywords);
    static const char* a[SORCER_NumManaKeywords] =
    {
        "[",
        "]",
        "{",
        "}",
        "#",
    };
    return a[w];
}





typedef struct SORCER_ManaLoadVarInfo
{
    u32 nameId;
    SORCER_Var var;
} SORCER_ManaLoadVarInfo;

typedef vec_t(SORCER_ManaLoadVarInfo) SORCER_ManaLoadVarInfoVec;



typedef struct SORCER_ManaLoadDefInfo
{
    u32 nameId;
    SORCER_Block block;
} SORCER_ManaLoadDefInfo;

typedef vec_t(SORCER_ManaLoadDefInfo) SORCER_ManaLoadDefInfoVec;




typedef struct SORCER_ManaLoadBlockInfo
{
    SORCER_Block scope;
    u32 begin;
    u32 end;
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
    u32 keyword[SORCER_NumManaKeywords];
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

    for (u32 i = 0; i < SORCER_NumManaKeywords; ++i)
    {
        const char* s = SORCER_ManaKeywordNameTable(i);
        ctx->keyword[i] = MANA_spaceDataIdByCstr(space, s);
    }
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







static void SORCER_manaLoadErrorAtTok(SORCER_ManaLoadContext* ctx, u32 tokId, SORCER_ManaError err)
{
    const MANA_SpaceSrcInfo* srcInfo = ctx->srcInfo;
    SORCER_ManaErrorInfo* errInfo = ctx->errInfo;
    if (errInfo && srcInfo)
    {
        errInfo->error = err;
        const MANA_TokSrcInfo* tokSrcInfo = MANA_tokSrcInfo(srcInfo, tokId);
        errInfo->file = tokSrcInfo->file;
        errInfo->line = tokSrcInfo->line;
        errInfo->column = tokSrcInfo->column;
    }
}









static SORCER_ManaKeyword SORCER_manaKeywordMatchDataId(SORCER_ManaLoadContext* ctx, const u32 dataId)
{
    for (SORCER_ManaKeyword i = 0; i < SORCER_NumManaKeywords; ++i)
    {
        SORCER_ManaKeyword k = SORCER_NumManaKeywords - 1 - i;
        if (dataId == ctx->keyword[k])
        {
            return k;
        }
    }
    return SORCER_ManaKeyword_Invalid;
}




static SORCER_ManaLoadVarInfo* SORCER_manaLoadFindVar(SORCER_ManaLoadContext* ctx, u32 nameId, SORCER_Block scope)
{
    while (scope.id != SORCER_Block_Invalid.id)
    {
        SORCER_ManaLoadBlockInfo* blockInfo = SORCER_manaLoadBlockInfo(ctx, scope);
        for (u32 i = 0; i < blockInfo->varTable->length; ++i)
        {
            SORCER_ManaLoadVarInfo* info = blockInfo->varTable->data + i;
            if (info->nameId == nameId)
            {
                return info;
            }
        }
        scope = blockInfo->scope;
    }
    return NULL;
}

static SORCER_ManaLoadDefInfo* SORCER_manaLoadFindDef(SORCER_ManaLoadContext* ctx, u32 nameId, SORCER_Block scope)
{
    while (scope.id != SORCER_Block_Invalid.id)
    {
        SORCER_ManaLoadBlockInfo* blockInfo = SORCER_manaLoadBlockInfo(ctx, scope);
        for (u32 i = 0; i < blockInfo->defTable->length; ++i)
        {
            SORCER_ManaLoadDefInfo* info = blockInfo->defTable->data + i;
            if (info->nameId == nameId)
            {
                return info;
            }
        }
        scope = blockInfo->scope;
    }
    return NULL;
}

static SORCER_Opr SORCER_manaLoadFindOpr(SORCER_ManaLoadContext* ctx, u32 nameId)
{
    const char* name = MANA_spaceDataById(ctx->space, nameId);
    return SORCER_oprByName(ctx->sorcer, name);
}












static SORCER_Block SORCER_manaLoadBlockNew(SORCER_ManaLoadContext* ctx, SORCER_Block scope, u32 begin, u32 end)
{
    SORCER_Context* sorcer = ctx->sorcer;
    SORCER_ManaLoadBlockInfoVec* blockTable = ctx->blockTable;

    SORCER_ManaLoadBlockInfo info = { scope, begin, end };
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







static u32 SORCER_manaLoadDefNameByTokId(MANA_Space* space, u32 tokId)
{
    const char* str = MANA_tokData(space, tokId);
    u32 size = MANA_tokDataSize(space, tokId);
    if (':' == str[size - 1])
    {
        u32 dataId = MANA_spaceDataIdByBuf(space, str, size - 1);
        return dataId;
    }
    else
    {
        return -1;
    }
}








static SORCER_Block SORCER_manaLoadBlockDefTree(SORCER_ManaLoadContext* ctx)
{
    SORCER_Context* sorcer = ctx->sorcer;
    MANA_Space* space = ctx->space;
    SORCER_ManaLoadCallStack* callStack = ctx->callStack;

    u32 callBase = callStack->length;
    SORCER_ManaLoadCallLevel* curLvl = NULL;
next:
    curLvl = &vec_last(callStack);
    u32 p = curLvl->p++;
    u32 tokStr = MANA_tokDataId(space, p);
    if (SORCER_ManaKeyword_BlockEnd == SORCER_manaKeywordMatchDataId(ctx, tokStr))
    {
        if (callStack->length > callBase)
        {
            vec_pop(callStack);
            goto next;
        }
        else
        {
            curLvl->p = 0;
            return curLvl->block;
        }
    }
    else
    {
        u32 defNameId = SORCER_manaLoadDefNameByTokId(space, p);
        if (defNameId != -1)
        {
            u32 p = curLvl->p++;
            SORCER_Block block;
            if (SORCER_ManaKeyword_BlockBegin == SORCER_manaKeywordMatchDataId(ctx, tokStr))
            {
                block = SORCER_manaLoadBlockNew(ctx, curLvl->block, p, -1);
            }
            else if
                ((SORCER_manaKeywordMatchDataId(ctx, tokStr) == SORCER_ManaKeyword_Apply) ||
                 (SORCER_manaKeywordMatchDataId(ctx, tokStr) == SORCER_ManaKeyword_Invalid))
            {
                block = SORCER_manaLoadBlockNew(ctx, curLvl->block, p, p + 1);
            }
            else
            {
                SORCER_manaLoadErrorAtTok(ctx, p, SORCER_ManaError_Syntax);
                return SORCER_Block_Invalid;
            }
            SORCER_ManaLoadDefInfo def = { defNameId, block };
            SORCER_ManaLoadBlockInfo* scopeInfo = SORCER_manaLoadBlockInfo(ctx, curLvl->block);
            vec_push(scopeInfo->defTable, def);
        }
    }
    goto next;
}







static void SORCER_manaLoadBlockSetLoaded(SORCER_ManaLoadContext* ctx, SORCER_Block block)
{
    SORCER_ManaLoadBlockInfo* blkInfo = SORCER_manaLoadBlockInfo(ctx, block);
    assert(!blkInfo->loaded);
    blkInfo->loaded = true;
}










SORCER_Block SORCER_manaLoadBlock(SORCER_ManaLoadContext* ctx, u32 begin, u32 end)
{
    SORCER_Context* sorcer = ctx->sorcer;
    MANA_Space* space = ctx->space;
    SORCER_ManaLoadCallStack* callStack = ctx->callStack;
    u32 blocksTotal0 = SORCER_ctxBlocksTotal(sorcer);
    {
        SORCER_Block blockRoot = SORCER_manaLoadBlockNew(ctx, SORCER_Block_Invalid, begin, end);
        if (blockRoot.id == SORCER_Block_Invalid.id)
        {
            goto failed;
        }
        SORCER_ManaLoadCallLevel root = { blockRoot, begin };
        vec_push(callStack, root);
        SORCER_manaLoadBlockDefTree(ctx);
        SORCER_manaLoadBlockSetLoaded(ctx, blockRoot);
    }
    u32 callBase = callStack->length;
    SORCER_ManaLoadCallLevel* curLvl = NULL;
next:
    curLvl = &vec_last(callStack);
    u32 p = curLvl->p++;
    if (p == end)
    {
        if (callStack->length == callBase)
        {
            SORCER_Block blk = curLvl->block;
            vec_pop(callStack);
            return blk;
        }
        else
        {
            SORCER_manaLoadErrorAtTok(ctx, p, SORCER_ManaError_Syntax);
            return SORCER_Block_Invalid;
        }
    }
    u32 tokStr = MANA_tokDataId(space, p);
    if (SORCER_ManaKeyword_BlockEnd == SORCER_manaKeywordMatchDataId(ctx, tokStr))
    {
        if (callStack->length > callBase)
        {
            vec_pop(callStack);
            goto next;
        }
        else
        {
            SORCER_manaLoadErrorAtTok(ctx, p, SORCER_ManaError_Syntax);
            return SORCER_Block_Invalid;
        }
    }
    u32 defNameId = SORCER_manaLoadDefNameByTokId(space, p);
    if (defNameId != -1)
    {
        SORCER_ManaLoadDefInfo* defInfo = SORCER_manaLoadFindDef(ctx, defNameId, curLvl->block);
        assert(defInfo);
        SORCER_ManaLoadBlockInfo* defBlkInfo = SORCER_manaLoadBlockInfo(ctx, defInfo->block);
        assert(defBlkInfo);
        assert(defBlkInfo->begin == p + 1);
        p = defBlkInfo->end;
        goto next;
    }
    else
    {
        u32 numTypes = SORCER_ctxTypesTotal(sorcer);
        if (MANA_tokFlags(space, p) & MANA_TokFlag_Quoted)
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
                const char* str = MANA_tokData(space, p);
                bool r = SORCER_cellNew(sorcer, type, str, cell);
                if (r)
                {
                    SORCER_cellFree(sorcer, cell);
                    SORCER_blockAddInstPushImm(sorcer, curLvl->block, type, str);
                    goto next;
                }
            }
            SORCER_manaLoadErrorAtTok(ctx, p, SORCER_ManaError_UnkWord);
            goto failed;
        }
        else
        {
            SORCER_ManaKeyword keyword = SORCER_manaKeywordMatchDataId(ctx, tokStr);
            if (keyword != SORCER_ManaKeyword_Invalid)
            {
                switch (keyword)
                {
                case SORCER_ManaKeyword_Apply:
                {
                    SORCER_blockAddInstApply(sorcer, curLvl->block);
                    break;
                }
                default:
                    assert(false);
                    break;
                }
                goto next;
            }
            SORCER_ManaLoadVarInfo* varInfo = SORCER_manaLoadFindVar(ctx, tokStr, curLvl->block);
            if (varInfo)
            {
                SORCER_blockAddInstPushVar(sorcer, curLvl->block, varInfo->var);
                goto next;
            }
            SORCER_ManaLoadDefInfo* defInfo = SORCER_manaLoadFindDef(ctx, tokStr, curLvl->block);
            if (defInfo)
            {
                SORCER_ManaLoadBlockInfo* defBlkInfo = SORCER_manaLoadBlockInfo(ctx, defInfo->block);
                if (!defBlkInfo->loaded)
                {
                    SORCER_manaLoadBlockSetLoaded(ctx, defInfo->block);
                    SORCER_ManaLoadCallLevel level = { defInfo->block, defBlkInfo->begin };
                    vec_push(callStack, level);
                    curLvl->p -= 1;
                    goto next;
                }
                SORCER_blockAddInstCall(sorcer, curLvl->block, defInfo->block);
                goto next;
            }
            SORCER_Opr opr = SORCER_manaLoadFindOpr(ctx, tokStr);
            if (opr.id != SORCER_Opr_Invalid.id)
            {
                SORCER_blockAddInstOpr(sorcer, curLvl->block, opr);
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
                const char* str = MANA_tokData(space, p);
                bool r = SORCER_cellNew(sorcer, type, str, cell);
                if (r)
                {
                    SORCER_cellFree(sorcer, cell);
                    SORCER_blockAddInstPushImm(sorcer, curLvl->block, type, str);
                    goto next;
                }
            }
            SORCER_manaLoadErrorAtTok(ctx, p, SORCER_ManaError_UnkWord);
            goto failed;
        }
    }
    SORCER_manaLoadErrorAtTok(ctx, p, SORCER_ManaError_Syntax);
failed:
    SORCER_manaLoadBlocksRollback(ctx, blocksTotal0);
    return SORCER_Block_Invalid;
}






SORCER_Block SORCER_blockFromManaToks
(
    SORCER_Context* ctx, MANA_Space* space, u32 begin, u32 end,
    const MANA_SpaceSrcInfo* srcInfo, SORCER_ManaErrorInfo* errInfo, SORCER_ManaFileInfoVec* fileTable
)
{
    if (fileTable && !fileTable->length)
    {
        SORCER_ManaFileInfo info = { 0 };
        vec_push(fileTable, info);
    }
    SORCER_ManaLoadContext tctx[1] = { SORCER_manaLoadContextNew(ctx, space, srcInfo, errInfo, fileTable) };
    SORCER_Block r = SORCER_manaLoadBlock(tctx, begin, end);
    SORCER_manaLoadContextFree(tctx);
    return r;
}




SORCER_Block SORCER_blockFromManaStr
(
    SORCER_Context* ctx, const char* str, SORCER_ManaErrorInfo* errInfo, SORCER_ManaFileInfoVec* fileTable
)
{
    SORCER_Block block = SORCER_Block_Invalid;
    MANA_Space* space = MANA_spaceNew();
    MANA_SpaceSrcInfo* srcInfo = MANA_spaceSrcInfoNew();
    MANA_LexingOpt opt[1] = { { "()[]{};," } };
    MANA_lexing(space, str, opt, srcInfo);
    block = SORCER_blockFromManaToks(ctx, space, 0, MANA_spaceToksTotal(space), srcInfo, errInfo, fileTable);
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
    SORCER_Block blk = SORCER_blockFromManaStr(ctx, str, errInfo, fileTable);
    free(str);
    return blk;
}

















































































