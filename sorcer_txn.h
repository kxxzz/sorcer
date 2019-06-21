#pragma once



#include "sorcer.h"

#include <txn.h>




typedef enum SORCER_TxnErr
{
    SORCER_TxnErr_NONE = 0,

    SORCER_TxnErr_TxnFile,
    SORCER_TxnErr_TxnSyntex,
    SORCER_TxnErr_UnkFormat,
    SORCER_TxnErr_UnkWord,
    SORCER_TxnErr_UnkCall,
    SORCER_TxnErr_UnkFunType,
    SORCER_TxnErr_UnkTypeDecl,
    SORCER_TxnErr_Args,
    SORCER_TxnErr_BranchUneq,
    SORCER_TxnErr_RecurNoBaseCase,
    SORCER_TxnErr_Unification,
    SORCER_TxnErr_CellFromSym,
    SORCER_TxnErr_TypeUnsolvable,

    SORCER_NumTxnErrs
} SORCER_TxnErr;

static const char** SORCER_TxnErrNameTable(void)
{
    const char* a[SORCER_NumTxnErrs] =
    {
        "NONE",
        "TxnFile",
        "TxnSyntex",
        "Syntax",
        "UnkWord",
        "UnkCall",
        "UnkFunType",
        "UnkTypeDecl",
        "Args",
        "BranchUneq",
        "RecurNoBaseCase",
        "Unification",
        "CellFromSym",
        "TypeUnsolvable",
    };
    return a;
}



typedef struct SORCER_TxnErrInfo
{
    SORCER_TxnErr error;
    u32 file;
    u32 line;
    u32 column;
} SORCER_TxnErrInfo;




SORCER_Block SORCER_blockFromTxnNode
(
    SORCER_Context* ctx, TXN_Space* space, TXN_Node node, const TXN_SpaceSrcInfo* srcInfo, SORCER_TxnErrInfo* errInfo
);

SORCER_Block SORCER_blockFromTxnFile(SORCER_Context* ctx, const char* path, SORCER_TxnErrInfo* errInfo);



































































































