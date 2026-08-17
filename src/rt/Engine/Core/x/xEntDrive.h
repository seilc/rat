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

struct xEntDriveSimple
{
    xMat4x3* drivenMat;
    xEnt* driver;
    xVec3 drivenPosInDriver;
    xVec3 lastDrivenPos;
    xVec3 dpos;
};

void xEntDriveInit(xEntDrive* drv, xEnt* driven);
void xEntDriveMount(xEntDrive* drv, xEnt* driver, F32 mt, const xCollis* coll);
void xEntDriveDismount(xEntDrive* drv, F32 dmt);
void xEntDriveUpdate(xEntDrive* drv, xScene*, F32 dt, const xCollis*);
void xEntDriveSimpleInit(xEntDriveSimple* drv, xMat4x3* mat);
void xEntDriveSimpleMount(xEntDriveSimple* drv, xEnt* driver);
void xEntDriveSimpleUpdate(xEntDriveSimple* drv, F32);
void xEntDriveSimplePostUpdate(xEntDriveSimple* drv);

#endif
