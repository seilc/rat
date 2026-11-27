#ifndef ZENTTRIGGER_H
#define ZENTTRIGGER_H

#include "zEnt.h"
#include "xEntDrive.h"
#include "xTriggerAsset.h"

class xCamTransition;

enum
{
    eEntTriggerTypeBox
};

struct zEntTrigger : zEnt
{
    struct JumpTriggerExtra
    {
        xPlane faces[4];
        S32 jumpFaces[4];
    };

    xBox triggerBox;
    U32 entered;
    union
    {
        xEntDrive drive;
        JumpTriggerExtra jumpTriggerExtra;
        xCamTransition* camTransition;
    };

    bool PointIntersects(const xVec3& chkPos) const;

    xTriggerAsset& Asset() const;
};

class zSpecialTriggersTree
{
public:
    static zEntTrigger* IntersectingSphere(const xVec3& pos, F32 radius, U16 triggerFilter, U16 entFilter);
};

#if DEBUG || RELEASE
void zEntTriggerDebugDraw(xBase* btrig);
#endif

#endif
