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

    xVec3& assign(F32 x, F32 y, F32 z);
    void Sub(const xVec3& a, const xVec3& b);
};

F32 xVec3Dot(const xVec3* vec1, const xVec3* vec2);

void xVec3AddTo(xVec3* a, const xVec3* b);
void xVec3Sub(xVec3* o, const xVec3* a, const xVec3* b);
void xVec3SMul(xVec3* o, const xVec3* v, F32 s);
void xVec3Cross(xVec3* o, const xVec3* a, const xVec3* b);
F32 xVec3Length2(const xVec3* v);

#ifndef DEBUG
#include "xVec3Inlines.h"
#endif

#endif
