#pragma once



#include "sorcer.h"

#include <txn.h>




typedef struct SORCER_TxnLoadStepInfo
{
    const char* name;
} SORCER_TxnLoadStepInfo;

typedef vec_t(SORCER_TxnLoadStepInfo) SORCER_TxnStepInfoVec;

typedef bool(*SORCER_TxnLoadCellFromSym)(SORCER_Cell* out, const char* str, u32 len);

typedef struct SORCER_TxnLoadBase
{
    SORCER_TxnStepInfoVec stepInfoTable[1];
    SORCER_TxnLoadCellFromSym cellFromSym;
} SORCER_TxnLoadBase;



SORCER_Block SORCER_blockFromTxnNode(SORCER_Context* ctx, const SORCER_TxnLoadBase* base, TXN_Space* space, TXN_Node node);
SORCER_Block SORCER_blockFromTxnFile(SORCER_Context* ctx, const SORCER_TxnLoadBase* base, const char* path);



































































































