#ifndef XMATH_H
#define XMATH_H

#include "iMath.h"

#include <cmath>

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

template <class T>
T range_limit(T v, T minv, T maxv);

F32 xDampSpringCoeff(F32 dt, F32 k);
void xDampSpring(F32& s, F32& v, F32 dt, F32 k, F32 e);
void xDampSpring(F32& s, F32& v, F32 dt, F32 k);

F32 xsqrt(F32 x);
F32 xexp(F32 x);
F32 xasin(F32 x);
F32 xatan2(F32 y, F32 x);
void xsqrtfast(F32& o, F32 fVal);

inline F32 xrmod(F32 ang)
{
    F32 frac = (1.0f / (2.0f * PI)) * ang;
    if (frac < 0.0f) return 2.0f * PI * (1.0f + (frac - std::ceilf(frac)));
    else if (frac >= 1.0f) return 2.0f * PI * (frac - std::floorf(frac));
    return ang;
}

#if !DEBUG
#include "xMathInlines.h"
#endif

#endif
