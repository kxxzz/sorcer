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
static SORCER_Type SORCER_Type_Invalid = { (u32)-1 };

typedef struct SORCER_Block { u32 id; } SORCER_Block;
static SORCER_Block SORCER_Block_Invalid = { (u32)-1 };

typedef struct SORCER_Var { u32 id; } SORCER_Var;
static SORCER_Var SORCER_Var_Invalid = { (u32)-1 };

typedef struct SORCER_Opr { u32 id; } SORCER_Opr;
static SORCER_Opr SORCER_Opr_Invalid = { (u32)-1 };



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

typedef bool(*SORCER_CellCtor)(void* pool, const char* str, SORCER_Cell* out);
typedef void(*SORCER_CellDtor)(void* pool, void* ptr);
typedef void*(*SORCER_PoolCtor)(void);
typedef void(*SORCER_PoolDtor)(void* pool);

typedef struct SORCER_TypeInfo
{
    const char* name;
    SORCER_CellCtor cellCtor;
    SORCER_CellDtor cellDtor;
    SORCER_PoolCtor poolCtor;
    SORCER_PoolDtor poolDtor;
    bool litQuoted;
} SORCER_TypeInfo;

SORCER_Type SORCER_typeNew(SORCER_Context* ctx, const SORCER_TypeInfo* info);
SORCER_Type SORCER_typeByName(SORCER_Context* ctx, const char* name);
u32 SORCER_ctxTypesTotal(SORCER_Context* ctx);
bool SORCER_cellNew(SORCER_Context* ctx, SORCER_Type type, const char* str, bool quoted, SORCER_Cell* out);





typedef void(*SORCER_OprFunc)(const SORCER_Cell* ins, SORCER_Cell* outs);

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
} SORCER_OprInfo;

SORCER_Opr SORCER_oprNew(SORCER_Context* ctx, const SORCER_OprInfo* info);
SORCER_Opr SORCER_oprByName(SORCER_Context* ctx, const char* name);




u32 SORCER_ctxBlocksTotal(SORCER_Context* ctx);
void SORCER_ctxBlocksRollback(SORCER_Context* ctx, u32 n);

SORCER_Block SORCER_blockNew(SORCER_Context* ctx);




SORCER_Var SORCER_blockAddInstPopVar(SORCER_Context* ctx, SORCER_Block blk);

void SORCER_blockAddInstPushCell(SORCER_Context* ctx, SORCER_Block blk, const SORCER_Cell* x);
void SORCER_blockAddInstPushVar(SORCER_Context* ctx, SORCER_Block blk, SORCER_Var v);
void SORCER_blockAddInstPushBlock(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block b);
void SORCER_blockAddInstCall(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block callee);
void SORCER_blockAddInstApply(SORCER_Context* ctx, SORCER_Block blk);
void SORCER_blockAddInstIfte(SORCER_Context* ctx, SORCER_Block blk);
void SORCER_blockAddInstOpr(SORCER_Context* ctx, SORCER_Block blk, SORCER_Opr opr);

void SORCER_blockAddInlineBlock(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block ib);

void SORCER_blockAddPatIfteCT(SORCER_Context* ctx, SORCER_Block blk, SORCER_Block onTrue, SORCER_Block onFalse);





typedef enum SORCER_RunErr
{
    SORCER_RunErr_NONE = 0,

    SORCER_RunErr_TypeUnmatch,

    SORCER_NumRunErrs
} SORCER_RunErr;

SORCER_RunErr SORCER_run(SORCER_Context* ctx, SORCER_Block blk);






u32 SORCER_dsSize(SORCER_Context* ctx);
const SORCER_Cell* SORCER_dsBase(SORCER_Context* ctx);

void SORCER_dsPush(SORCER_Context* ctx, const SORCER_Cell* x);
void SORCER_dsPop(SORCER_Context* ctx, u32 n, SORCER_Cell* out);








































