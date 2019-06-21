#include "sorcer_a.h"





static void SORCER_stepFunc_Not(const SORCER_Cell* ins, SORCER_Cell* outs)
{
    outs[0].as.val = !ins[0].as.val;
}



const SORCER_StepInfo* SORCER_StepInfoTable(void)
{
    static const SORCER_StepInfo a[SORCER_NumSteps] =
    {
        { "not", 1, 1, SORCER_stepFunc_Not },
    };
    return a;
}































































































