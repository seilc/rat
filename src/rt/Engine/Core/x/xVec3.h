#ifndef XVEC3_H
#define XVEC3_H

#include "xMath.h"

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
    static const xVec3 m_UnitAxisX;
    static const xVec3 m_UnitAxisY;
    static const xVec3 m_UnitAxisZ;

    xVec3& assign(F32 x, F32 y, F32 z);
    xVec3& assign(F32 v);
    xVec3& operator+=(const xVec3& v);
    xVec3& operator-=(const xVec3& v);
    xVec3& operator*=(F32 f);
    xVec3& operator=(F32 f) { x = y = z = f; return *this; }
    F32 dot(const xVec3& c) const;
    F32 length2() const;
    F32 up_normalize();
    void AddScale(const xVec3& d, F32 s);
    void Sub(const xVec3& a, const xVec3& b);
    void Add(const xVec3& a);
    void Scale(F32 scalar);
    F32 Distance2(const xVec3& other) const;
};

#define xVec3NormalizeMacro(o, v, outlen)                                                          \
do {                                                                                               \
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
} while (0)

F32 xVec3Normalize(xVec3* o, const xVec3* v);
void xVec3Copy(xVec3* o, const xVec3* v);
F32 xVec3Dot(const xVec3* vec1, const xVec3* vec2);

void xVec3AddTo(xVec3& a, const xVec3& b);
void xVec3AddTo(xVec3* a, const xVec3* b);
void xVec3Sub(xVec3* o, const xVec3* a, const xVec3* b);
void xVec3Sub(xVec3& o, const xVec3& a, const xVec3& b);
void xVec3SMul(xVec3* o, const xVec3* v, F32 s);
void xVec3SMulBy(xVec3& v, F32 s);
void xVec3Cross(xVec3* o, const xVec3* a, const xVec3* b);
F32 xVec3Length2(const xVec3* v);
void xVec3Inv(xVec3& v);
F32 xVec3Hdng(xVec3* hdng, const xVec3* a, const xVec3* b);
F32 xVec3Dist2(const xVec3* a, const xVec3* b);

#ifndef DEBUG
#include "xVec3Inlines.h"
#endif

#endif
