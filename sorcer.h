#pragma once



#include <stdbool.h>


typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef float f32;
typedef double f64;




typedef struct SORCER_Context SORCER_Context;

SORCER_Context* SORCER_ctxNew(void);
void SORCER_ctxFree(SORCER_Context* ctx);




typedef struct SORCER_Type { u32 id; } SORCER_Type;
static const SORCER_Type SORCER_Type_Invalid = { (u32)-1 };

typedef struct SORCER_Block { u32 id; } SORCER_Block;
static const SORCER_Block SORCER_Block_Invalid = { (u32)-1 };

typedef struct SORCER_Var { u32 id; } SORCER_Var;
static const SORCER_Var SORCER_Var_Invalid = { (u32)-1 };

typedef struct SORCER_Opr { u32 id; } SORCER_Opr;
static const SORCER_Opr SORCER_Opr_Invalid = { (u32)-1 };



static const SORCER_Type SORCER_Type_Block = { 0 };




typedef struct SORCER_Cell
{
    SORCER_Type type;
    union
    {
        u64 val;
        void* ptr;
        SORCER_Block block;
        u32 address;
    } as;
} SORCER_Cell;

typedef bool(*SORCER_CellCtor)(void* pool, const char* str, void** pOut);
typedef void(*SORCER_CellDtor)(void* pool, void* x);
typedef void(*SORCER_CellCopier)(void* pool, const void* x, void** pOut);
typedef void*(*SORCER_PoolCtor)(void);
typedef void(*SORCER_PoolDtor)(void* pool);
typedef u32(*SORCER_CellToStr)(char* buf, u32 bufLen, void* pool, const SORCER_Cell* x);

typedef struct SORCER_TypeInfo
{
    const char* name;
    SORCER_CellCtor ctor;
    SORCER_CellDtor dtor;
    SORCER_CellCopier copier;
    SORCER_PoolCtor poolCtor;
    SORCER_PoolDtor poolDtor;
    SORCER_CellToStr toStr;
    u64 flags;
} SORCER_TypeInfo;

SORCER_Type SORCER_typeNew(SORCER_Context* ctx, const SORCER_TypeInfo* info);
SORCER_Type SORCER_typeByName(SORCER_Context* ctx, const char* name);
SORCER_Type SORCER_typeByIndex(SORCER_Context* ctx, u32 idx);
const SORCER_TypeInfo* SORCER_typeInfo(SORCER_Context* ctx, SORCER_Type type);
u32 SORCER_ctxTypesTotal(SORCER_Context* ctx);

void* SORCER_pool(SORCER_Context* ctx, SORCER_Type type);

bool SORCER_cellNew(SORCER_Context* ctx, SORCER_Type type, const char* str, SORCER_Cell* out);
void SORCER_cellFree(SORCER_Context* ctx, SORCER_Cell* x);
void SORCER_cellDup(SORCER_Context* ctx, const SORCER_Cell* x, SORCER_Cell* out);
u32 SORCER_cellToStr(char* buf, u32 bufSize, SORCER_Context* ctx, const SORCER_Cell* x);



typedef void(*SORCER_OprFunc)(SORCER_Context* ctx, void* funcCtx, const SORCER_Cell* ins, SORCER_Cell* outs);

enum
{
    SORCER_OprIO_MAX = 16,
};

typedef struct SORCER_OprInfo
{
    const char* name;
    u32 numIns;
    SORCER_Type ins[SORCER_OprIO_MAX];
    u32 numOuts;
    SORCER_Type outs[SORCER_OprIO_MAX];
    SORCER_OprFunc func;
    void* funcCtx;
    bool hasSideEffect;
} SORCER_OprInfo;

SORCER_Opr SORCER_oprNew(SORCER_Context* ctx, const SORCER_OprInfo* info);
SORCER_Opr SORCER_oprByName(SORCER_Context* ctx, const char* name);
SORCER_Opr SORCER_oprByIndex(SORCER_Context* ctx, u32 idx);
const SORCER_OprInfo* SORCER_oprInfo(SORCER_Context* ctx, SORCER_Opr opr);




u32 SORCER_ctxBlocksTotal(SORCER_Context* ctx);
void SORCER_ctxBlocksRollback(SORCER_Context* ctx, u32 n);


SORCER_Block SORCER_blockNew(SORCER_Context* ctx);


void SORCER_blockAddInstPopNull(SORCER_Context* ctx, SORCER_Block blk);
SORCER_Var SORCER_blockAddInstPopVar(SORCER_Context* ctx, SORCER_Block blk);

void SORCER_blockAddInstPushImm(SORCER_Context* ctx, SORCER_Block blk, SORCER_Type type, const char* str);
void SORCER_blockAddInstPushVar(SORCER_Context* ctx, SORCER_Block blk, SORCER_Var v);
void SORCER_blockAddInstPushBlock(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block b);
void SORCER_blockAddInstCall(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block callee);
void SORCER_blockAddInstApply(SORCER_Context* ctx, SORCER_Block blk);
void SORCER_blockAddInstOpr(SORCER_Context* ctx, SORCER_Block blk, SORCER_Opr opr);

void SORCER_blockAddInlineBlock(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block ib);






typedef enum SORCER_RunError
{
    SORCER_RunError_NONE = 0,

    SORCER_RunError_OprArgs,

    SORCER_NumRunErrors
} SORCER_RunError;

static const char* SORCER_RunErrorNameTable(SORCER_RunError err)
{
    const char* a[SORCER_NumRunErrors] =
    {
        "NONE",
        "OprArgs",
    };
    return a[err];
}

typedef struct SORCER_RunErrorInfo
{
    SORCER_RunError error;
    u32 file;
    u32 line;
    u32 column;
} SORCER_RunErrorInfo;

void SORCER_run(SORCER_Context* ctx, SORCER_Block blk, SORCER_RunErrorInfo* errInfo);






u32 SORCER_dsSize(SORCER_Context* ctx);
const SORCER_Cell* SORCER_dsBase(SORCER_Context* ctx);

void SORCER_dsPush(SORCER_Context* ctx, const SORCER_Cell* x);
void SORCER_dsPop(SORCER_Context* ctx, u32 n, SORCER_Cell* out);









































