#ifndef XMATH_H
#define XMATH_H

#include "iMath.h"

#define xabs(x) iabs(x)

F32 xDampSpringCoeff(F32 dt, F32 k);
void xDampSpring(F32& s, F32& v, F32 dt, F32 k, F32 e);

#endif