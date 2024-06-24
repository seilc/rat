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

#endif