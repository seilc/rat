#include "xCommon.h"
#include "zNPCAlarmer.h"

#include "zComponentPool.h"
#include "zICompNPCEntity.h"
#if RELEASE
#include "zCompLogicAlarmer.h"
#include "zCompPerception.h"
#include "zCompPerceptionParams.h"
#include "zCompSteeringGeneric.h"
#else
#include "xGlobals.h"
#include "zPlayer.h"
#endif

#if RELEASE
zICompNPCEntityParams zNPCAlarmer::sEntParams;
zCompPerceptionParams zNPCAlarmer::sPerceptionParams;
zCompSteeringGenericParams zNPCAlarmer::sSteeringPatrolParams;

void zNPCAlarmer::CompParametersInit()
{
    sEntParams.boundScale = 0.5f;

    sPerceptionParams.detectedUpdateFrequency = 0.0f;
    sPerceptionParams.lostUpdateFrequency = 0.0f;
    sPerceptionParams.flags |= k_ZCOMPPERCEPTIONPARAMS_CYLINDER;
    sPerceptionParams.cylinderR = 20.0f;
    sPerceptionParams.cylinderHUp = 10.0f;
    sPerceptionParams.cylinderHDown = 10.0f;
    sPerceptionParams.cylinderRatio = 1.25f;

    sSteeringPatrolParams.maxSpeed = 8.0f;
    sSteeringPatrolParams.steeringType = eSteeringType_Seek;
}

void zNPCAlarmer::CreateAnimTable(xAnimTable* table)
{
    xAnimTableNewState(table, "TEMP03_L_IDLE01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP03_L_IDLECROUCH01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP03_L_MOVE01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP03_L_ALARMIDLE01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP03_L_ALARMREADY01", AnimState_EndStop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);

    xAnimTableNewState(table, "TEMP03_L_CARRYIDLE01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP03_L_CARRYMOVE01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP03_L_TAKE", AnimState_EndStop, 0x4000000, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);

    zICompLogic::CreateAnimTable(table);
}

bool zNPCAlarmer::Activate()
{
    npcLogicComp = zComponentPool<zCompLogicAlarmer, zICompLogic, zNPCBase>::AttachComponent(this, NULL);
    npcEntityComp = zComponentPool<zICompNPCEntity, zICompNPCEntity, zNPCBase>::AttachComponent(this, &sEntParams);
    npcSteeringComp = zComponentPool<zCompSteeringGeneric, zICompSteering, zNPCBase>::AttachComponent(this, NULL);
    npcPerceptionComp = zComponentPool<zCompPerception, zICompPerception, zNPCBase>::AttachComponent(this, &sPerceptionParams);

    if (npcLogicComp == NULL || npcEntityComp == NULL || npcSteeringComp == NULL || npcPerceptionComp == NULL) {
        Deactivate();
        return false;
    }

    return true;
}

void zNPCAlarmer::Update(xScene* xscn, F32 dt)
{
    PreUpdateAllComponents(xscn, dt);

    zICompLogic* logicComponent = GetLogicComponent();
    if (logicComponent) {
        logicComponent->Update(xscn, dt);
    }

    PostUpdateAllComponents(xscn, dt);
}
#else
bool zNPCAlarmer::Activate()
{
    npcLogicComp = zComponentPool<zCompLogicAlarmer, zICompLogic, zNPCBase>::AttachComponent(this, NULL);
    npcEntityComp = zComponentPool<zICompNPCEntity, zICompNPCEntity, zNPCBase>::AttachComponent(this, NULL);

    if (npcLogicComp == NULL || npcEntityComp == NULL) {
        Deactivate();
        return false;
    }

    return true;
}

void zNPCAlarmer::Deactivate()
{
    DetachAllComponents();
}

void zNPCAlarmer::Update(xScene* xscn, F32 dt)
{
    PreUpdateAllComponents(xscn, dt);

    zICompLogic* logicComponent = GetLogicComponent();
    zICompNPCEntity* compNPCEntity = GetEntityComponent();

    if (logicComponent) {
        logicComponent->Update(xscn, dt);
    }
    if (compNPCEntity) {
        compNPCEntity->Update(xscn, dt);
    }
    PostUpdateAllComponents(xscn, dt);
}
#endif

void zNPCAlarmer::Render()
{
    zICompNPCEntity* compNPCEntity = GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->Render();
    }
    DebugRenderAllComponents();
}

#if !RELEASE
void zNPCAlarmer::CreateAnimTable(xAnimTable* table)
{
    xAnimTableNewState(table, "TEMP03_L_IDLE01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP03_L_IDLECROUCH01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP03_L_MOVE01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP03_L_ALARMIDLE01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP03_L_ALARMREADY01", AnimState_EndStop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);

    xAnimTableNewState(table, "TEMP03_L_CARRYIDLE01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP03_L_CARRYMOVE01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP03_L_TAKE", AnimState_EndStop, 0x4000000, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);

    zICompLogic::CreateAnimTable(table);
}

#if DEBUG
void zCompLogicAlarmer::AddStaticTweaks()
{
}
#endif

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
#endif
