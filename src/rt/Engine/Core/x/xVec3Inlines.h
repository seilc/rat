#ifndef XVEC3INLINES_H
#define XVEC3INLINES_H

#include "xDebug.h"

#if DEBUG
#define inline
#endif

inline void xVec3Init(xVec3* v, F32 _x, F32 _y, F32 _z)
{
    v->x = _x, v->y = _y, v->z = _z;
}

inline void xVec3Init(xVec3& v, F32 _x, F32 _y, F32 _z)
{
    v.x = _x;
    v.y = _y;
    v.z = _z;
}

inline void xVec3FromLatLong(xVec3* v, F32 lat, F32 lng); // UNUSED

inline U32 xVec3Equals(const xVec3* a, const xVec3* b)
{
    return xeq(a->x, b->x, 0.0f) && xeq(a->y, b->y, 0.0f) && xeq(a->z, b->z, 0.0f);
}

inline U32 xVec3EqualsWT(const xVec3* a, const xVec3* b, F32 t); // UNUSED

inline void xVec3AddC(xVec3* o, const xVec3* a, F32 _x, F32 _y, F32 _z)
{
    o->x = a->x + _x, o->y = a->y + _y, o->z = a->z + _z;
}

inline void xVec3Add(xVec3* o, const xVec3* a, const xVec3* b)
{
    o->x = a->x + b->x, o->y = a->y + b->y, o->z = a->z + b->z;
}

inline void xVec3Add(xVec3& o, const xVec3& a, const xVec3& b)
{
    o.x = a.x + b.x;
    o.y = a.y + b.y;
    o.z = a.z + b.z;
}

inline void xVec3Add(xVec3& o, const xVec3& a)
{
    o.x += a.x;
    o.y += a.y;
    o.z += a.z;
}

inline void xVec3AddTo(xVec3& a, const xVec3& b)
{
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
}

inline void xVec3AddTo(xVec3* a, const xVec3* b)
{
    a->x += b->x, a->y += b->y, a->z += b->z;
}

inline void xVec3AddScaled(xVec3* a, const xVec3* b, F32 scale)
{
    a->x += b->x * scale, a->y += b->y * scale, a->z += b->z * scale;
}

inline void xVec3AddScaled(xVec3& a, const xVec3& b, F32 scale)
{
    a.x += b.x * scale;
    a.y += b.y * scale;
    a.z += b.z * scale;
}

inline void xVec3AddScaled(xVec3& o, const xVec3& a, const xVec3& b, F32 scale)
{
    o.x = a.x + b.x * scale;
    o.y = a.y + b.y * scale;
    o.z = a.z + b.z * scale;
}

inline void xVec3Sub(xVec3* o, const xVec3* a, const xVec3* b)
{
    o->x = a->x - b->x, o->y = a->y - b->y, o->z = a->z - b->z;
}

inline void xVec3Sub(xVec3& o, const xVec3& a, const xVec3& b)
{
    o.x = a.x - b.x;
    o.y = a.y - b.y;
    o.z = a.z - b.z;
}

inline void xVec3SubXZ(xVec3& o, const xVec3& a, const xVec3& b); // UNUSED

inline void xVec3SubFrom(xVec3* a, const xVec3* b)
{
    a->x -= b->x, a->y -= b->y, a->z -= b->z;
}

inline void xVec3SubFrom(xVec3& a, const xVec3& b)
{
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
}

inline void xVec3SMul(xVec3* o, const xVec3* v, F32 s)
{
    o->x = v->x * s, o->y = v->y * s, o->z = v->z * s;
}

inline void xVec3SMul(xVec3& o, const xVec3& v, F32 s)
{
    o.x = v.x * s;
    o.y = v.y * s;
    o.z = v.z * s;
}

inline void xVec3SMulBy(xVec3* v, F32 s)
{
    v->x *= s, v->y *= s, v->z *= s;
}

inline void xVec3SMulBy(xVec3& v, F32 s)
{
    v.x *= s;
    v.y *= s;
    v.z *= s;
}

inline void xVec3Cross(xVec3* o, const xVec3* a, const xVec3* b)
{
    xASSERT_FMT_L(287, o != a && o != b, "Sorry, but the input vector can't be the same as the output vector. You'll get the wrong answer");
    o->x = a->y * b->z - b->y * a->z, o->y = a->z * b->x - b->z * a->x, o->z = a->x * b->y - b->x * a->y;
}

inline void xVec3Cross(xVec3& o, const xVec3& a, const xVec3& b)
{
    xASSERT_FMT_L(292, &o != &a && &o != &b, "Sorry, but the input vector can't be the same as the output vector. You'll get the wrong answer");
    o.x = a.y * b.z - b.y * a.z, o.y = a.z * b.x - b.z * a.x, o.z = a.x * b.y - b.x * a.y;
}

inline F32 xVec3Length(F32 _x, F32 _y, F32 _z)
{
    return xsqrt(_x * _x + _y * _y + _z * _z);
}

inline F32 xVec3Length(const xVec3* v)
{
    return xsqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

inline F32 xVec3LengthFast(F32 _x, F32 _y, F32 _z)
{
    F32 len;
    xsqrtfast(len, _x * _x + _y * _y + _z * _z);
    return len;
}

inline F32 xVec3LengthFast(const xVec3* v); // UNUSED
inline F32 xVec3Length2(F32 _x, F32 _y, F32 _z); // UNUSED

inline F32 xVec3Length2(const xVec3* v)
{
    return v->x * v->x + v->y * v->y + v->z * v->z;
}

inline void xVec3Inv(xVec3* o, const xVec3* v)
{
    o->x = -v->x, o->y = -v->y, o->z = -v->z;
}

inline void xVec3Inv(xVec3& o, const xVec3& v)
{
    o.x = -v.x;
    o.y = -v.y;
    o.z = -v.z;
}

inline void xVec3Inv(xVec3& v)
{
    v.x = -v.x;
    v.y = -v.y;
    v.z = -v.z;
}

inline void xVec3ScaleC(xVec3* o, const xVec3* v, F32 _x, F32 _y, F32 _z); // UNUSED

inline void xVec3ScaleC(xVec3& o, F32 scale)
{
    o.x *= scale;
    o.y *= scale;
    o.z *= scale;
}

inline void xVec3ScaleC(xVec3& o, const xVec3& v, F32 scale)
{
    o.x = v.x * scale;
    o.y = v.y * scale;
    o.z = v.z * scale;
}

inline void xVec3Scale(xVec3* o, const xVec3* v, const xVec3* s); // UNUSED

inline F32 xVec3Hdng(xVec3* hdng, const xVec3* a, const xVec3* b)
{
    F32 d;
    xVec3HdngMacro(hdng, a, b, &d);
    return d;
}

inline F32 xVec3Dist(F32 ax, F32 ay, F32 az, F32 bx, F32 by, F32 bz); // UNUSED
inline F32 xVec3DistFast(F32 ax, F32 ay, F32 az, F32 bx, F32 by, F32 bz); // UNUSED
inline F32 xVec3Dist2(F32 ax, F32 ay, F32 az, F32 bx, F32 by, F32 bz); // UNUSED

inline F32 xVec3Dist(const xVec3* a, const xVec3* b)
{
    F32 d;
    xVec3DistMacro(a, b, &d);
    return d;
}

inline F32 xVec3DistFast(const xVec3* a, const xVec3* b)
{
    F32 d;
    xVec3DistFastMacro(a, b, &d);
    return d;
}

inline F32 xVec3Dist2(const xVec3* a, const xVec3* b)
{
    F32 d;
    xVec3Dist2Macro(a, b, &d);
    return d;
}

inline void xVec3Lerp(xVec3* o, const xVec3* a, const xVec3* b, F32 f)
{
    o->x = a->x * (1.0f - f) + b->x * f, o->y = a->y * (1.0f - f) + b->y * f, o->z = a->z * (1.0f - f) + b->z * f;
}

inline F32 xVec3NormalizeSafe(xVec3& v)
{
    F32 len_squ = v.length2();
    if (len_squ < 9.9999994e-11f) {
        return 0.0f;
    }
    len_squ = xsqrt(len_squ);
    xVec3ScaleC(v, 1.0f / len_squ);
    return len_squ;
}

inline F32 xVec3NormalizeSafe(xVec3& o, const xVec3& v)
{
    F32 len_squ = v.length2();
    if (len_squ < 9.9999994e-11f) {
        return 0.0f;
    }
    len_squ = xsqrt(len_squ);
    xVec3ScaleC(o, v, 1.0f / len_squ);
    return len_squ;
}

#undef inline

#endif
