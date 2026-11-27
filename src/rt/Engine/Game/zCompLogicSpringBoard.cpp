#include "xCommon.h"
#include "zCompLogicSpringBoard.h"

#include "xCollideSweptSphere.h"
#include "xGlobals.h"
#include "xMovePoint.h"
#include "zCompSteeringBasic.h"
#include "zICompNPCEntity.h"
#include "zNPCAssetSpringBoard.h"
#include "zNPCSpringBoard.h"
#include "zPlayer.h"
#include "zScene.h"

static zCompSteeringBasicParams gSpringBoardPatrolSteeringParams;

static zCompStateMachineTable gSpringboardStateMachineTable[] = {
    { 0, zCompState_New<zSpringBoardPatrol> },
    { 1, zCompState_New<zSpringBoardTurnAround> },
    { 2, zCompState_New<zSpringBoardIdle> },
    { 3, zCompState_New<zSpringBoardGoToPatrol> },
    { 4, zCompState_New<zSpringBoardLaunch> },
    { -1, NULL }
};

#if DEBUG
void zCompLogicSpringBoard::AddStaticTweaks()
{
    zSpringBoardPatrol::AddStaticTweaks("NPC|SpringBoard|Patrol");
}
#endif

bool zCompLogicSpringBoard::SystemEvent(xBase*, xBase*, U32, const F32*, xBase*, U32)
{
    return false;
}

void zCompLogicSpringBoard::CreateAnimTable(xAnimTable* table)
{
    xAnimTableNewState(table, "TEMP01_L_CARRY01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP01_L_IDLE01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP01_L_MOVE01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP01_L_PUTDOWN01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP01_L_SPRINGBEGIN01", AnimState_EndStop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP01_L_SPRINGBOUNCE01", AnimState_EndStop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP01_L_SPRINGDONE01", AnimState_EndStop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP01_L_SPRINGIDLE01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);

    zICompLogic::CreateAnimTable(table);
}

void zCompLogicSpringBoard::SetupStateTranstitionTests()
{
    AddSwitchTest(0, 1, IsPlayerInRange, 0.0f);
    AddSwitchTest(1, 2, IsAnimationStopped, 0.0f);
    AddSwitchTest(2, 3, IsPlayerOutRange, 0.0f);
    AddSwitchTest(3, 0, IsAnimationStopped, 0.0f);

    AddPopTest(4, IsAnimationStopped, 0.0f);
}

S32 zCompLogicSpringBoard::GetInitialStateEnum()
{
    return 0;
}

zCompStateMachineTable* zCompLogicSpringBoard::GetStateTable()
{
    return gSpringboardStateMachineTable;
}

void zCompLogicSpringBoard::Attached()
{
    zCompStateMachine::Attached();
}

bool zCompLogicSpringBoard::InteractionCheck(const SphereCollisionResults& results)
{
    if (results.sphereIndex != 0) {
        return false;
    }

    if (GetCurrentStateEnum() == 2 || GetCurrentStateEnum() == 4) {
        zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();

        xASSERT_FMT_L(DEBUG ? 137 : 129, compNPCEntity->bound.type == k_XBOUNDTYPE_OBB, "SpringBoard type NPC needs to have a OBB bounding box?");
        if (results.normal.dot(compNPCEntity->bound.mat->up) < 0.7f) {
            return false;
        }

        xVec3 cornerPos;
        xMat4x3ToWorld(&cornerPos, compNPCEntity->bound.mat, &compNPCEntity->bound.box.box.upper);
        F32 d = compNPCEntity->bound.mat->up.dot(cornerPos);
        F32 playerD = results.contact.dot(compNPCEntity->bound.mat->up);
        return xabs(d - playerD) < 0.05f;
    } else {
        return false;
    }
}

bool zCompLogicSpringBoard::IsPlayerInRange(zNPCBase* owner, zCompState*, zCompState*)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    xVec3* playerPos = &CURPLAYERS[0]->frame->mat.pos;
    xVec3* NPCPos = &compNPCEntity->frame->mat.pos;

    if (NPCPos->Distance2(*playerPos) > 100.0f) {
        return false;
    }

    return true;
}

bool zCompLogicSpringBoard::IsPlayerOutRange(zNPCBase* owner, zCompState*, zCompState*)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    xVec3* playerPos = &CURPLAYERS[0]->frame->mat.pos;
    xVec3* NPCPos = &compNPCEntity->frame->mat.pos;

    if (NPCPos->Distance2(*playerPos) < 225.0f) {
        return false;
    }

    return true;
}

bool zCompLogicSpringBoard::IsAnimationStopped(zNPCBase* owner, zCompState*, zCompState*)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    return compNPCEntity->IsAnimationStopped(0);
}

void zCompLogicSpringBoard::GetLaunchVelocity(xVec3& vel)
{
    vel.x = 0.0f;
    vel.y = 30.0f;
    vel.z = 0.0f;
}

void zCompLogicSpringBoard::Launch()
{
    PushStateOnLast(4);
}

void zCompLogicSpringBoard::StopNPC(F32 StopForceMag, F32 dt)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (!compNPCEntity) {
        return;
    }

    xEntFrame* entFrame = compNPCEntity->GetFrame();
    xVec3* entVel = &entFrame->vel;
    xVec3* entOldVel = &entFrame->oldvel;

    F32 oldSpeed = entOldVel->length();

    if (oldSpeed < EPSILON) {
        return;
    }

    F32 accMag = StopForceMag;

    if (dt > oldSpeed / accMag) {
        *entVel = xVec3::m_Null;
        oldAcc = xVec3::m_Null;
        acc = xVec3::m_Null;
    } else {
        xVec3 VelocityDir = *entOldVel;
        VelocityDir.Scale(1.0f / oldSpeed);

        acc.Scale(VelocityDir, -accMag);

        xVec3 dAcc;
        dAcc.Scale(acc, dt);

        xVec3Add(entVel, &dAcc, entOldVel);
    }

    oldAcc = acc;
}

#if DEBUG
void zSpringBoardPatrol::AddStaticTweaks(const char* name)
{
    gSpringBoardPatrolSteeringParams.AddTweaks(name);
}
#endif

zSpringBoardPatrol::zSpringBoardPatrol()
{
    gSpringBoardPatrolSteeringParams.maxSpeed = 6.0f;

    gSpringBoardPatrolSteeringParams.seekMultiplier = 3.2f;
    gSpringBoardPatrolSteeringParams.fleeMultiplier = 0.0f;
    gSpringBoardPatrolSteeringParams.sphereAvoidenceMultiplier = 15.0f;
    gSpringBoardPatrolSteeringParams.sphereAvoidenceBreakRatio = 0.0f;
    gSpringBoardPatrolSteeringParams.sphereAvoidenceRange = 8.0f;
    gSpringBoardPatrolSteeringParams.wanderMultiplier = 3.0f;
    curPatrolPoint = NULL;
}

void zSpringBoardPatrol::Update(zNPCBase* owner, F32 dt)
{
    zICompSteering* compSteering = owner->GetSteeringComponent();
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();

    if (compSteering && compNPCEntity) {
        compSteering->ApplySteering(compNPCEntity, dt);
    }

    if (compNPCEntity && curPatrolPoint) {
        const xVec3* curPos = compNPCEntity->GetPosition();

        F32 mpArriveDistance = 5.0f;

        if (mpArriveDistance == -1.0f) {
            mpArriveDistance = compNPCEntity->bound.sph.r;
        }

        if (curPatrolPoint->pos->Distance2(*curPos) < xsqr(mpArriveDistance)) {
            xMovePoint* nextPatrolPoint;
            xMovePointGetNext(curPatrolPoint, NULL, &nextPatrolPoint, NULL);

            if (!nextPatrolPoint) {
                xASSERT_DESIGN_L(DEBUG ? 317 : 293, nextPatrolPoint, "Thief NPC %s has arrived its last move point. He will just wander around it.", xSceneID2Name(xglobals->sceneCur, owner->id));
                return;
            }

            curPatrolPoint = nextPatrolPoint;

            compSteering->SetAttractorV(curPatrolPoint->pos);
        }
    }
}

void zSpringBoardPatrol::Enter(zNPCBase* owner)
{
    zNPCSpringBoard* npcSpringBoard = (zNPCSpringBoard*)owner;
    zNPCAssetSpringBoard* npcassSpringBoard = (zNPCAssetSpringBoard*)npcSpringBoard->GetNPCAsset();
    if (npcassSpringBoard->firstMovePoint == 0) {
        xFAIL_FMT_L(DEBUG ? 335 : 311, "SpringBoard NPC %s has no movepoints attached to it. He will just wander.", xSceneID2Name(xglobals->sceneCur, npcSpringBoard->id));
        curPatrolPoint = NULL;
    } else {
        curPatrolPoint = (xMovePoint*)zSceneFindObject(npcassSpringBoard->firstMovePoint);
    }

    Resume(owner);
}

void zSpringBoardPatrol::Resume(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->SetAnimState("TEMP01_L_MOVE01", 0.3f, NULL);
    }

    zICompSteering* compSteering = owner->GetSteeringComponent();
    if (compSteering) {
        compSteering->SetParameters(&gSpringBoardPatrolSteeringParams);
        if (curPatrolPoint)
            compSteering->SetAttractorV(curPatrolPoint->pos);
    }
}

void zSpringBoardPatrol::DebugRender(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity && curPatrolPoint) {
        const xVec3* curPos = compNPCEntity->GetPosition();
    }
}

zSpringBoardTurnAround::zSpringBoardTurnAround()
{
}

void zSpringBoardTurnAround::Update(zNPCBase* owner, F32 dt)
{
    zCompLogicSpringBoard* compLogic = (zCompLogicSpringBoard*)owner->GetLogicComponent();

    if (compLogic)
        compLogic->StopNPC(20.0f, dt);
}

void zSpringBoardTurnAround::Enter(zNPCBase* owner)
{
    Resume(owner);
}

void zSpringBoardTurnAround::Resume(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->SetAnimState("TEMP01_L_SPRINGBEGIN01", 0.3f, NULL);
    }
}

zSpringBoardIdle::zSpringBoardIdle()
{
}

void zSpringBoardIdle::Update(zNPCBase*, F32)
{
}

void zSpringBoardIdle::Enter(zNPCBase* owner)
{
    Resume(owner);
}

void zSpringBoardIdle::Resume(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->SetAnimState("TEMP01_L_SPRINGIDLE01", 0.3f, NULL);
    }
}

zSpringBoardLaunch::zSpringBoardLaunch()
{
}

void zSpringBoardLaunch::Update(zNPCBase*, F32)
{
}

void zSpringBoardLaunch::Enter(zNPCBase* owner)
{
    Resume(owner);
}

void zSpringBoardLaunch::Resume(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->SetAnimState("TEMP01_L_SPRINGBOUNCE01", 0.3f, NULL);
    }
}

zSpringBoardGoToPatrol::zSpringBoardGoToPatrol()
{
}

void zSpringBoardGoToPatrol::Update(zNPCBase*, F32)
{
}

void zSpringBoardGoToPatrol::Enter(zNPCBase* owner)
{
    Resume(owner);
}

void zSpringBoardGoToPatrol::Resume(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->SetAnimState("TEMP01_L_SPRINGDONE01", 0.3f, NULL);
    }
}
