#include "xCommon.h"
#include "zNPCPerception.h"

#include "xEntMotion.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xstransvc.h"
#include "zBase.h"
#include "zCommonPlayer.h"
#include "zNPCCommon.h"

#include <string.h>

static void CLAMP021(F32 min, F32* test, F32 max)
{
    if (*test < min)
        *test = min;
    if (*test > max)
        *test = max;
}

zNPCPerceptionTarget::zNPCPerceptionTarget()
{
    targetEnt = NULL;
    priority = 0.0f;
    distSq = 0.0f;
    damagedByTimer = 0.0f;
}

void zNPCPerceptionTarget::Reset()
{
    targetEnt = NULL;
    priority = 0.0f;
    distSq = 0.0f;
    damagedByTimer = 0.0f;
}

void zNPCPerceptionTarget::DamagedBy()
{
    damagedByTimer = 10.0f;
}

zNPCPerceptionTarget::~zNPCPerceptionTarget()
{
}

void zNPCPerceptionTarget::SetEnt(xEnt* inEnt)
{
    targetEnt = inEnt;
#if DEBUG
    const char* name = xSTAssetName(targetEnt);
    if (name) {
        strcpy(description, name);
    }
#endif
}

F32 zNPCPerceptionTarget::GetPriority()
{
    return priority;
}

void zNPCPerceptionTarget::AddToPriority(const char*, F32 addPriority, F32 importance)
{
    priority += addPriority * importance;
}

void zNPCPerceptionTarget::CalculateDistance(zNPC::zNPCCommon* owner)
{
    xASSERT_L(113, targetEnt);
    xVec3* targetPos;
    if (targetEnt->baseType == eBaseTypeNPC)
        targetPos = xEntGetPos(targetEnt);
    else
        targetPos = xEntGetPos(targetEnt);
    xVec3* ourPos = xEntGetPos(owner);
    xASSERT_L(123, targetPos);
    xASSERT_L(124, ourPos);
    distSq = ourPos->Distance2(*targetPos);
}

F32 zNPCPerceptionTarget::GetDistance()
{
    return distSq;
}

void zNPCPerceptionTarget::UpdatePriorityHealth(zNPC::zNPCCommon*, F32 importance)
{
    F32 hpPriority = 0.0f;
    if (targetEnt->baseType == eBaseTypePlayer) {
        zCommonPlayer* player = (zCommonPlayer*)targetEnt;
        hpPriority = player->GetHealthPercentage();
        if (player->IsDead())
            hpPriority = -10.0f;
        else
            CLAMP021(0.0f, &hpPriority, 1.0f);
    }
    AddToPriority("Health", hpPriority, importance);
}

void zNPCPerceptionTarget::UpdatePriorityDistance(zNPC::zNPCCommon* owner, F32 importance)
{
    F32 distPriority, maxRange = owner->WeaponGetMaxRange();
    maxRange *= maxRange;
    if (distSq <= maxRange)
        distPriority = 1.0f;
    else {
        F32 tempPriority = distSq / 900.0f;
        CLAMP021(0.0f, &tempPriority, 1.0f);
        distPriority = 1.0f - tempPriority;
    }
    AddToPriority("Distance", distPriority, importance);
}

void zNPCPerceptionTarget::UpdatePriorityFacing(zNPC::zNPCCommon* owner, F32 importance)
{
    xVec3* ownerDir = xEntGetFacingDirection(owner);
    xVec3* targetPos = xEntGetPos(targetEnt);
    xVec3* ownerPos = xEntGetPos(owner);
    xVec3 toTargetDir;
    toTargetDir.Sub(*targetPos, *ownerPos);
    F32 facingPriority = 1.0f;
    if (toTargetDir.length2() > 0.0f) {
        toTargetDir.normalize();
        facingPriority = toTargetDir.dot(*ownerDir);
        CLAMP021(0.0f, &facingPriority, 1.0f);
    }
    AddToPriority("Facing", facingPriority, importance);
}

void zNPCPerceptionTarget::UpdatePriorityLineOfSight(zNPC::zNPCCommon*, F32)
{
}

void zNPCPerceptionTarget::UpdatePriorityEquallyDivide(zNPC::zNPCCommon*, F32 importance)
{
    S32 playerSize = CURPLAYERS.size();
    S32 ourTotal = 0;
    S32 otherTotal = 0;
    for (S32 i = 0; i < playerSize; i++) {
        if (CURPLAYERS[i] == targetEnt)
            ourTotal = CURPLAYERS[i]->NPCAttackerGetTotal(ezNPCAT_All);
        else
            otherTotal = CURPLAYERS[i]->NPCAttackerGetTotal(ezNPCAT_All);
    }
    S32 diff = otherTotal - ourTotal;
    F32 maxDiff = 5.0f, dividePriority = diff / maxDiff;
    CLAMP021(0.0f, &dividePriority, 1.0f);
    AddToPriority("EquallyDivide", dividePriority, importance);
}

void zNPCPerceptionTarget::UpdatePriorityAttackVulnerabilities(zNPC::zNPCCommon*, F32)
{
}

void zNPCPerceptionTarget::UpdatePriorityDefenseVulnerabilities(zNPC::zNPCCommon*, F32)
{
}

void zNPCPerceptionTarget::UpdatePriorityDamagedBy(zNPC::zNPCCommon*, F32 importance)
{
    F32 damagedByPriority = 0.1f * damagedByTimer;
    AddToPriority("DamagedBy", damagedByPriority, importance);
}

void zNPCPerceptionTarget::UpdatePriorityWidget(zNPC::zNPCCommon*, F32 importance)
{
    F32 widgetPriority = 0.0f;
    if (targetEnt->baseType != eBaseTypePlayer) {
        widgetPriority = 1.0f;
    }
    AddToPriority("Widget", widgetPriority, importance);
}

void zNPCPerceptionTarget::UpdatePriorityInaction(zNPC::zNPCCommon*, F32 importance)
{
    F32 inactionPriority = 0.0f;
    if (targetEnt->baseType == eBaseTypePlayer) {
        zPlayerInput playerInput = ((zPlayer*)targetEnt)->GetPlayerInput();
        inactionPriority = playerInput.GetActionTimer() / 30.0f;
        CLAMP021(0.0f, &inactionPriority, 1.0f);
        inactionPriority = 1.0f - inactionPriority;
    }
    AddToPriority("Inaction", inactionPriority, importance);
}

void zNPCPerceptionTarget::UpdatePriority(zNPC::zNPCCommon* owner)
{
    priority = 0.0f;
    UpdatePriorityHealth(owner, 0.4f);
    UpdatePriorityDistance(owner, 0.75f);
    UpdatePriorityFacing(owner, 0.1f);
    UpdatePriorityLineOfSight(owner, 0.25f);
    UpdatePriorityEquallyDivide(owner, 0.1f);
    UpdatePriorityAttackVulnerabilities(owner, 0.25f);
    UpdatePriorityDefenseVulnerabilities(owner, 0.25f);
    UpdatePriorityDamagedBy(owner, 5.0f);
    UpdatePriorityWidget(owner, 5.0f);
    UpdatePriorityInaction(owner, 0.15f);
}

void zNPCPerceptionTarget::UpdateBasics(F32 seconds)
{
    if (damagedByTimer > 0.0f) {
        damagedByTimer -= seconds;
        if (damagedByTimer < 0.0f)
            damagedByTimer = 0.0f;
    }
}

zNPCPerception::zNPCPerception()
{
    totalTargets = 0;
    owner = NULL;
}

zNPCPerception::~zNPCPerception()
{
}

void zNPCPerception::Reset(zNPC::zNPCCommon* npc)
{
    totalTargets = 0;
    owner = npc;
    for (S32 i = 0; i < zMAX_PERCEPTION_TARGETS; i++)
        target[i].Reset();
}

void zNPCPerception::UpdateBasics(F32 seconds)
{
    for (S32 i = 0; i < totalTargets; i++)
        target[i].UpdateBasics(seconds);
}

void zNPCPerception::DamagedBy(xEnt* damagedByEnt)
{
    if (widgetTargetEnt && !widgetAttackForever) {
        widgetTargetEnt = NULL;
    }
    if (smashMeWidgetTargetEnt) {
        smashMeWidgetTargetEnt = NULL;
    }
    if (damagedByEnt) {
        for (S32 i = 0; i < totalTargets; i++) {
            if (target[i].GetEnt() == damagedByEnt) {
                target[i].DamagedBy();
                UpdateCompleteDoWork(0.016666668f);
                ResetCompleteUpdateTimers();
                break;
            }
        }
    }
}

S32 zNPCPerception::completeUpdateCount;

void zNPCPerception::ResetCompleteUpdateCount()
{
    completeUpdateCount = 3;
}

void zNPCPerception::ForceCompleteUpdate()
{
    completeUpdateTimeLeft = 0.0f;
}

bool zNPCPerception::CanWeDoCompleteUpdate(F32 seconds)
{
    completeUpdateTimeLeft -= seconds;
    if (completeUpdateTimeLeft <= 0.0f) {
        completeUpdateTimeSince += seconds;
        completeUpdateTimeLeft = 0.0f;
        if (completeUpdateCount > 0) {
            completeUpdateCount--;
            return true;
        }
        if (completeUpdateTimeSince >= 5.0f) {
            return true;
        }
    }
    return false;
}

void zNPCPerception::ResetCompleteUpdateTimers()
{
    completeUpdateTimeSince = 0.0f;
    F32 dist2Target = 0.0f;
    if (totalTargets > 0) {
        dist2Target = target[0].GetDistance();
        for (S32 i = 1; i < totalTargets; i++) {
            F32 tempDist = target[i].GetDistance();
            if (tempDist < dist2Target)
                dist2Target = tempDist;
        }
    }
    dist2Target = xsqrt(dist2Target);
    completeUpdateTimeLeft = 0.1f * dist2Target;
    if (completeUpdateTimeLeft > 5.0f)
        completeUpdateTimeLeft = 5.0f;
    else if (completeUpdateTimeLeft < 0.75f)
        completeUpdateTimeLeft = 0.75f;
}

void zNPCPerception::UpdateCompleteDoWork(F32)
{
    if (owner->manager.getCurrentBehavior()->disableTargetTest()) {
        totalTargets = 0;
        return;
    }

    zNPCPerceptionTarget oldTarget[zMAX_PERCEPTION_TARGETS];
    S32 oldTotalTargets = totalTargets;
    if (oldTotalTargets > 0) {
        memcpy(oldTarget, target, oldTotalTargets * sizeof(zNPCPerceptionTarget));
    }

    S32 size = CURPLAYERS.size();
    totalTargets = 0;
    for (S32 i = 0; i < size; i++) {
        target[totalTargets].SetEnt(CURPLAYERS[i]);
        for (S32 a = 0; a < oldTotalTargets; a++) {
            if (oldTarget[a].GetEnt() == target[totalTargets].GetEnt()) {
                target[totalTargets] = oldTarget[a];
                break;
            }
        }
        target[totalTargets].CalculateDistance(owner);
        target[totalTargets].UpdatePriority(owner);
        totalTargets++;
    }

    xEnt* tempWidgetTarget = NULL;
    if (smashMeWidgetTargetEnt && xEntIsEnabled(smashMeWidgetTargetEnt) && xEntIsVisible(smashMeWidgetTargetEnt))
        tempWidgetTarget = smashMeWidgetTargetEnt;
    else if (widgetTargetEnt && xEntIsEnabled(widgetTargetEnt) && xEntIsVisible(widgetTargetEnt))
        tempWidgetTarget = widgetTargetEnt;

    if (tempWidgetTarget) {
        xASSERT_FMT_L(565, totalTargets >= 0 && totalTargets < zMAX_PERCEPTION_TARGETS, "Run out of perception target slots!");
        target[totalTargets].SetEnt(tempWidgetTarget);
        for (S32 a = 0; a < oldTotalTargets; a++) {
            if (oldTarget[a].GetEnt() == target[totalTargets].GetEnt()) {
                target[totalTargets] = oldTarget[a];
                break;
            }
        }
        target[totalTargets].CalculateDistance(owner);
        target[totalTargets].UpdatePriority(owner);
        totalTargets++;
    }

    for (S32 a = 0; a < totalTargets; a++) {
        for (S32 b = a; b < totalTargets; b++) {
            if (target[a].GetPriority() < target[b].GetPriority()) {
                zNPCPerceptionTarget tmp = target[a];
                target[a] = target[b];
                target[b] = tmp;
            }
        }
    }
}

void zNPCPerception::UpdateComplete(F32 seconds, bool force)
{
    if (force || CanWeDoCompleteUpdate(seconds)) {
        UpdateCompleteDoWork(seconds);
        ResetCompleteUpdateTimers();
    }
}

xEnt* zNPCPerception::GetPriorityTarget()
{
    if (totalTargets < 1) {
        return NULL;
    }
    xEnt* ent = target[0].GetEnt();
    xASSERT_L(625, ent);
    return ent;
}

bool zNPCPerception::IsPriorityTargetAPlayer()
{
    xEnt* ent = GetPriorityTarget();
    if (ent && ent->baseType == eBaseTypePlayer)
        return true;
    return false;
}

xVec3* zNPCPerception::GetPriorityTargetPos()
{
    xEnt* ent = GetPriorityTarget();
    if (ent) {
        xVec3* targetPos = xEntGetPos(ent);
        return targetPos;
    }
    return NULL;
}

F32 zNPCPerception::GetPriorityTargetDistance()
{
    if (totalTargets < 1) {
        return -1.0f;
    }
    return target[0].GetDistance();
}

// UNUSED: needed to match (code from RotU GC)
void zNPCPerception::SetWidgetTarget(xEnt* inWidget, bool attackForever)
{
    xASSERT(inWidget);
    widgetTargetEnt = inWidget;
    widgetAttackForever = attackForever;
    ForceCompleteUpdate();
    UpdateComplete(0.016666668f, false);
}

// UNUSED: needed to match (code from RotU GC)
void zNPCPerception::PlayerDefeated(zCommonPlayer* player)
{
    xASSERT(player);
    if (IsPriorityTargetAPlayer()) {
        xEnt* targetEnt = GetPriorityTarget();
        if (targetEnt == player) {
            ForceCompleteUpdate();
            UpdateComplete(0.016666668f, true);
        }
    }
}
