#ifndef XVEC3INLINES_H
#define XVEC3INLINES_H

#include "xDebug.h"

#ifdef DEBUG
#define INLINE
#else
#define INLINE inline
#endif

INLINE void xVec3Cross(xVec3* o, const xVec3* a, const xVec3* b)
{
    xASSERTMSG(287, o != a && o != b, "Sorry, but the input vector can't be the same as the output vector. You'll get the wrong answer");
    o->x = a->y * b->z - b->y * a->z,
    o->y = a->z * b->x - b->z * a->x,
    o->z = a->x * b->y - b->x * a->y;
}

#endif
