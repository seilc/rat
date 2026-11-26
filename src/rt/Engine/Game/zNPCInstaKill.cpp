#include "xCommon.h"
#include "zNPCInstaKill.h"

#include "zComponentPool.h"
#include "zCompPerception.h"
#include "zCompSteeringGeneric.h"
#include "zICompNPCEntity.h"
#if RELEASE
#include "zCompLogicInstaKill.h"
#include "zCompPerceptionParams.h"
#else
#include "xGlobals.h"
#include "xMovePoint.h"
#include "zNPCAssetInstaKill.h"
#include "zScene.h"
#endif

#if RELEASE
zICompNPCEntityParams zNPCInstaKill::sEntParams;
zCompPerceptionParams zNPCInstaKill::sPerceptionParams;
zCompSteeringGenericParams zNPCInstaKill::sSteeringPatrolParams;
zCompSteeringGenericParams zNPCInstaKill::sSteeringAlertParams;
zCompSteeringGenericParams zNPCInstaKill::sSteeringChaseParams;

void zNPCInstaKill::CompParametersInit()
{
    sEntParams.boundScale = 0.5f;

    sPerceptionParams.detectedUpdateFrequency = 0.0f;
    sPerceptionParams.lostUpdateFrequency = 0.0f;
    sPerceptionParams.flags |= 0x4;
    sPerceptionParams.cylinderR = 20.0f;
    sPerceptionParams.cylinderHUp = 10.0f;
    sPerceptionParams.cylinderHDown = 10.0f;
    sPerceptionParams.cylinderRatio = 1.25f;

    sSteeringPatrolParams.maxSpeed = 8.0f;
    sSteeringPatrolParams.steeringType = eSteeringType_Seek;

    sSteeringAlertParams.turnSpringK = 10.0f;
    sSteeringAlertParams.steeringType = eSteeringType_Turn;
    sSteeringAlertParams.frictionK = 10.0f;
    sSteeringAlertParams.wanderOn = false;
    sSteeringAlertParams.frictionOn = true;

    sSteeringChaseParams.maxAcc = 30.0f;
    sSteeringChaseParams.maxSpeed = 18.0f;
    sSteeringChaseParams.turnSpringK = 20.0f;
    sSteeringChaseParams.steeringType = eSteeringType_Seek;
}

void zNPCInstaKill::CreateAnimTable(xAnimTable* table)
{
    xAnimTableNewState(table, "INSTAKILL_ATTACK_BEGIN", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "INSTAKILL_ATTACK_HIT", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "INSTAKILL_ATTACK_MISS", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "INSTAKILL_IDLE", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "INSTAKILL_MOVE", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "INSTAKILL_MOVEALERT", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "INSTAKILL_NOTICE", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);

    zICompLogic::CreateAnimTable(table);
}

void zNPCInstaKill::InitCombatDamage()
{
}

void zNPCInstaKill::AddTweaks(const char* branchString)
{
    sSteeringPatrolParams.AddTweaks(branchString, "Patrol Steering");
    sSteeringAlertParams.AddTweaks(branchString, "Alert Steering");
    sSteeringChaseParams.AddTweaks(branchString, "Chase Steering");
}

bool zNPCInstaKill::Activate()
{
    npcLogicComp = zComponentPool<zCompLogicInstaKill, zICompLogic, zNPCBase>::AttachComponent(this, NULL);
    npcEntityComp = zComponentPool<zICompNPCEntity, zICompNPCEntity, zNPCBase>::AttachComponent(this, &sEntParams);
    npcSteeringComp = zComponentPool<zCompSteeringGeneric, zICompSteering, zNPCBase>::AttachComponent(this, NULL);
    npcPerceptionComp = zComponentPool<zCompPerception, zICompPerception, zNPCBase>::AttachComponent(this, &sPerceptionParams);

    if (npcLogicComp == NULL || npcEntityComp == NULL || npcSteeringComp == NULL || npcPerceptionComp == NULL) {
        Deactivate();
        return false;
    }

    return true;
}

void zNPCInstaKill::Update(xScene* xscn, F32 dt)
{
    PreUpdateAllComponents(xscn, dt);

    zICompLogic* logicComponent = GetLogicComponent();
    if (logicComponent) {
        logicComponent->Update(xscn, dt);
    }

    PostUpdateAllComponents(xscn, dt);
}
#else
static zICompNPCEntityParams gInstaKillEntParams;
static zCompSteeringGenericParams gInstaKillPatrolSteeringParams;

void zNPCInstaKill::CompParametersInit()
{
    gInstaKillEntParams.boundScale = 0.5f;
    gInstaKillPatrolSteeringParams.maxSpeed = 8.0f;
    gInstaKillPatrolSteeringParams.steeringType = eSteeringType_Seek;
}

#if DEBUG
void zNPCInstaKill::AddTweaks(const char*)
{
}
#endif

bool zNPCInstaKill::Activate()
{
    npcLogicComp = zComponentPool<zCompLogicInstaKill, zICompLogic, zNPCBase>::AttachComponent(this, NULL);
    npcEntityComp = zComponentPool<zICompNPCEntity, zICompNPCEntity, zNPCBase>::AttachComponent(this, &gInstaKillEntParams);
    npcSteeringComp = zComponentPool<zCompSteeringGeneric, zICompSteering, zNPCBase>::AttachComponent(this, NULL);
    npcPerceptionComp = zComponentPool<zCompPerception, zICompPerception, zNPCBase>::AttachComponent(this, NULL);

    if (npcLogicComp == NULL || npcEntityComp == NULL || npcSteeringComp == NULL || npcPerceptionComp == NULL) {
        Deactivate();
        return false;
    }

    return true;
}

void zNPCInstaKill::Deactivate()
{
    DetachAllComponents();
}

void zNPCInstaKill::Update(xScene* xscn, F32 dt)
{
    PreUpdateAllComponents(xscn, dt);

    zICompLogic* logicComponent = GetLogicComponent();
    if (logicComponent) {
        logicComponent->Update(xscn, dt);
    }

    PostUpdateAllComponents(xscn, dt);
}
#endif

void zNPCInstaKill::Render()
{
    zICompNPCEntity* compNPCEntity = GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->Render();
    }
    DebugRenderAllComponents();
}

#if !RELEASE
class zInstaKillPatrol : public zCompState
{
    xMovePoint* curPatrolPoint;

public:
    zInstaKillPatrol();

    virtual void Enter(zNPCBase* owner);
    virtual void Resume(zNPCBase* owner);
    virtual void Update(zNPCBase* owner, F32 dt);
    void CheckTranstitions(zNPCBase*);
    virtual void DebugRender(zNPCBase*);
};

static zCompStateMachineTable gInstaKillStateMachineTable[] = {
    { 0, zCompState_New<zInstaKillPatrol> },
    { -1, NULL }
};

void zCompLogicInstaKill::SetupStateTranstitionTests()
{
}

S32 zCompLogicInstaKill::GetInitialStateEnum()
{
    return 0;
}

zCompStateMachineTable* zCompLogicInstaKill::GetStateTable()
{
    return gInstaKillStateMachineTable;
}

void zCompLogicInstaKill::AllAttached()
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    zoneCenter = *compNPCEntity->GetPosition();
    zCompStateMachine::AllAttached();
}

zInstaKillPatrol::zInstaKillPatrol()
{
    curPatrolPoint = NULL;
}

void zInstaKillPatrol::Enter(zNPCBase* owner)
{
    zNPCInstaKill* npcInstaKill = (zNPCInstaKill*)owner;
    zNPCAssetInstaKill* npcassInstaKill = (zNPCAssetInstaKill*)npcInstaKill->GetNPCAsset();
    if (npcassInstaKill->firstMovePoint == 0) {
        xFAIL_FMT_L(229, "Thief NPC %s has no movepoints attached to it. He will just wander.", xSceneID2Name(xglobals->sceneCur, npcInstaKill->id));
        curPatrolPoint = NULL;
    } else {
        curPatrolPoint = (xMovePoint*)zSceneFindObject(npcassInstaKill->firstMovePoint);
    }

    Resume(owner);
}

void zInstaKillPatrol::Resume(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->SetAnimState("TEMP03_L_MOVE01", 0.3f, NULL);
    }

    zICompSteering* compSteering = owner->GetSteeringComponent();
    if (compSteering) {
        compSteering->SetParameters(&gInstaKillPatrolSteeringParams);
        if (curPatrolPoint)
            compSteering->SetDestination(curPatrolPoint->pos);
    }
}

void zInstaKillPatrol::Update(zNPCBase* owner, F32 dt)
{
    zICompSteering* compSteering = NULL;
    compSteering = owner->GetSteeringComponent();
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();

    if (compSteering && compNPCEntity) {
        compSteering->ApplySteering(compNPCEntity, dt);
    }

    if (compNPCEntity && curPatrolPoint) {
        const xVec3* curPos = compNPCEntity->GetPosition();

        F32 mpArriveDistance = curPatrolPoint->asset->zoneRadius;
        if (mpArriveDistance == -1.0f) {
            mpArriveDistance = compNPCEntity->bound.sph.r;
        }

        if (curPatrolPoint->pos->Distance2XZ(*curPos) < xsqr(mpArriveDistance)) {
            xMovePoint* nextPatrolPoint;
            xMovePointGetNext(curPatrolPoint, NULL, &nextPatrolPoint, NULL);

            if (!nextPatrolPoint) {
                xASSERT_DESIGN_L(294, nextPatrolPoint, "Thief NPC %s has arrived its last move point. He will just wander around it.", xSceneID2Name(xglobals->sceneCur, owner->id));
                return;
            }

            curPatrolPoint = nextPatrolPoint;
            compSteering->SetDestination(curPatrolPoint->pos);
        }
    }

    CheckTranstitions(owner);
}

void zInstaKillPatrol::CheckTranstitions(zNPCBase*)
{
}

void zInstaKillPatrol::DebugRender(zNPCBase*)
{
}
#endif
