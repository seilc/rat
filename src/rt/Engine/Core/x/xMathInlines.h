#ifndef XMATHINLINES_H
#define XMATHINLINES_H

#if DEBUG
#define inline
#endif

inline F32 xsqrt(register F32 x)
{
    register F32 c_half = 0.5f;
    register F32 c_three = 3.0f;
    register F32 rsqrt;
    register F32 nwork0;
    register F32 nwork1;

    if (x <= 0.0f || isinf(x)) {
        return x;
    }

    asm {
        frsqrte rsqrt, x
        fmuls nwork0, rsqrt, rsqrt
        fmuls nwork1, rsqrt, c_half
        fnmsubs nwork0, nwork0, x, c_three
        fmuls rsqrt, nwork0, nwork1
    }

    if (rsqrt > 0.00001f) {
        return 1.0f / rsqrt;
    }

    return 100000.0f;
}

// RESTORED: from RotU GC
inline F32 xatan(F32 x)
{
    return std::atanf(x);
}
// END RESTORED

inline void xatanfast(F32& fValue, F32);
inline void xatan2fast(F32& o, F32 y, F32 x);

#undef inline

#endif
