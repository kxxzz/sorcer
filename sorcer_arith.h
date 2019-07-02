#pragma once



#include "sorcer.h"



typedef enum SR_ArithType
{
    SR_ArithType_NUM = 0,

    SR_NumArithTypes
} SR_ArithType;

typedef enum SR_ArithOpr
{
    SR_ArithOpr_Neg = 0,
    SR_ArithOpr_Add,
    SR_ArithOpr_Sub,
    SR_ArithOpr_Mul,
    SR_ArithOpr_Div,

    SR_NumArithOprs
} SR_ArithOpr;

typedef struct SR_ArithContext
{
    SR_Type type[SR_NumArithTypes];
    SR_Opr opr[SR_NumArithOprs];
} SR_ArithContext;

SR_Type SR_ArithTypeTable(SR_ArithContext* ctx, SR_ArithType at);
SR_Opr SR_ArithOprTable(SR_ArithContext* ctx, SR_ArithOpr aop);



void SR_arith(SR_Context* ctx, SR_ArithContext* arithCtx);





























































































