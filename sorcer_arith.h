#pragma once



#include "sorcer.h"



typedef enum SORCER_ArithType
{
    SORCER_ArithType_NUM = 0,

    SORCER_NumArithTypes
} SORCER_ArithType;

typedef enum SORCER_ArithOpr
{
    SORCER_ArithOpr_Neg = 0,
    SORCER_ArithOpr_Add,
    SORCER_ArithOpr_Sub,
    SORCER_ArithOpr_Mul,
    SORCER_ArithOpr_Div,

    SORCER_NumArithOprs
} SORCER_ArithOpr;

typedef struct SORCER_ArithContext
{
    SORCER_Type type[SORCER_NumArithTypes];
    SORCER_Opr opr[SORCER_NumArithOprs];
} SORCER_ArithContext;

SORCER_Type SORCER_ArithTypeTable(SORCER_ArithContext* ctx, SORCER_ArithType at);
SORCER_Opr SORCER_ArithOprTable(SORCER_ArithContext* ctx, SORCER_ArithOpr aop);



void SORCER_arith(SORCER_Context* ctx, SORCER_ArithContext* arithCtx);





























































































