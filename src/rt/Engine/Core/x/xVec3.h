#ifndef XVEC3_H
#define XVEC3_H

#include "xMath.h"
#include "xDebug.h"

#include <rwcore.h>

struct xVec3
{
    union
    {
        RwV3d m_RwV3d;
        struct
        {
            F32 x, y, z;
        };
        F32 a[3];
    };

    static const xVec3 m_Null;
    static const xVec3 m_Ones;
    static const xVec3 m_NegOnes;
    static const xVec3 m_UnitAxisX;
    static const xVec3 m_UnitAxisY;
    static const xVec3 m_UnitAxisZ;
    static const xVec3 m_HalfVec;
    static const xVec3 m_NegHalfVec;
    static const xVec3 m_DoubleVec;
    static const xVec3 m_NegDoubleVec;

    xVec3& assign(F32 x, F32 y, F32 z) { this->x = x; this->y = y; this->z = z; return *this; }
    xVec3& assign(F32 v);
    xVec3& operator+=(const xVec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    xVec3& operator-=(const xVec3& v);
    xVec3& operator*=(F32 f) { x *= f; y *= f; z *= f; return *this; }
    xVec3& operator=(F32 f) { x = y = z = f; return *this; }

    xVec3& cross(const xVec3& c0, const xVec3& c1)
    {
        x = c0.y * c1.z - c1.y * c0.z;
        y = c0.z * c1.x - c1.z * c0.x;
        z = c0.x * c1.y - c1.x * c0.y;
        return *this;
    }

    F32 dot(const xVec3& c) const { return x * c.x + y * c.y + z * c.z; }
    F32 length() const { return xsqrt(length2()); }
    F32 length2() const { return x * x + y * y + z * z; }
    xVec3& negate();

    F32 normalize()
    {
        F32 len2 = length2();
        xASSERT_L(86, len2 > 0.f);

        F32 len = xsqrt(len2);
        *this *= 1.0f / len;
        
        return len;
    }

    F32 safe_normalize(const xVec3& safety)
    {
        F32 len = length();
        if (len < 0.00001f)
            *this = safety;
        else
            *this *= 1.0f / len;
        return len;
    }

    F32 up_normalize();
    void AddScale(const xVec3& p, const xVec3& d, F32 s);
    void AddScale(const xVec3& d, F32 s);
    void Sub(const xVec3& a, const xVec3& b);
    void ScaleDiff(const xVec3& a, const xVec3& b, F32 scale);
    void Add(const xVec3& a);
    void Add(const xVec3& a, const xVec3& b);
    void Scale(F32 scalar);
    void Scale(const xVec3& a, F32 scalar);
    F32 Distance(const xVec3& other) const;
    F32 Distance2(const xVec3& other) const;
    void Negate(const xVec3& a);
    void SubAdd(const xVec3& a, const xVec3& b, const xVec3& c);
    void ScaleComponents(const xVec3& scale);
    void Combine(const xVec3& a, F32 scale0, const xVec3& b, F32 scale1);
};

#define xVec3NormalizeMacro(o, v, outlen)                                                          \
{                                                                                                  \
    F32 len2 = xsqr((v)->x) + xsqr((v)->y) + xsqr((v)->z);                                         \
    if (xeq(len2, 1.0f, EPSILON)) {                                                                \
        (o)->x = (v)->x;                                                                           \
        (o)->y = (v)->y;                                                                           \
        (o)->z = (v)->z;                                                                           \
        *(outlen) = 1.0f;                                                                          \
    } else if (xeq(len2, 0.0f, EPSILON)) {                                                         \
        (o)->x = 0.0f;                                                                             \
        (o)->y = 1.0f;                                                                             \
        (o)->z = 0.0f;                                                                             \
        *(outlen) = 0.0f;                                                                          \
    } else {                                                                                       \
        *(outlen) = xsqrt(len2);                                                                   \
        F32 len_inv = 1.0f / *(outlen);                                                            \
        (o)->x = (v)->x * len_inv;                                                                 \
        (o)->y = (v)->y * len_inv;                                                                 \
        (o)->z = (v)->z * len_inv;                                                                 \
    }                                                                                              \
}

F32 xVec3Normalize(xVec3* o, const xVec3* v);
F32 xVec3NormalizeFast(xVec3* o, const xVec3* v);
void xVec3Copy(xVec3* o, const xVec3* v);
F32 xVec3Dot(F32 ax, F32 ay, F32 az, F32 bx, F32 by, F32 bz);
F32 xVec3Dot(const xVec3* vec1, const xVec3* vec2);

void xVec3Init(xVec3& v, F32 _x, F32 _y, F32 _z);
void xVec3Add(xVec3* o, const xVec3* a, const xVec3* b);
void xVec3AddTo(xVec3& a, const xVec3& b);
void xVec3AddTo(xVec3* a, const xVec3* b);
void xVec3Sub(xVec3* o, const xVec3* a, const xVec3* b);
void xVec3Sub(xVec3& o, const xVec3& a, const xVec3& b);
void xVec3SMul(xVec3* o, const xVec3* v, F32 s);
void xVec3SMulBy(xVec3& v, F32 s);
void xVec3Cross(xVec3* o, const xVec3* a, const xVec3* b);
void xVec3Cross(xVec3& o, const xVec3& a, const xVec3& b);
F32 xVec3Length(const xVec3* v);
F32 xVec3Length2(const xVec3* v);
void xVec3Inv(xVec3& v);
void xVec3ScaleC(xVec3& o, F32 scale);
F32 xVec3Hdng(xVec3* hdng, const xVec3* a, const xVec3* b);
F32 xVec3Dist2(const xVec3* a, const xVec3* b);
F32 xVec3NormalizeSafe(xVec3& v);

inline void xVec3::AddScale(const xVec3& d, F32 s)
{
    x += d.x * s;
    y += d.y * s;
    z += d.z * s;
}

inline void xVec3::Sub(const xVec3& a, const xVec3& b)
{
    x = a.x - b.x;
    y = a.y - b.y;
    z = a.z - b.z;
}

inline void xVec3::SubAdd(const xVec3& a, const xVec3& b, const xVec3& c)
{
    x = a.x - b.x + c.x;
    y = a.y - b.y + c.y;
    z = a.z - b.z + c.z;
}

inline void xVec3::ScaleDiff(const xVec3& a, const xVec3& b, F32 scale)
{
    x = scale * (a.x - b.x);
    y = scale * (a.y - b.y);
    z = scale * (a.z - b.z);
}

inline void xVec3::Add(const xVec3& a, const xVec3& b)
{
    x = a.x + b.x;
    y = a.y + b.y;
    z = a.z + b.z;
}

inline void xVec3::Add(const xVec3& a)
{
    x += a.x;
    y += a.y;
    z += a.z;
}

inline void xVec3::Scale(F32 scalar)
{
    x *= scalar;
    y *= scalar;
    z *= scalar;
}

inline void xVec3::Scale(const xVec3& a, F32 scalar)
{
    x = a.x * scalar;
    y = a.y * scalar;
    z = a.z * scalar;
}

inline void xVec3::ScaleComponents(const xVec3& scale)
{
    x *= scale.x;
    y *= scale.y;
    z *= scale.z;
}

inline F32 xVec3::Distance(const xVec3& other) const
{
    xVec3 diff;
    diff.Sub(other, *this);
    return diff.length();
}

inline F32 xVec3::Distance2(const xVec3& other) const
{
    xVec3 diff;
    diff.Sub(other, *this);
    return diff.length2();
}

inline void xVec3::Combine(const xVec3& a, F32 scale0, const xVec3& b, F32 scale1)
{
    x = a.x * scale0 + b.x * scale1;
    y = a.y * scale0 + b.y * scale1;
    z = a.z * scale0 + b.z * scale1;
}

#if !DEBUG
#include "xVec3Inlines.h"
#endif

#endif
