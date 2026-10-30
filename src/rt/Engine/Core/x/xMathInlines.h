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

inline F32 xlog(F32 x)
{
    return std::logf(x);
}

inline F32 xexp(F32 x)
{
    return std::expf(x);
}

inline F32 xfmod(F32 x, F32 y)
{
    return std::fmodf(x, y);
}

inline F32 xpow(F32 x, F32 y)
{
    return std::powf(x, y);
}

inline F32 xacos(F32 x)
{
    if (x > 1.0f) {
        x = 1.0f;
    }

    if (x < -1.0f) {
        x = -1.0f;
    }

    return std::acosf(x);
}

inline F32 xasin(F32 x)
{
    if (x > 1.0f) {
        x = 1.0f;
    }

    if (x < -1.0f) {
        x = -1.0f;
    }

    return std::asinf(x);
}

// RESTORED: from RotU GC
inline F32 xatan(F32 x)
{
    return std::atanf(x);
}
// END RESTORED

inline F32 xcosh(F32 x);
inline F32 xsinh(F32 x);
inline F32 xtanh(F32 x);

inline F32 xatan2(F32 y, F32 x)
{
    return xAngleClampFast(std::atan2f(y, x));
}

inline F32 xmin3(F32 a, F32 b, F32 c);

inline void xsqrtfast(F32& o, register F32 fVal)
{
    register F32 fRetVal;

    xASSERT_L(191, fVal >= 0.0f);

    if (fVal <= 0.0f)
        o = 0.0f;
    else
        asm {
            frsqrte fRetVal, fVal
            fres fRetVal, fRetVal
        }

    o = fRetVal;
}

inline void xlogfast(F32& o, F32 x);
inline void xexpfast(F32& o, F32 x);
inline void xfmodfast(F32& o, F32 x, F32 y);
inline void xceilfast(F32& o, F32 x);
inline void xfloorfast(F32& o, F32 x);
inline void xpowfast(F32& o, F32 x, F32 y);
inline void xcosfast(F32& fResult, F32 fAngle);

inline void xsinfast(F32& fResult, F32 fAngle)
{
    F32 fASqr = fAngle * fAngle;
    fResult = 0.00761f;
    fResult *= fASqr;
    fResult -= 0.16605f;
    fResult *= fASqr;
    fResult += 1.0f;
    fResult *= fAngle;
}

inline void xtanfast(F32& fResult, F32 fAngle);
inline void xacosfast(F32& fResult, F32 fValue);
inline void xasinfast(F32& fResult, F32 fValue);
inline void xatanfast(F32& fValue, F32);
inline void xcoshfast(F32& o, F32 x);
inline void xsinhfast(F32& o, F32 x);
inline void xtanhfast(F32& o, F32 x);
inline void xatan2fast(F32& o, F32 y, F32 x);

// RESTORED: speculative
inline void xabsfast(F32& o, F32 x)
{
    o = std::fabsf(x);
}
// END RESTORED

inline void xminfast(F32& o, F32 a, F32 b);
inline void xmin3fast(F32& o, F32 a, F32 b, F32 c);
inline void xmaxfast(F32& o, F32 a, F32 b);
inline void xmax3fast(F32& o, F32 a, F32 b, F32 c);

// RESTORED: speculative
inline F32 xinv(register F32 fVal)
{
    register F32 fRetVal;

    xASSERT(fVal);

    asm {
        fres fRetVal, fVal
    }

    return fRetVal;
}
// END RESTORED

inline F32 xinvsqrt(register F32 fVal)
{
    register F32 fRetVal;

    xASSERT_L(510, fVal > 0.0f);

    asm {
        frsqrte fRetVal, fVal
    }

    return fRetVal;
}

#undef inline

#endif
