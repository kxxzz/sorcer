
/*
#include "sorcer_a.h"





u32 TXN_evalArraySize(SORCER_Array* a)
{
    return a->size;
}

u32 TXN_evalArrayElmSize(SORCER_Array* a)
{
    if (a->data.length)
    {
        assert(a->data.length / a->size == a->elmSize);
    }
    return a->elmSize;
}





void TXN_evalArraySetElmSize(SORCER_Array* a, u32 elmSize)
{
    a->elmSize = elmSize;
    u32 dataLen = a->elmSize * a->size;
    vec_resize(&a->data, dataLen);
}

void TXN_evalArrayResize(SORCER_Array* a, u32 size)
{
    a->size = size;
    u32 dataLen = a->elmSize * a->size;
    vec_resize(&a->data, dataLen);
}






bool TXN_evalArraySetElm(SORCER_Array* a, u32 p, const SORCER_Value* inBuf)
{
    if (p < a->size)
    {
        u32 elmSize = a->elmSize;
        memcpy(a->data.data + elmSize * p, inBuf, sizeof(SORCER_Value)*elmSize);
        return true;
    }
    else
    {
        return false;
    }
}




bool TXN_evalArrayGetElm(SORCER_Array* a, u32 p, SORCER_Value* outBuf)
{
    if (p < a->size)
    {
        u32 elmSize = a->elmSize;
        memcpy(outBuf, a->data.data + elmSize * p, sizeof(SORCER_Value)*elmSize);
        return true;
    }
    else
    {
        return false;
    }
}






void TXN_evalArrayPushElm(SORCER_Array* a, const SORCER_Value* inBuf)
{
    ++a->size;
    u32 elmSize = a->elmSize;
    u32 dataLen = elmSize * a->size;
    vec_resize(&a->data, dataLen);
    memcpy(a->data.data + elmSize * (a->size - 1), inBuf, sizeof(SORCER_Value)*elmSize);
}






*/











































































