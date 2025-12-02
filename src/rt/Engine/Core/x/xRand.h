#ifndef XRAND_H
#define XRAND_H

#include "xDebug.h"

F32 xrand_GenRandFloat();

U16 xrand_RandomInt16();
U32 xrand_RandomInt32();
F32 xrand_RandomFloat();

inline U16 xrand_RandomInt16()
{
    return (U16)xrand_RandomInt32();
}

inline F32 xrand_RandomBipolarUnitFloat()
{
    return 2.0f * xrand_RandomFloat() - 1.0f;
}

inline U32 xrand_RandomChoice(U32 nChoice)
{
    xASSERT(124, nChoice <= 0xffff);
    return (nChoice * xrand_RandomInt16()) / 0x10000;
}

#endif
