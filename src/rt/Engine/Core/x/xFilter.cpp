#include "xFilter.h"

F32 xFilterCurvedInterp5(F32 in_t)
{
    F32 in_t2 = in_t * in_t;
    F32 in_t3 = in_t2 * in_t;
    F32 out_t = 3.9166667f * in_t + -5.0f * in_t2 + 2.0833333f * in_t3;
    return out_t;
}