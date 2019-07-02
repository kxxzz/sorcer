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




typedef struct SR_Context SR_Context;

SR_Context* SR_ctxNew(void);
void SR_ctxFree(SR_Context* ctx);




typedef struct SR_Type { u32 id; } SR_Type;
static const SR_Type SR_Type_Invalid = { (u32)-1 };

typedef struct SR_Block { u32 id; } SR_Block;
static const SR_Block SR_Block_Invalid = { (u32)-1 };

typedef struct SR_Var { u32 id; } SR_Var;
static const SR_Var SR_Var_Invalid = { (u32)-1 };

typedef struct SR_Opr { u32 id; } SR_Opr;
static const SR_Opr SR_Opr_Invalid = { (u32)-1 };



static const SR_Type SR_Type_Block = { 0 };




typedef union SR_CellValue
{
    u64 val;
    void* ptr;
    SR_Block blk;
} SR_CellValue;

typedef struct SR_Cell
{
    SR_Type type;
    SR_CellValue value[1];
} SR_Cell;

typedef bool(*SR_CellCtor)(void* pool, const char* str, SR_CellValue* pOut);
typedef void(*SR_CellDtor)(void* pool, SR_CellValue* x);
typedef void(*SR_CellCopier)(void* pool, const SR_CellValue* x, SR_CellValue* pOut);
typedef void*(*SR_PoolCtor)(void);
typedef void(*SR_PoolDtor)(void* pool);
typedef u32(*SR_CellToStr)(char* buf, u32 bufLen, void* pool, const SR_CellValue* x);
typedef u32(*SR_CellDumper)(const SR_CellValue* x, char* buf, u32 bufSize);
typedef void(*SR_CellLoader)(SR_CellValue* x, const char* ptr, u32 size);

typedef struct SR_TypeInfo
{
    const char* name;
    SR_CellCtor ctor;
    SR_CellDtor dtor;
    SR_CellCopier copier;
    SR_PoolCtor poolCtor;
    SR_PoolDtor poolDtor;
    SR_CellToStr toStr;
    SR_CellDumper dumper;
    SR_CellLoader loader;
    u64 flags;
} SR_TypeInfo;

SR_Type SR_typeNew(SR_Context* ctx, const SR_TypeInfo* info);
SR_Type SR_typeByName(SR_Context* ctx, const char* name);
SR_Type SR_typeByIndex(SR_Context* ctx, u32 idx);
const SR_TypeInfo* SR_typeInfo(SR_Context* ctx, SR_Type type);
u32 SR_ctxTypesTotal(SR_Context* ctx);

void* SR_pool(SR_Context* ctx, SR_Type type);

bool SR_cellNew(SR_Context* ctx, SR_Type type, const char* str, SR_Cell* out);
void SR_cellFree(SR_Context* ctx, SR_Cell* x);
void SR_cellDup(SR_Context* ctx, const SR_Cell* x, SR_Cell* out);
u32 SR_cellToStr(char* buf, u32 bufSize, SR_Context* ctx, const SR_Cell* x);



typedef void(*SR_OprFunc)(SR_Context* ctx, void* funcCtx, const SR_Cell* ins, SR_Cell* outs);

enum
{
    SR_OprIO_MAX = 16,
};

typedef struct SR_OprInfo
{
    const char* name;
    u32 numIns;
    SR_Type ins[SR_OprIO_MAX];
    u32 numOuts;
    SR_Type outs[SR_OprIO_MAX];
    SR_OprFunc func;
    void* funcCtx;
    bool hasSideEffect;
} SR_OprInfo;

SR_Opr SR_oprNew(SR_Context* ctx, const SR_OprInfo* info);
SR_Opr SR_oprByName(SR_Context* ctx, const char* name);
SR_Opr SR_oprByIndex(SR_Context* ctx, u32 idx);
const SR_OprInfo* SR_oprInfo(SR_Context* ctx, SR_Opr opr);




u32 SR_ctxBlocksTotal(SR_Context* ctx);
void SR_ctxBlocksRollback(SR_Context* ctx, u32 n);


SR_Block SR_blockNew(SR_Context* ctx);


void SR_blockAddInstPopNull(SR_Context* ctx, SR_Block blk);
SR_Var SR_blockAddInstPopVar(SR_Context* ctx, SR_Block blk);

void SR_blockAddInstPushImm(SR_Context* ctx, SR_Block blk, SR_Type type, const char* str);
void SR_blockAddInstPushVar(SR_Context* ctx, SR_Block blk, SR_Var v);
void SR_blockAddInstPushBlock(SR_Context* ctx, SR_Block blk, SR_Block b);
void SR_blockAddInstCall(SR_Context* ctx, SR_Block blk, SR_Block callee);
void SR_blockAddInstApply(SR_Context* ctx, SR_Block blk);
void SR_blockAddInstOpr(SR_Context* ctx, SR_Block blk, SR_Opr opr);

void SR_blockAddInlineBlock(SR_Context* ctx, SR_Block blk, SR_Block ib);






typedef enum SR_RunError
{
    SR_RunError_NONE = 0,

    SR_RunError_OprArgs,

    SR_NumRunErrors
} SR_RunError;

static const char* SR_RunErrorNameTable(SR_RunError err)
{
    const char* a[SR_NumRunErrors] =
    {
        "NONE",
        "OprArgs",
    };
    return a[err];
}

typedef struct SR_RunErrorInfo
{
    SR_RunError error;
    u32 file;
    u32 line;
    u32 column;
} SR_RunErrorInfo;

void SR_run(SR_Context* ctx, SR_Block blk, SR_RunErrorInfo* errInfo);






u32 SR_dsSize(SR_Context* ctx);
const SR_Cell* SR_dsBase(SR_Context* ctx);

void SR_dsPush(SR_Context* ctx, const SR_Cell* x);
void SR_dsPop(SR_Context* ctx, u32 n, SR_Cell* out);









































