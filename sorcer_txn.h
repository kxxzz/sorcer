#pragma once



#include "sorcer.h"

#include <txn.h>




typedef enum SORCER_TxnError
{
    SORCER_TxnError_NONE = 0,

    SORCER_TxnError_TxnFile,
    SORCER_TxnError_TxnSyntex,
    SORCER_TxnError_Syntax,
    SORCER_TxnError_UnkWord,
    SORCER_TxnError_UnkCall,
    SORCER_TxnError_UnkFunType,
    SORCER_TxnError_UnkTypeDecl,
    SORCER_TxnError_BranchUneq,
    SORCER_TxnError_RecurNoBaseCase,
    SORCER_TxnError_TypeUnsolvable,

    SORCER_NumTxnErrors
} SORCER_TxnError;

static const char* SORCER_TxnErrorNameTable(SORCER_TxnError err)
{
    const char* a[SORCER_NumTxnErrors] =
    {
        "NONE",
        "TxnFile",
        "TxnSyntex",
        "Syntax",
        "UnkWord",
        "UnkCall",
        "UnkFunType",
        "UnkTypeDecl",
        "BranchUneq",
        "RecurNoBaseCase",
        "TypeUnsolvable",
    };
    return a[err];
}

typedef struct SORCER_TxnErrorInfo
{
    SORCER_TxnError error;
    u32 file;
    u32 line;
    u32 column;
} SORCER_TxnErrorInfo;




SORCER_Block SORCER_blockFromTxnNode
(
    SORCER_Context* ctx, TXN_Space* space, TXN_Node node, const TXN_SpaceSrcInfo* srcInfo, SORCER_TxnErrorInfo* errInfo
);
SORCER_Block SORCER_blockFromTxnCode(SORCER_Context* ctx, const char* path, SORCER_TxnErrorInfo* errInfo);
SORCER_Block SORCER_blockFromTxnFile(SORCER_Context* ctx, const char* path, SORCER_TxnErrorInfo* errInfo);



































































































