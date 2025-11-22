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

    xVec3& assign(F32 x, F32 y, F32 z);
    xVec3& operator-=(const xVec3& v);
    xVec3& operator*=(F32 f);
    F32 dot(const xVec3& c) const;
    F32 up_normalize();
    void AddScale(const xVec3& d, F32 s);
    void Sub(const xVec3& a, const xVec3& b);
    F32 Distance2(const xVec3& other) const;
};

F32 xVec3Normalize(xVec3* o, const xVec3* v);
F32 xVec3Dot(const xVec3* vec1, const xVec3* vec2);

void xVec3AddTo(xVec3* a, const xVec3* b);
void xVec3Sub(xVec3* o, const xVec3* a, const xVec3* b);
void xVec3SMul(xVec3* o, const xVec3* v, F32 s);
void xVec3Cross(xVec3* o, const xVec3* a, const xVec3* b);
F32 xVec3Length2(const xVec3* v);
F32 xVec3Hdng(xVec3* hdng, const xVec3* a, const xVec3* b);
F32 xVec3Dist2(const xVec3* a, const xVec3* b);

#ifndef DEBUG
#include "xVec3Inlines.h"
#endif

#endif
