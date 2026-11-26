#ifndef ZNPCHELPER_H
#define ZNPCHELPER_H

#include "xVec3.h"

class zNPCHelper
{
public:
    static void RotateWorldToLocal(xVec3& oLocal, const xVec3& iWorld, const xVec3& iEntityHeading)
    {
        oLocal.x = iEntityHeading.z * iWorld.x - iEntityHeading.x * iWorld.z;
        oLocal.y = iWorld.y;
        oLocal.z = iEntityHeading.x * iWorld.x + iEntityHeading.z * iWorld.z;
    }
};

#endif
