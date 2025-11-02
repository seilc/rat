#ifndef XVEC3INLINES_H
#define XVEC3INLINES_H

#ifdef DEBUG
#include "xVec3.h"
#endif

#include "xDebug.h"

#ifdef DEBUG
#define INLINE
#else
#define INLINE inline
#endif

INLINE void xVec3Cross(xVec3* o, const xVec3* a, const xVec3* b)
{
    xASSERTM(287, o != a && o != b, "Sorry, but the input vector can't be the same as the output vector. You'll get the wrong answer");
    o->x = a->y * b->z - b->y * a->z,
    o->y = a->z * b->x - b->z * a->x,
    o->z = a->x * b->y - b->x * a->y;
}

INLINE F32 xVec3Hdng(xVec3* hdng, const xVec3* a, const xVec3* b)
{
    F32 d;
    F32 dx__ = b->x - a->x;
    F32 dy__ = b->y - a->y;
    F32 dz__ = b->z - a->z;
    F32 dist2 = dx__*dx__ + dy__*dy__ + dz__*dz__;
    if (xeq(dist2, 1.0f, 0.00001f)) {
        hdng->x = dx__;
        hdng->y = dy__;
        hdng->z = dz__;
        d = 1.0f;
    } else if (xeq(dist2, 0.0f, 0.00001f)) {
        hdng->x = 0.0f;
        hdng->y = 1.0f;
        hdng->z = 0.0f;
        d = 0.0f;
    } else {
        d = xsqrt(dist2);
        F32 dist_inv = 1.0f / d;
        hdng->x = dx__ * dist_inv;
        hdng->y = dy__ * dist_inv;
        hdng->z = dz__ * dist_inv;
    }
    return d;
}

#endif
