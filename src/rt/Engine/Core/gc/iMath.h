#ifndef IMATH_H
#define IMATH_H

#include "types.h"

#ifndef __MWERKS__
#include <math.h>
#endif

#ifdef __MWERKS__
#define iabs(x) (float)(__fabs((float)(x)))
#else
#define iabs(x) fabsf(x)
#endif

F32 isin(F32 f);
F32 icos(F32 f);
F32 itan(F32 f);

#endif
