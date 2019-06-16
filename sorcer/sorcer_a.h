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










typedef enum SORCER_Prim
{
    SORCER_Prim_Def = 0,
    SORCER_Prim_Var,
    SORCER_Prim_If,
    SORCER_Prim_Apply,

    SORCER_NumPrims
} SORCER_Prim;

const char** SORCER_PrimNameTable(void);






typedef bool(*SORCER_AtypeCtorByStr)(SORCER_Value* pVal, const char* str, u32 len, void* ctx);
typedef void(*SORCER_AtypeDtor)(void* ptr);

typedef struct SORCER_AtypeInfo
{
    const char* name;
    SORCER_AtypeCtorByStr ctorByStr;
    SORCER_AtypeDtor dtor;
    bool fromQuotedStr;
} SORCER_AtypeInfo;





enum
{
    SORCER_AfunIns_MAX = 16,
    SORCER_AfunOuts_MAX = 16,
};

typedef void(*SORCER_AfunCall)(SORCER_Value* ins, SORCER_Value* outs);

typedef struct SORCER_AfunInfo
{
    const char* name;
    const char* typeDecl;
    SORCER_AfunCall call;
} SORCER_AfunInfo;











typedef enum SORCER_Atype
{
    SORCER_Atype_BOOL = 0,
    SORCER_Atype_NUMBER,
    SORCER_Atype_STRING,

    SORCER_NumAtypes
} SORCER_Atype;

const SORCER_AtypeInfo* SORCER_AtypeInfoTable(void);



typedef enum SORCER_Afun
{
    SORCER_Afun_Not,

    SORCER_Afun_Add,
    SORCER_Afun_Sub,
    SORCER_Afun_Mul,
    SORCER_Afun_Div,

    SORCER_Afun_Neg,

    SORCER_Afun_EQ,

    SORCER_Afun_GT,
    SORCER_Afun_LT,
    SORCER_Afun_GE,
    SORCER_Afun_LE,

    SORCER_NumAfuns
} SORCER_Afun;

const SORCER_AfunInfo* SORCER_AfunInfoTable(void);




typedef struct SORCER_AtomTable
{
    u32 numTypes;
    SORCER_AtypeInfo* types;
    u32 numFuns;
    SORCER_AfunInfo* funs;
} SORCER_AtomTable;














typedef enum SORCER_ErrCode
{
    SORCER_ErrCode_NONE,

    SORCER_ErrCode_SrcFile,
    SORCER_ErrCode_ExpSyntax,
    SORCER_ErrCode_EvalSyntax,
    SORCER_ErrCode_EvalUnkWord,
    SORCER_ErrCode_EvalUnkCall,
    SORCER_ErrCode_EvalUnkFunType,
    SORCER_ErrCode_EvalUnkTypeDecl,
    SORCER_ErrCode_EvalArgs,
    SORCER_ErrCode_EvalBranchUneq,
    SORCER_ErrCode_EvalRecurNoBaseCase,
    SORCER_ErrCode_EvalUnification,
    SORCER_ErrCode_EvalAtomCtorByStr,
    SORCER_ErrCode_EvalTypeUnsolvable,

    TXN_NumEvalErrorCodes
} SORCER_ErrCode;

const char** SORCER_ErrCodeNameTable(void);


typedef struct SORCER_Error
{
    SORCER_ErrCode code;
    u32 file;
    u32 line;
    u32 column;
} SORCER_Error;







SORCER_Error SORCER_ctxLastError(SORCER_Context* ctx);
SORCER_ValueVec* SORCER_ctxDataStack(SORCER_Context* ctx);





void SORCER_execBlock(SORCER_Context* ctx, TXN_Node block);
bool SORCER_execCode(SORCER_Context* ctx, const char* filename, const char* code, bool enableSrcInfo);
bool SORCER_execFile(SORCER_Context* ctx, const char* fileName, bool enableSrcInfo);






enum
{
    SORCER_FileName_MAX = 255,
};

typedef struct SORCER_SrcFileInfo
{
    char name[SORCER_FileName_MAX];
} SORCER_SrcFileInfo;

typedef vec_t(SORCER_SrcFileInfo) SORCER_SrcFileInfoTable;

const SORCER_SrcFileInfoTable* SORCER_ctxSrcFileInfoTable(SORCER_Context* ctx);














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





























































