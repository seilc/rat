#ifndef ZHANGABLEASSET_H
#define ZHANGABLEASSET_H

#include "xDynAsset.h"
#include "xVec3.h"

struct zHangableAsset : xDynAsset
{
    enum { VERSION = 1 };

    U32 object;
    xVec3 pivot;
    xVec3 handle;
    F32 onGravity;
    F32 offGravity;
    F32 maxAngVel;
    F32 minArcDegrees;
    U32 hangFlags;
};

#endif
