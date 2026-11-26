#ifndef ZNPCHELPER_H
#define ZNPCHELPER_H

#include "xVec3.h"

class zNPCHelper
{
public:
    static void RotateLocalToWorld(xVec3& oWorld, const xVec3& iLocal, const xVec3& iEntityHeading)
    {
        oWorld.x = iEntityHeading.z * iLocal.x + iEntityHeading.x * iLocal.z;
        oWorld.y = iLocal.y;
        oWorld.z = iEntityHeading.z * iLocal.z - iEntityHeading.x * iLocal.x;
    }

    static void RotateWorldToLocal(xVec3& oLocal, const xVec3& iWorld, const xVec3& iEntityHeading)
    {
        oLocal.x = iEntityHeading.z * iWorld.x - iEntityHeading.x * iWorld.z;
        oLocal.y = iWorld.y;
        oLocal.z = iEntityHeading.x * iWorld.x + iEntityHeading.z * iWorld.z;
    }
};

#endif
