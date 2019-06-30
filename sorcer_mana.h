#pragma once



#include "sorcer.h"

#include <mana.h>




typedef enum SORCER_ManaError
{
    SORCER_ManaError_NONE = 0,

    SORCER_ManaError_SrcFile,
    SORCER_ManaError_Syntax,
    SORCER_ManaError_UnkWord,
    SORCER_ManaError_UnkFunType,
    SORCER_ManaError_UnkTypeDecl,
    SORCER_ManaError_BranchUneq,
    SORCER_ManaError_RecurNoBaseCase,
    SORCER_ManaError_TypeUnsolvable,

    SORCER_NumManaErrors
} SORCER_ManaError;

static const char* SORCER_ManaErrorNameTable(SORCER_ManaError err)
{
    const char* a[SORCER_NumManaErrors] =
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

typedef struct SORCER_ManaErrorInfo
{
    SORCER_ManaError error;
    u32 file;
    u32 line;
    u32 column;
} SORCER_ManaErrorInfo;



enum
{
    SORCER_ManaFilePath_MAX = 260,
};

typedef struct SORCER_ManaFileInfo
{
    char path[SORCER_ManaFilePath_MAX];
} SORCER_ManaFileInfo;

typedef vec_t(SORCER_ManaFileInfo) SORCER_ManaFileInfoVec;



SORCER_Block SORCER_blockFromManaToks
(
    SORCER_Context* ctx, MANA_Space* space, u32 tokId0, u32 numToks,
    const MANA_SpaceSrcInfo* srcInfo, SORCER_ManaErrorInfo* errInfo, SORCER_ManaFileInfoVec* fileTable
);

SORCER_Block SORCER_blockFromManaStr
(
    SORCER_Context* ctx, const char* str, SORCER_ManaErrorInfo* errInfo, SORCER_ManaFileInfoVec* fileTable
);

SORCER_Block SORCER_blockFromManaFile
(
    SORCER_Context* ctx, const char* path, SORCER_ManaErrorInfo* errInfo, SORCER_ManaFileInfoVec* fileTable
);



































































































