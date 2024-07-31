#ifndef XENTDRIVE_H
#define XENTDRIVE_H

#include "xEnt.h"

struct xEntDrive
{
    struct triData : xCollis::tri_data
    {
        xVec3 loc;
        F32 yaw;
        const xCollis* coll;
        xMat4x3 trioldmat;
    };

    U32 flags;
    F32 outroTime;
    F32 outroTimer;
    F32 influenceOfOldDriver;
    F32 introTime;
    F32 introTimer;
    F32 influenceOfDriver;
    xEnt* oldDriver;
    xEnt* driver;
    xEnt* driven;
    xVec3 drivenPosInOldDriver;
    xVec3 drivenPosInDriver;
    xVec3 lastDrivenPos;
    F32 yawInDriver;
    xVec3 dLoc;
    triData tri;
};

#endif
