#ifndef ZCONTEXTOBJECTCLIMBABLEROPE_H
#define ZCONTEXTOBJECTCLIMBABLEROPE_H

#include "xDynAsset.h"
#include "xEnt.h"
#include "zContextObject.h"

struct RwRaster;

class zCollidable : public xEnt
{
protected:
    xMat4x3 collisionMat;
};

class zContextObjectClimbableRope : public zContextObject
{
public:
    struct Asset : xDynAsset
    {
        U32 mountObjectID;
        F32 length;
        F32 yaw;
        U32 textureID;
        F32 textureLength;
    };

#if DEBUG || RELEASE
    static void AddStaticTweaks();
#endif

    bool Ascend(F32 dt);
    bool Descend(F32 dt);
    void SwingInput(F32 stickMagnitude);
    void UpdateClimberFrame();
    void ClearCollisionFlags();
    void RestoreCollisionFlags();

protected:
    zCollidable collidables[2];
    Asset* asset;
    RwRaster* raster;
    union
    {
        xEnt* ent;
        xVec3 pos;
    };
    F32 swingAngle;
    F32 swingAngleVel;
    F32 swingInput;
    xMat3x3 orientation;
    xEnt* climberEnt;
    F32 climberRopePos;
    xVec3 climberHandPos;
    bool attachEnt;
};

#endif
