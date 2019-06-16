#pragma once



#include "txn.h"



typedef enum SORCER_Key
{
    SORCER_Key_Def,
    SORCER_Key_Var,
    SORCER_Key_If,
    SORCER_Key_Apply,

    TXN_NumEvalKeys
} SORCER_Key;

const char** SORCER_KeyNameTable(void);



typedef enum SORCER_ValueType
{
    SORCER_ValueType_Inline = 0,
    SORCER_ValueType_Object,

    TXN_NumEvalValueTypes
} SORCER_ValueType;

typedef struct SORCER_Value
{
    union
    {
        bool b;
        void* a;
        TXN_Node src;
        struct SORCER_Array* ary;
    };
    SORCER_ValueType type;
} SORCER_Value;


typedef vec_t(struct SORCER_Value) SORCER_ValueVec;


typedef struct SORCER_Array SORCER_Array;

u32 TXN_evalArraySize(SORCER_Array* a);
u32 TXN_evalArrayElmSize(SORCER_Array* a);
void TXN_evalArraySetElmSize(SORCER_Array* a, u32 elmSize);
void TXN_evalArrayResize(SORCER_Array* a, u32 size);
bool TXN_evalArraySetElm(SORCER_Array* a, u32 p, const SORCER_Value* inBuf);
bool TXN_evalArrayGetElm(SORCER_Array* a, u32 p, SORCER_Value* outBuf);
void TXN_evalArrayPushElm(SORCER_Array* a, const SORCER_Value* inBuf);



typedef struct SORCER_Context SORCER_Context;


typedef bool(*SORCER_AtomCtorByStr)(SORCER_Value* pVal, const char* str, u32 len, SORCER_Context* ctx);
typedef void(*SORCER_AtomADtor)(void* ptr);

typedef struct SORCER_AtypeInfo
{
    const char* name;
    SORCER_AtomCtorByStr ctorByStr;
    bool fromSymAble;
    u32 allocMemSize;
    SORCER_AtomADtor aDtor;
} SORCER_AtypeInfo;



enum
{
    SORCER_AfunIns_MAX = 16,
    SORCER_AfunOuts_MAX = 16,
};

typedef void(*SORCER_AfunCall)(TXN_Space* space, SORCER_Value* ins, SORCER_Value* outs, SORCER_Context* ctx);

typedef enum SORCER_AfunMode
{
    SORCER_AfunMode_Independ = 0,
    SORCER_AfunMode_ContextDepend,
    SORCER_AfunMode_HighOrder,
} SORCER_AfunMode;

typedef struct SORCER_AfunInfo
{
    const char* name;
    const char* typeDecl;
    SORCER_AfunCall call;
    SORCER_AfunMode mode;
} SORCER_AfunInfo;




typedef enum SORCER_PrimType
{
    SORCER_PrimType_BOOL,
    SORCER_PrimType_NUM,
    SORCER_PrimType_STRING,

    TXN_NumEvalPrimTypes
} SORCER_PrimType;

const SORCER_AtypeInfo* SORCER_PrimTypeInfoTable(void);



typedef enum SORCER_PrimFun
{

    SORCER_PrimFun_Array,
    SORCER_PrimFun_LoadAt,
    SORCER_PrimFun_SaveAt,
    SORCER_PrimFun_Size,

    SORCER_PrimFun_Map,
    SORCER_PrimFun_Filter,
    SORCER_PrimFun_Reduce,

    SORCER_PrimFun_Not,

    SORCER_PrimFun_Add,
    SORCER_PrimFun_Sub,
    SORCER_PrimFun_Mul,
    SORCER_PrimFun_Div,

    SORCER_PrimFun_Neg,

    SORCER_PrimFun_EQ,

    SORCER_PrimFun_GT,
    SORCER_PrimFun_LT,
    SORCER_PrimFun_GE,
    SORCER_PrimFun_LE,

    TXN_NumEvalPrimFuns
} SORCER_PrimFun;

const SORCER_AfunInfo* SORCER_PrimFunInfoTable(void);




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





typedef struct SORCER_Context SORCER_Context;


SORCER_Context* TXN_newEvalContext(const SORCER_AtomTable* addAtomTable);
void TXN_evalContextFree(SORCER_Context* ctx);




SORCER_Error TXN_evalLastError(SORCER_Context* ctx);
SORCER_TypeContext* TXN_evalDataTypeContext(SORCER_Context* ctx);
vec_u32* TXN_evalDataTypeStack(SORCER_Context* ctx);
SORCER_ValueVec* TXN_evalDataStack(SORCER_Context* ctx);




void TXN_evalPushValue(SORCER_Context* ctx, u32 type, SORCER_Value val);
void TXN_evalDrop(SORCER_Context* ctx);




void TXN_evalBlock(SORCER_Context* ctx, TXN_Node block);
bool TXN_evalCode(SORCER_Context* ctx, const char* filename, const char* code, bool enableSrcInfo);
bool TXN_evalFile(SORCER_Context* ctx, const char* fileName, bool enableSrcInfo);






enum
{
    SORCER_FileName_MAX = 255,
};

typedef struct SORCER_FileInfo
{
    char name[SORCER_FileName_MAX];
} SORCER_FileInfo;

typedef vec_t(SORCER_FileInfo) SORCER_FileInfoTable;

const SORCER_FileInfoTable* TXN_evalFileInfoTable(SORCER_Context* ctx);






















































































