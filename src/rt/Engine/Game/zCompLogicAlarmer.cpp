#include "xCommon.h"
#include "zCompLogicAlarmer.h"

#include "xGlobals.h"
#include "zICompNPCEntity.h"
#include "zPlayer.h"

void zCompLogicAlarmer::Attached()
{
    curState = eAlarmerStates_None;
}

void zCompLogicAlarmer::Update(xScene*, F32)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity == NULL) {
        return;
    }

    xVec3* playerPos = &CURPLAYERS[0]->frame->mat.pos;
    const xVec3* myPos = compNPCEntity->GetPosition();
    F32 playerDist2 = myPos->Distance2(*playerPos);
    F32 CheckDist = 625.0f;
    bool PlayerInDetection = playerDist2 < CheckDist;

    switch (curState) {
    case eAlarmerStates_None:
        ChangeState(eAlarmerStates_SearchPlayer);
        break;
    case eAlarmerStates_SearchPlayer:
        if (PlayerInDetection) {
            ChangeState(eAlarmerStates_ActivatingAlarm);
        }
        break;
    case eAlarmerStates_ActivatingAlarm:
        if (compNPCEntity->IsAnimationStopped(0)) {
            ChangeState(eAlarmerStates_Alarm);
        }
        if (!PlayerInDetection) {
            ChangeState(eAlarmerStates_DeactivatingAlarm);
        }
        break;
    case eAlarmerStates_Alarm:
        if (!PlayerInDetection) {
            ChangeState(eAlarmerStates_DeactivatingAlarm);
        }
        break;
    case eAlarmerStates_DeactivatingAlarm:
        if (compNPCEntity->IsAnimationTranstitionFinished(0)) {
            ChangeState(eAlarmerStates_SearchPlayer);
        }
        if (PlayerInDetection) {
            ChangeState(eAlarmerStates_ActivatingAlarm);
        }
        break;
    }
}

void zCompLogicAlarmer::ChangeState(eAlarmerStates nextState)
{
    if (curState == nextState) {
        return;
    }

    if (nextState == eAlarmerStates_None) {
        return;
    }

    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity == NULL) {
        return;
    }

    switch (curState) {
    case eAlarmerStates_SearchPlayer:
        break;
    case eAlarmerStates_ActivatingAlarm:
        break;
    case eAlarmerStates_Alarm:
        break;
    case eAlarmerStates_DeactivatingAlarm:
        break;
    }

    curState = nextState;

    switch (curState) {
    case eAlarmerStates_SearchPlayer:
        compNPCEntity->SetAnimState("TEMP03_L_IDLE01", 0.5f, NULL);
        break;
    case eAlarmerStates_ActivatingAlarm:
        compNPCEntity->SetAnimState("TEMP03_L_ALARMREADY01", 1.0f, NULL);
        break;
    case eAlarmerStates_Alarm:
        compNPCEntity->SetAnimState("TEMP03_L_ALARMIDLE01", 0.5f, NULL);
        break;
    case eAlarmerStates_DeactivatingAlarm:
        compNPCEntity->SetAnimState("TEMP03_L_IDLE01", 2.0f, NULL);
        break;
    }
}
