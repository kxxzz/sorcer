#pragma once



#include "sorcer.h"

#include <txn.h>




typedef enum SORCER_TxnErr
{
    SORCER_TxnErr_NONE,

    SORCER_TxnErr_SrcFile,
    SORCER_TxnErr_Syntax,
    SORCER_TxnErr_UnkWord,
    SORCER_TxnErr_UnkCall,
    SORCER_TxnErr_UnkFunType,
    SORCER_TxnErr_UnkTypeDecl,
    SORCER_TxnErr_Args,
    SORCER_TxnErr_BranchUneq,
    SORCER_TxnErr_RecurNoBaseCase,
    SORCER_TxnErr_Unification,
    SORCER_TxnErr_AtomCtorByStr,
    SORCER_TxnErr_TypeUnsolvable,

    SORCER_NumTxnErrs
} SORCER_TxnErr;

static const char** SORCER_TxnErrNameTable(void)
{
    const char* a[SORCER_NumTxnErrs] =
    {
        "NONE",
        "SrcFile",
        "Syntax",
        "UnkWord",
        "UnkCall",
        "UnkFunType",
        "UnkTypeDecl",
        "Args",
        "BranchUneq",
        "RecurNoBaseCase",
        "Unification",
        "AtomCtorByStr",
        "TypeUnsolvable",
    };
    return a;
}



typedef struct SORCER_TxnErrInfo
{
    SORCER_TxnErr code;
    u32 file;
    u32 line;
    u32 column;
} SORCER_TxnErrInfo;




SORCER_Block SORCER_blockFromTxnNode(SORCER_Context* ctx, TXN_Space* space, TXN_Node node);
SORCER_Block SORCER_blockFromTxnFile(SORCER_Context* ctx, const char* path);



































































































