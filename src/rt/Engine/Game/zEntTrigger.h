#ifndef ZENTTRIGGER_H
#define ZENTTRIGGER_H

#include "zEnt.h"
#include "xEntDrive.h"
#include "xTriggerAsset.h"

class xCamTransition;

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

#if DEBUG || RELEASE
void zEntTriggerDebugDraw(xBase* btrig);
#endif

#endif
