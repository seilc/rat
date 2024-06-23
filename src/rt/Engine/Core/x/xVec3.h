#ifndef XVEC3_H
#define XVEC3_H

#include "types.h"

#include <rwcore.h>

union xVec3
{
    RwV3d m_RwV3d;
    struct
    {
        F32 x;
        F32 y;
        F32 z;
    };
    F32 a[3];
};

#endif