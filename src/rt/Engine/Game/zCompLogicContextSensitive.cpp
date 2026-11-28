#include "xCommon.h"
#include "zCompLogicContextSensitive.h"

#include "xGlobals.h"
#include "xMovePoint.h"
#include "xPad.h"
#include "zCombat.h"
#include "zCompSteeringBasic.h"
#include "zICompNPCEntity.h"
#include "zNPCAssetContextSensitive.h"
#include "zNPCContextSensitive.h"
#include "zNPCStates.h"
#include "zPlayer.h"
#include "zScene.h"

static zCompSteeringBasicParams gConSenPatrolSteeringParams;
static zCompSteeringBasicParams gConSenChaseSteeringParams;

static zCompStateMachineTable gContextSensitiveStateMachineTable[] = {
    { 0, zCompState_New<zConSenPatrol> },
    { 1, zCompState_New<zConSenChase> },
    { 2, zCompState_New<zConSenAttack1> },
    { 3, zCompState_New<zAnimWTimerState> },
    { 4, zCompState_New<zAnimWTimerState> },
    { 5, zCompState_New<zAnimPlayState> },
    { 6, zCompState_New<zAnimPlayState> },
    { 7, zCompState_New<zAnimPlayState> },
    { 8, zCompState_New<zAnimPlayState> },
    { -1, NULL }
};

#if DEBUG
void zCompLogicContextSensitive::AddStaticTweaks()
{
    zConSenPatrol::AddStaticTweaks("NPC|Context Sensitive|Patrol");
}
#endif

bool zCompLogicContextSensitive::SystemEvent(xBase*, xBase*, U32, const F32*, xBase*, U32)
{
    return false;
}

void zCompLogicContextSensitive::CreateAnimTable(xAnimTable* table)
{
    xAnimTableNewState(table, "TEMP05_L_IDLE", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP05_L_MOVE", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP05_L_PLAYERDAM", AnimState_EndStop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP05_L_PLAYERHIT", AnimState_EndStop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP05_L_PLAYERON", AnimState_EndStop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP05_L_TAUNT", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP05_L_ATTACKBEGIN", AnimState_EndStop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP05_L_ATTACKCYCLE", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP05_L_ATTACKDONE", AnimState_EndStop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "TEMP05_L_ATTACKHIT", AnimState_EndStop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);

    zICompLogic::CreateAnimTable(table);
}

void zCompLogicContextSensitive::SetupStateTranstitionTests()
{
    AddSwitchTest(0, 1, GoToChase, 0.0f);
    AddSwitchTest(1, 0, BackToPatrol, 0.0f);
    AddSwitchTest(1, 2, IsInAttackDistance, 0.0f);

    AddSwitchTest(2, 3, GroundAttackHit, 0.0f);
    AddSwitchTest(2, 4, GroundAttackMissed, 0.0f);

    zAnimWTimerState* TauntState = (zAnimWTimerState*)GetState(3);
    TauntState->Setup(1, "TEMP05_L_TAUNT", 2.0f);

    zAnimWTimerState* VulnerableState = (zAnimWTimerState*)GetState(4);
    VulnerableState->Setup(5, "TEMP05_L_ATTACKCYCLE", 2.0f);
    AddSwitchTest(4, 6, ButtonPressed, 0.0f);

    zAnimPlayState* AttackDoneState = (zAnimPlayState*)GetState(5);
    AttackDoneState->Setup(1, "TEMP05_L_ATTACKDONE");

    zAnimPlayState* PlayerGoesOnTop = (zAnimPlayState*)GetState(6);
    PlayerGoesOnTop->Setup(7, "TEMP05_L_PLAYERON");
    AddSwitchTest(6, 8, ButtonPressLast, 0.0f);

    zAnimPlayState* PlayerHit = (zAnimPlayState*)GetState(7);
    PlayerHit->Setup(1, "TEMP05_L_PLAYERHIT");

    zAnimPlayState* NPCMiss = (zAnimPlayState*)GetState(8);
    NPCMiss->Setup(1, "TEMP05_L_PLAYERDAM");
}

S32 zCompLogicContextSensitive::GetInitialStateEnum()
{
    return 0;
}

zCompStateMachineTable* zCompLogicContextSensitive::GetStateTable()
{
    return gContextSensitiveStateMachineTable;
}

void zCompLogicContextSensitive::Attached()
{
    zCompStateMachine::Attached();
}

void zCompLogicContextSensitive::AllAttached()
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    zoneCenter = *compNPCEntity->GetPosition();
    zCompStateMachine::AllAttached();
}

bool zCompLogicContextSensitive::GoToChase(zNPCBase* owner, zCompState*, zCompState*)
{
    zCompLogicContextSensitive* logicComp = (zCompLogicContextSensitive*)owner->GetLogicComponent();

    xVec3* playerPos = &CURPLAYERS[0]->frame->mat.pos;

    if (playerPos->Distance2(logicComp->zoneCenter) < 2500.0f) {
        return true;
    }

    return false;
}

bool zCompLogicContextSensitive::BackToPatrol(zNPCBase* owner, zCompState*, zCompState*)
{
    zCompLogicContextSensitive* logicComp = (zCompLogicContextSensitive*)owner->GetLogicComponent();

    xVec3* playerPos = &CURPLAYERS[0]->frame->mat.pos;

    if (playerPos->Distance2(logicComp->zoneCenter) > 4225.0f) {
        return true;
    }

    return false;
}

bool zCompLogicContextSensitive::IsInAttackDistance(zNPCBase* owner, zCompState*, zCompState*)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();

    if (!compNPCEntity->IsAnimationTranstitionFinished(0)) {
        return false;
    }

    xVec3* playerPos = &CURPLAYERS[0]->frame->mat.pos;
    const xVec3* NPCPos = compNPCEntity->GetPosition();

    if (NPCPos->Distance2(*playerPos) < 64.0f) {
        return true;
    }

    return false;
}

bool zCompLogicContextSensitive::ButtonPressed(zNPCBase*, zCompState*, zCompState*)
{
    xPad* pad = &mPad[0];

    return (pad->pressed & 0x80000) ? true : false;
}

bool zCompLogicContextSensitive::ButtonPressLast(zNPCBase* owner, zCompState*, zCompState*)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();

    if (compNPCEntity->GetAnimTime() < 0.8f * compNPCEntity->GetAnimDuration()) {
        return false;
    }

    xPad* pad = &mPad[0];

    return (pad->pressed & 0x80000) ? true : false;
}

bool zCompLogicContextSensitive::GroundAttackMissed(zNPCBase* owner, zCompState*, zCompState*)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();

    if (!compNPCEntity->IsAnimationStopped(0)) {
        return false;
    }

    xVec3* playerPos = &CURPLAYERS[0]->frame->mat.pos;

    if (compNPCEntity->GetPosition()->Distance2(*playerPos) < 144.0f) {
        return false;
    }

    return true;
}

bool zCompLogicContextSensitive::GroundAttackHit(zNPCBase* owner, zCompState*, zCompState*)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();

    if (!compNPCEntity->IsAnimationStopped(0)) {
        return false;
    }

    xVec3* playerPos = &CURPLAYERS[0]->frame->mat.pos;

    if (compNPCEntity->GetPosition()->Distance2(*playerPos) < 144.0f) {
        return true;
    }

    return false;
}

#if DEBUG
void zConSenPatrol::AddStaticTweaks(const char* name)
{
    gConSenPatrolSteeringParams.AddTweaks(name);
}
#endif

zConSenPatrol::zConSenPatrol()
{
    gConSenPatrolSteeringParams.maxSpeed = 4.0f;
    gConSenPatrolSteeringParams.mass = 1.0f;

    gConSenPatrolSteeringParams.seekMultiplier = 3.2f;
    gConSenPatrolSteeringParams.fleeMultiplier = 0.0f;
    gConSenPatrolSteeringParams.sphereAvoidenceMultiplier = 15.0f;
    gConSenPatrolSteeringParams.sphereAvoidenceBreakRatio = 0.0f;
    gConSenPatrolSteeringParams.sphereAvoidenceRange = 8.0f;
    gConSenPatrolSteeringParams.wanderMultiplier = 3.0f;
    curPatrolPoint = NULL;
}

void zConSenPatrol::Update(zNPCBase* owner, F32 dt)
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
                xASSERT_DESIGN_L(DEBUG ? 334 : 318, nextPatrolPoint, "Thief NPC %s has arrived its last move point. He will just wander around it.", xSceneID2Name(xglobals->sceneCur, owner->id));
                return;
            }

            curPatrolPoint = nextPatrolPoint;

            compSteering->SetAttractorV(curPatrolPoint->pos);
        }
    }
}

void zConSenPatrol::Enter(zNPCBase* owner)
{
    zNPCContextSensitive* npcConSen = (zNPCContextSensitive*)owner;
    zNPCAssetContextSensitive* npcassConSen = (zNPCAssetContextSensitive*)npcConSen->GetNPCAsset();
    if (npcassConSen->firstMovePoint == 0) {
        xFAIL_FMT_L(DEBUG ? 352 : 336, "SpringBoard NPC %s has no movepoints attached to it. He will just wander.", xSceneID2Name(xglobals->sceneCur, npcConSen->id));
        curPatrolPoint = NULL;
    } else {
        curPatrolPoint = (xMovePoint*)zSceneFindObject(npcassConSen->firstMovePoint);
    }

    Resume(owner);
}

void zConSenPatrol::Resume(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->SetAnimState("TEMP05_L_MOVE", 0.3f, NULL);
    }

    zICompSteering* compSteering = owner->GetSteeringComponent();
    if (compSteering) {
        compSteering->SetParameters(&gConSenPatrolSteeringParams);
        if (curPatrolPoint)
            compSteering->SetAttractorV(curPatrolPoint->pos);
    }
}

void zConSenPatrol::DebugRender(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity && curPatrolPoint) {
        const xVec3* curPos = compNPCEntity->GetPosition();
    }
}

zConSenChase::zConSenChase()
{
    gConSenChaseSteeringParams.maxSpeed = 4.0f;
    // BUG: sets the patrol mass, not the chase mass
    gConSenPatrolSteeringParams.mass = 1.0f;

    gConSenChaseSteeringParams.seekMultiplier = 4.0f;
    gConSenChaseSteeringParams.fleeMultiplier = 0.0f;
    gConSenChaseSteeringParams.sphereAvoidenceMultiplier = 3.0f;
    gConSenChaseSteeringParams.sphereAvoidenceBreakRatio = 0.0f;
    gConSenChaseSteeringParams.sphereAvoidenceRange = 8.0f;
    gConSenChaseSteeringParams.wanderMultiplier = 3.0f;
}

void zConSenChase::Update(zNPCBase* owner, F32 dt)
{
    zICompSteering* compSteering = owner->GetSteeringComponent();
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();

    if (compSteering && compNPCEntity) {
        compSteering->ApplySteering(compNPCEntity, dt);
    }
}

void zConSenChase::Resume(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->SetAnimState("TEMP05_L_MOVE", 0.3f, NULL);
    }

    zICompSteering* compSteering = owner->GetSteeringComponent();
    if (compSteering) {
        compSteering->SetParameters(&gConSenChaseSteeringParams);
        compSteering->SetAttractor(CURPLAYERS[0]);
    }
}

void zConSenAttack1::Resume(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->SetAnimState("TEMP05_L_ATTACKBEGIN", 0.3f, NULL);
    }
}

void zConSenAttack1::Exit(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();

    zCombatDamageInfo combatDamageInfo;
    combatDamageInfo.from = owner;
    combatDamageInfo.damage = 10;
    combatDamageInfo.source = zHS_GENERAL;
    combatDamageInfo.target = zHT_GENERAL;
    combatDamageInfo.knockback = *compNPCEntity->GetHeading();
    combatDamageInfo.knockback *= 10.0f;
    combatDamageInfo.direction = xVec3::m_Null;
    combatDamageInfo.flags |= 0x1;

    CURPLAYERS[0]->Damage(combatDamageInfo);
}
