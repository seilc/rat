#include "xCommon.h"
#include "zCompLogicInstaKill.h"

#include "xEntMotion.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xMovePoint.h"
#include "zCommonPlayer.h"
#include "zCompSteeringGeneric.h"
#include "zICompNPCEntity.h"
#include "zICompPerception.h"
#include "zNPCAssetInstaKill.h"
#include "zNPCInstaKill.h"
#include "zPlayerContainer.h"
#include "zScene.h"

static zCompStateMachineTable gInstaKillStateMachineTable[] = {
    { 0, zCompState_New<zInstaKillPatrolXZ> },
    { 1, zCompState_New<zInstaKillAlertState> },
    { 2, zCompState_New<zInstaKillChase> },
    { -1, NULL }
};

void zCompLogicInstaKill::SetupStateTranstitionTests()
{
    zAnimWTimerState* AlertState = (zAnimWTimerState*)GetState(1);
    AlertState->Setup(2, "TEMP05_L_TAUNT", 2.0f);
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

zInstaKillPatrolXZ::zInstaKillPatrolXZ()
{
    curPatrolPoint = NULL;
}

xMovePoint* zInstaKillPatrolXZ::FindClosestMP(zNPCBase* owner, U32 firstMovePointID)
{
    if (firstMovePointID == 0) {
        return NULL;
    }

    xMovePoint* firstMP = (xMovePoint*)zSceneFindObject(firstMovePointID);
    if (!firstMP) {
        return NULL;
    }

    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (!compNPCEntity)
        return firstMP;
    const xVec3* npcPos = compNPCEntity->GetPosition();

    xMovePoint* curMP;
    F32 curMPDistance2;
    xMovePoint* minDistMP = firstMP;
    F32 minMPDistance2 = firstMP->pos->Distance2XZ(*npcPos);
    xMovePointGetNext(firstMP, NULL, &curMP, NULL);

    while (curMP && curMP != firstMP) {
        curMPDistance2 = curMP->pos->Distance2XZ(*npcPos);
        if (curMPDistance2 < minMPDistance2) {
            minMPDistance2 = curMPDistance2;
            minDistMP = curMP;
        }

        xMovePointGetNext(curMP, NULL, &curMP, NULL);
    }

    xASSERT_L(137, minDistMP);
    return minDistMP;
}

void zInstaKillPatrolXZ::Activate(zNPCBase* owner)
{
    zNPCInstaKill* npcInstaKill = (zNPCInstaKill*)owner;
    zNPCAssetInstaKill* npcassInstaKill = (zNPCAssetInstaKill*)npcInstaKill->GetNPCAsset();

    curPatrolPoint = FindClosestMP(owner, npcassInstaKill->firstMovePoint);
    xASSERT_FMT_L(149, curPatrolPoint, "Thief NPC %s has no movepoints attached to it. He will just wander.", xSceneID2Name(xglobals->sceneCur, npcInstaKill->id));

    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->SetAnimState("TEMP05_L_MOVE", 0.3f, NULL);
    }

    zICompSteering* compSteering = owner->GetSteeringComponent();
    if (compSteering) {
        compSteering->SetParameters(&zNPCInstaKill::sSteeringPatrolParams);
        if (curPatrolPoint)
            compSteering->SetDestination(curPatrolPoint->pos);
    }
}

void zInstaKillPatrolXZ::Update(zNPCBase* owner, F32 dt)
{
    zICompSteering* compSteering = owner->GetSteeringComponent();
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
                xASSERT_DESIGN_L(202, nextPatrolPoint, "Thief NPC %s has arrived its last move point. He will just wander around it.", xSceneID2Name(xglobals->sceneCur, owner->id));
                return;
            }

            curPatrolPoint = nextPatrolPoint;
            compSteering->SetDestination(curPatrolPoint->pos);
        }
    }
}

void zInstaKillPatrolXZ::CheckTranstitions(zNPCBase* owner)
{
    zICompPerception* npcPerc = owner->GetPerceptionComponent();
    if (npcPerc->IsPerceived()) {
        SwitchState(1);
    }
}

void zInstaKillPatrolXZ::DebugRender(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity && curPatrolPoint) {
        compNPCEntity->GetPosition();
    }
}

void zInstaKillAlertState::Activate(zNPCBase* owner)
{
    zAnimWTimerState::Activate(owner);

    zICompSteering* compSteering = owner->GetSteeringComponent();
    if (compSteering) {
        compSteering->SetParameters(&zNPCInstaKill::sSteeringAlertParams);

        zCommonPlayer* player = (zCommonPlayer*)CURPLAYERS[0];
        compSteering->SetFacing(&player->frame->mat.pos);
    }
}

void zInstaKillAlertState::Update(zNPCBase* owner, F32 dt)
{
    zAnimWTimerState::Update(owner, dt);

    zICompSteering* compSteering = owner->GetSteeringComponent();
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();

    if (compSteering && compNPCEntity) {
        compSteering->ApplySteering(compNPCEntity, dt);
    }
}

void zInstaKillChase::Activate(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->SetAnimState("TEMP05_L_MOVE", 0.3f, NULL);
    }

    zICompSteering* compSteering = owner->GetSteeringComponent();
    if (compSteering) {
        compSteering->SetParameters(&zNPCInstaKill::sSteeringChaseParams);

        zCommonPlayer* player = (zCommonPlayer*)CURPLAYERS[0];
        compSteering->SetDestination(&player->frame->mat.pos);
    }
}

void zInstaKillChase::Update(zNPCBase* owner, F32 dt)
{
    zICompSteering* compSteering = owner->GetSteeringComponent();
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();

    if (compSteering && compNPCEntity) {
        compSteering->ApplySteering(compNPCEntity, dt);
    }
}

void zInstaKillChase::CheckTranstitions(zNPCBase*)
{
}
