#pragma once


#include "sorcer.h"



typedef vec_t(SORCER_AtypeInfo) SORCER_AtypeInfoVec;
typedef vec_t(SORCER_AfunInfo) SORCER_AfunInfoVec;






typedef enum SORCER_NodeType
{
    SORCER_NodeType_None = 0,

    SORCER_NodeType_Afun,
    SORCER_NodeType_Apply,
    SORCER_NodeType_BlockExe,

    SORCER_NodeType_Var,
    SORCER_NodeType_Def,
    SORCER_NodeType_Atom,
    SORCER_NodeType_Block,

    SORCER_NodeType_CallAfun,
    SORCER_NodeType_CallDef,
    SORCER_NodeType_CallVar,

    SORCER_NodeType_VarNew,
    SORCER_NodeType_DefNew,
    SORCER_NodeType_If,
} SORCER_NodeType;

typedef struct SORCER_NodeVar
{
    TXN_Node block;
    u32 id;
} SORCER_NodeVar;

typedef struct SORCER_Node
{
    SORCER_NodeType type;
    union
    {
        u32 afun;
        u32 atype;
        TXN_Node blkSrc;
        SORCER_NodeVar var;
    };
    u32 numIns;
    u32 numOuts;
    u32 varsCount;
} SORCER_Node;

typedef vec_t(SORCER_Node) SORCER_NodeTable;













static bool TXN_evalCheckCall(TXN_Space* space, TXN_Node node)
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





static const TXN_Node* TXN_evalIfBranch0(TXN_Space* space, TXN_Node node)
{
    const TXN_Node* elms = TXN_seqElm(space, node);
    return elms + 2;
}
static const TXN_Node* TXN_evalIfBranch1(TXN_Space* space, TXN_Node node)
{
    const TXN_Node* elms = TXN_seqElm(space, node);
    return elms + 3;
}








static void TXN_evalErrorFound
(
    SORCER_Error* err, const TXN_SpaceSrcInfo* srcInfo, SORCER_ErrCode errCode, TXN_Node node
)
{
    if (err && srcInfo)
    {
        err->code = errCode;
        const TXN_NodeSrcInfo* nodeSrcInfo = TXN_nodeSrcInfo(srcInfo, node);
        err->file = nodeSrcInfo->file;
        err->line = nodeSrcInfo->line;
        err->column = nodeSrcInfo->column;
    }
}













typedef enum SORCER_BlockCallbackType
{
    SORCER_BlockCallbackType_NONE,
    SORCER_BlockCallbackType_Ncall,
    SORCER_BlockCallbackType_CallWord,
    SORCER_BlockCallbackType_CallVar,
    SORCER_BlockCallbackType_Cond,
    SORCER_BlockCallbackType_ArrayMap,
    SORCER_BlockCallbackType_ArrayFilter,
    SORCER_BlockCallbackType_ArrayReduce,
} SORCER_BlockCallbackType;



typedef struct SORCER_BlockCallback
{
    SORCER_BlockCallbackType type;
    union
    {
        u32 afun;
        TXN_Node blkSrc;
        u32 pos;
    };
} SORCER_BlockCallback;





typedef struct SORCER_Call
{
    TXN_Node srcNode;
    const TXN_Node* p;
    const TXN_Node* end;
    u32 varBase;
    SORCER_BlockCallback cb;
} SORCER_Call;

typedef vec_t(SORCER_Call) SORCER_CallStack;











typedef struct SORCER_Array
{
    SORCER_ValueVec data;
    u32 elmSize;
    u32 size;
} SORCER_Array;









typedef vec_t(SORCER_Array*) SORCER_ArrayPtrVec;









#include "exp_eval_object.h"










typedef struct SORCER_Context
{
    TXN_Space* space;
    TXN_SpaceSrcInfo srcInfo;
    SORCER_TypeContext* typeContext;
    SORCER_AtypeInfoVec atypeTable;
    SORCER_AfunInfoVec afunTable;
    vec_u32 afunTypeTable;
    SORCER_NodeTable nodeTable;
    SORCER_ObjectTable objectTable;
    APNUM_pool_t numPool;

    vec_u32 typeStack;
    SORCER_CallStack callStack;
    SORCER_ValueVec varStack;
    SORCER_ValueVec dataStack;

    SORCER_Error error;
    SORCER_Value ncallOutBuf[SORCER_AfunOuts_MAX];
    bool gcFlag;
    SORCER_TypeDeclContext* typeDeclContext;
    SORCER_ArrayPtrVec aryStack;

    SORCER_FileInfoTable fileInfoTable;
} SORCER_Context;





























































