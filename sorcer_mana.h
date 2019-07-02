#pragma once



#include "sorcer.h"

#include <mana.h>




typedef enum SR_ManaError
{
    SR_ManaError_NONE = 0,

    SR_ManaError_SrcFile,
    SR_ManaError_Syntax,
    SR_ManaError_UnkWord,
    SR_ManaError_UnkFunType,
    SR_ManaError_UnkTypeDecl,
    SR_ManaError_BranchUneq,
    SR_ManaError_RecurNoBaseCase,
    SR_ManaError_TypeUnsolvable,

    SR_NumManaErrors
} SR_ManaError;

static const char* SR_ManaErrorNameTable(SR_ManaError err)
{
    const char* a[SR_NumManaErrors] =
    {
        "NONE",
        "SrcFile",
        "Syntax",
        "UnkWord",
        "UnkFunType",
        "UnkTypeDecl",
        "BranchUneq",
        "RecurNoBaseCase",
        "TypeUnsolvable",
    };
    return a[err];
}

typedef struct SR_ManaErrorInfo
{
    SR_ManaError error;
    u32 file;
    u32 line;
    u32 column;
} SR_ManaErrorInfo;



enum
{
    SR_ManaFilePath_MAX = 260,
};

typedef struct SR_ManaFileInfo
{
    char path[SR_ManaFilePath_MAX];
} SR_ManaFileInfo;

typedef vec_t(SR_ManaFileInfo) SR_ManaFileInfoVec;



SR_Block SR_blockFromManaToks
(
    SR_Context* ctx, MANA_Space* space, u32 tokId0, u32 numToks,
    const MANA_SpaceSrcInfo* srcInfo, SR_ManaErrorInfo* errInfo, SR_ManaFileInfoVec* fileTable
);

SR_Block SR_blockFromManaStr
(
    SR_Context* ctx, const char* str, SR_ManaErrorInfo* errInfo, SR_ManaFileInfoVec* fileTable
);

SR_Block SR_blockFromManaFile
(
    SR_Context* ctx, const char* path, SR_ManaErrorInfo* errInfo, SR_ManaFileInfoVec* fileTable
);



































































































