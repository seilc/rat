#ifndef XMATH_H
#define XMATH_H

#include "types.h"

F32 xDampSpringCoeff(F32 dt, F32 k);
void xDampSpring(F32& s, F32& v, F32 dt, F32 k, F32 e);

#endif