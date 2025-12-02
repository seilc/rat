#include "xRand.h"

#ifndef NON_MATCHING
static void __unused()
{
    *(F32*)0 = 1.0f;
    xrand_RandomBipolarUnitFloat();
}

#endif
