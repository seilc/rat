#ifndef ZNPCPERCEPTION_H
#define ZNPCPERCEPTION_H

#include "xEnt.h"

namespace zNPC {
class zNPCCommon;
}

class zNPCPerceptionTarget
{
public:
    virtual void UpdatePriority(zNPC::zNPCCommon* owner);

protected:
    xEnt* targetEnt;
    F32 priority;
    F32 distSq;
    F32 damagedByTimer;
#ifdef DEBUG
    char description[128];
#endif
};

class zNPCPerception
{
public:
    virtual void UpdateBasics(F32 seconds);
    virtual void UpdateComplete(F32 seconds, bool force);
    virtual xEnt* GetPriorityTarget();
    virtual bool IsPriorityTargetAPlayer();
    virtual xVec3* GetPriorityTargetPos();
    virtual F32 GetPriorityTargetDistance();
    virtual void DamagedBy(xEnt* damagedByEnt);

protected:
    zNPC::zNPCCommon* owner;
    zNPCPerceptionTarget target[4];
    S32 totalTargets;
    xEnt* widgetTargetEnt;
    xEnt* smashMeWidgetTargetEnt;
    F32 completeUpdateTimeSince;
    F32 completeUpdateTimeLeft;
    bool widgetAttackForever;
};

#endif
