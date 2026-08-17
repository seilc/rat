#ifndef XCOLLIDE_H
#define XCOLLIDE_H

#include "xModel.h"

struct xCollis
{
    struct tri_data
    {
        U32 index;
        F32 r;
        F32 d;

        // Not present in DWARF, so probably a compiler-generated function, but somehow also appears in its own .text section?
        // Needed to match xEntDrive.cpp.
        tri_data& operator=(const tri_data& tri)
        {
            index = tri.index;
            r = tri.r;
            d = tri.d;
            return *this;
        }
    };

    U32 flags;
    U32 oid;
    void* optr;
    xModelInstance* mptr;
    F32 dist;
    F32 test_dist;
    xVec3 norm;
    xVec3 tohit;
    xVec3 depen;
    xVec3 hdng;
    union
    {
        struct
        {
            F32 t;
            F32 u;
            F32 v;
        } tuv;
        tri_data tri;
    };
};

#define k_HIT_IT ((U32)(1 << 0))
#define k_HIT_CALC_HDNG ((U32)(1 << 12))
#define k_HIT_CALC_TRI ((U32)(1 << 13))

xVec3 xCollisTriHit(const xCollis::tri_data& tri, const xModelInstance& model);

#endif
