#ifndef ZNPCPERCEPTION_H
#define ZNPCPERCEPTION_H

#include "xEnt.h"

namespace zNPC {
class zNPCCommon;
}

struct zCommonPlayer;

#define zMAX_PERCEPTION_TARGETS 4

class zNPCPerceptionTarget
{
public:
    zNPCPerceptionTarget();
    void Reset();
    void DamagedBy();
    ~zNPCPerceptionTarget();
    void SetEnt(xEnt* inEnt);
    xEnt* GetEnt() { return targetEnt; }
    F32 GetPriority();
    void AddToPriority(const char*, F32 addPriority, F32 importance);
    void CalculateDistance(zNPC::zNPCCommon* owner);
    F32 GetDistance();
    void UpdatePriorityHealth(zNPC::zNPCCommon*, F32 importance);
    void UpdatePriorityDistance(zNPC::zNPCCommon* owner, F32 importance);
    void UpdatePriorityFacing(zNPC::zNPCCommon* owner, F32 importance);
    void UpdatePriorityLineOfSight(zNPC::zNPCCommon*, F32);
    void UpdatePriorityEquallyDivide(zNPC::zNPCCommon*, F32 importance);
    void UpdatePriorityAttackVulnerabilities(zNPC::zNPCCommon*, F32);
    void UpdatePriorityDefenseVulnerabilities(zNPC::zNPCCommon*, F32);
    void UpdatePriorityDamagedBy(zNPC::zNPCCommon*, F32 importance);
    void UpdatePriorityWidget(zNPC::zNPCCommon*, F32 importance);
    void UpdatePriorityInaction(zNPC::zNPCCommon*, F32 importance);
    virtual void UpdatePriority(zNPC::zNPCCommon* owner);
    void UpdateBasics(F32 seconds);

protected:
    xEnt* targetEnt;
    F32 priority;
    F32 distSq;
    F32 damagedByTimer;
#if DEBUG
    char description[128];
#endif
};

class zNPCPerception
{
public:
    zNPCPerception();
    ~zNPCPerception();
    void Reset(zNPC::zNPCCommon* npc);
    virtual void UpdateBasics(F32 seconds);
    virtual void UpdateComplete(F32 seconds, bool force);
    virtual xEnt* GetPriorityTarget();
    virtual bool IsPriorityTargetAPlayer();
    virtual xVec3* GetPriorityTargetPos();
    virtual F32 GetPriorityTargetDistance();
    virtual void DamagedBy(xEnt* damagedByEnt);
    void SetSmashMeWidgetTarget(xEnt* inWidget); // UNUSED
    void SetWidgetTarget(xEnt* inWidget, bool attackForever); // UNUSED
    void ResetWidgetTarget(); // UNUSED
    void PlayerDefeated(zCommonPlayer* player); // UNUSED
    static void ResetCompleteUpdateCount();
    void ForceCompleteUpdate();
    bool CanWeDoCompleteUpdate(F32 seconds);
    void ResetCompleteUpdateTimers();
    void UpdateCompleteDoWork(F32);

protected:
    static S32 completeUpdateCount;

    zNPC::zNPCCommon* owner;
    zNPCPerceptionTarget target[zMAX_PERCEPTION_TARGETS];
    S32 totalTargets;
    xEnt* widgetTargetEnt;
    xEnt* smashMeWidgetTargetEnt;
    F32 completeUpdateTimeSince;
    F32 completeUpdateTimeLeft;
    bool widgetAttackForever;
};

#endif
