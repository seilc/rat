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

    void Sub(const xVec3& a, const xVec3& b);
};

#endif