#ifndef XMATH_H
#define XMATH_H

#include "iMath.h"

#define EPSILON 1e-5f
#define PI 3.14159265358979323846f
#define HUGE 1e38f

#define xabs(x) iabs(x)
#define xeq(a,b,e) (xabs((a) - (b)) <= (e))
#define xfeq0(x) (((x) >= -EPSILON) && ((x) <= EPSILON))
#define xmin(a,b) ((a) < (b) ? (a) : (b))
#define xmax(a,b) ((a) > (b) ? (a) : (b))
#define xmax3(a,b,c) (xmax(xmax((a), (b)), (c)))
#define xclamp(x,a,b) xmax((a), xmin((x), (b)))
#define xsqr(x) ((x) * (x))

inline S32 log2ceil(S32 value)
{
    S32 power = 0;

    S32 v = value;
    while (v > 1) {
        v >>= 1;
        power++;
    }

    if ((1 << power) < value) {
        power++;
    }

    return power;
}

F32 xDampSpringCoeff(F32 dt, F32 k);
void xDampSpring(F32& s, F32& v, F32 dt, F32 k, F32 e);

F32 xsqrt(F32 x);
F32 xexp(F32 x);
F32 xasin(F32 x);
F32 xatan2(F32 y, F32 x);

#ifndef DEBUG
#include "xMathInlines.h"
#endif

#endif
