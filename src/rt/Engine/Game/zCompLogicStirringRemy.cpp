#include "xCommon.h"
#include "zCompLogicStirringRemy.h"

#include "xDebugTweak.h"
#include "xMovePoint.h"
#include "xPad.h"
#include "xString.h"
#include "zCompSteeringBasic.h"
#include "zEvent.h"
#include "zICompNPCEntity.h"
#include "zScene.h"

#include <stdio.h>

class zMovePoint;

static zCompSteeringBasicParams sSteeringParams;
xTWEAKABLE F32 sRemySpeed = 3.0f;
xTWEAKABLE F32 sRemyAnimSpeed = 3.0f;
xTWEAKABLE F32 sRemyWeight = 0.05f;
xTWEAKABLE F32 sArriveDistance2 = 0.15f;
static bool gotPush;
static bool gotRun;
static bool arrived;
static S32 runIdx;
static S32 curIdx = 4;
static zMovePoint* mp[100];
static S32 mp_count;
static U32 replyToID;

static zCompStateMachineTable gStirringRemyStateMachineTable[] = {
    { 0, zCompState_New<zStirringRemyIdle> },
    { 1, zCompState_New<zStirringRemyRun> },
    { 2, zCompState_New<zStirringRemyPush> },
    { -1, NULL }
};

zStirringRemyIdle::zStirringRemyIdle()
{
}

void zStirringRemyIdle::Update(zNPCBase* owner, F32)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (!compNPCEntity) {
        return;
    }

    xEntFrame* entFrame = compNPCEntity->GetFrame();
    xVec3* entVel = &entFrame->vel;
    *entVel = xVec3::m_Null;
}

void zStirringRemyIdle::Enter(zNPCBase* owner)
{
    Resume(owner);
}

void zStirringRemyIdle::Resume(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->SetAnimState("IDLE01", 0.3f, NULL);
    }

    arrived = false;
}

zStirringRemyRun::zStirringRemyRun()
{
    sSteeringParams.maxSpeed = sRemySpeed;

    sSteeringParams.wallAvoidenceMultiplier = 0.0f;
    sSteeringParams.seekMultiplier = 1.0f;
    sSteeringParams.fleeMultiplier = 0.0f;
    sSteeringParams.sphereAvoidenceMultiplier = 0.0f;
    sSteeringParams.sphereAvoidenceBreakRatio = 0.0f;
    sSteeringParams.sphereAvoidenceRange = 0.0f;
    sSteeringParams.wanderMultiplier = 0.0f;
    sSteeringParams.wanderRadius = 0.0f;
}

void zStirringRemyRun::Update(zNPCBase* owner, F32 dt)
{
    zICompSteering* compSteering = owner->GetSteeringComponent();
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();

    if (compSteering && compNPCEntity) {
        compSteering->ApplySteering(compNPCEntity, dt);
    }

    if (!compNPCEntity) {
        return;
    }

    const xVec3* curPos = compNPCEntity->GetPosition();

    F32 mpArriveDistance = curPatrolPoint->asset->zoneRadius;
    if (mpArriveDistance == -1.0f) {
        mpArriveDistance = sArriveDistance2;
    }

    if (curPatrolPoint->pos->Distance2(*curPos) < xsqr(mpArriveDistance)) {
        if (curIdx == runIdx) {
            compNPCEntity->SetAnimState("IDLE01", 0.3f, NULL);
            arrived = true;
            if (replyToID)
                zEntEvent(replyToID, eEventUIMotionFinished);
        } else {
            if (curIdx < runIdx) {
                curIdx++;
            } else {
                curIdx--;
            }

            curPatrolPoint = (xMovePoint*)mp[curIdx];
            compSteering->SetAttractorV(curPatrolPoint->pos);
        }
    }
}

void zStirringRemyRun::Enter(zNPCBase* owner)
{
    xASSERT_L(164, mp_count > 0);

    curPatrolPoint = (xMovePoint*)mp[curIdx];
    Resume(owner);
}

void zStirringRemyRun::Resume(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->SetAnimState("RUN01", 0.3f, NULL);
        xModelInstance* model = compNPCEntity->GetModel();
        model->Anim->Single->CurrentSpeed = sRemyAnimSpeed;
    }

    zICompSteering* compSteering = owner->GetSteeringComponent();
    if (compSteering) {
        compSteering->SetParameters(&sSteeringParams);
        compSteering->SetAttractorV(curPatrolPoint->pos);
    }

    gotRun = false;
}

zStirringRemyPush::zStirringRemyPush()
{
}

void zStirringRemyPush::Update(zNPCBase*, F32)
{
}

void zStirringRemyPush::Enter(zNPCBase* owner)
{
    Resume(owner);
}

void zStirringRemyPush::Resume(zNPCBase* owner)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->SetAnimState("PUSH01", 0.3f, NULL);
    }

    gotPush = false;
}

bool zCompLogicStirringRemy::AreWeThereYet(zNPCBase*, zCompState*, zCompState*)
{
    return arrived;
}

bool zCompLogicStirringRemy::GotRunEvent(zNPCBase*, zCompState*, zCompState*)
{
    return gotRun;
}

bool zCompLogicStirringRemy::GotPushEvent(zNPCBase*, zCompState*, zCompState*)
{
    return gotPush;
}

bool zCompLogicStirringRemy::IsAnimationStopped(zNPCBase* owner, zCompState*, zCompState*)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();

    xModelInstance* model = compNPCEntity->GetModel();
    xAnimState* astate = model->Anim->Single->State;
    return model->Anim->Single->Time >= astate->Data->Duration;
}

#if DEBUG || RELEASE
void zCompLogicStirringRemy::AddStaticTweaks()
{
    {
        char* prefix = "Cooking Minigames|StirringRemy|";
        xAUTOTWEAK(prefix, "sRemySpeed", &sRemySpeed, 0.0f, 10.0f, NULL, NULL, 0, true);
        xAUTOTWEAK(prefix, "sRemyWeight", &sRemyWeight, 0.0f, 10.0f, NULL, NULL, 0, true);
        xAUTOTWEAK(prefix, "sRemyAnimSpeed", &sRemyAnimSpeed, 0.0f, 10.0f, NULL, NULL, 0, true);
        xAUTOTWEAK(prefix, "sArriveDistance2", &sArriveDistance2, 0.0f, 10.0f, NULL, NULL, 0, true);
    }
    {
        char* prefix = "Cooking Minigames|StirringRemy|";
        xAUTOTWEAK(prefix, "sSteeringParams.seekMultiplier", &sSteeringParams.seekMultiplier, 0.0f, 10.0f, NULL, NULL, 0, true);
        xAUTOTWEAK(prefix, "sSteeringParams.seekRadius", &sSteeringParams.seekRadius, 0.0f, 100.0f, NULL, NULL, 0, true);
        xAUTOTWEAK(prefix, "sSteeringParams.fleeMultiplier", &sSteeringParams.fleeMultiplier, 0.0f, 10.0f, NULL, NULL, 0, true);
        xAUTOTWEAK(prefix, "sSteeringParams.sphereAvoidenceMultiplier", &sSteeringParams.sphereAvoidenceMultiplier, 0.0f, 10.0f, NULL, NULL, 0, true);
        xAUTOTWEAK(prefix, "sSteeringParams.sphereAvoidenceBreakRatio", &sSteeringParams.sphereAvoidenceBreakRatio, 0.0f, 10.0f, NULL, NULL, 0, true);
        xAUTOTWEAK(prefix, "sSteeringParams.sphereAvoidenceRange", &sSteeringParams.sphereAvoidenceRange, 0.0f, 10.0f, NULL, NULL, 0, true);
        xAUTOTWEAK(prefix, "sSteeringParams.wanderMultiplier", &sSteeringParams.wanderMultiplier, 0.0f, 10.0f, NULL, NULL, 0, true);
        xAUTOTWEAK(prefix, "sSteeringParams.wanderRadius", &sSteeringParams.wanderRadius, 0.0f, 10.0f, NULL, NULL, 0, true);
    }
}
#endif

void zCompLogicStirringRemy::CreateAnimTable(xAnimTable* table)
{
    xAnimTableNewState(table, "IDLE01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "RUN01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "PUSH01", AnimState_EndStop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);

    zICompLogic::CreateAnimTable(table);
}

void zCompLogicStirringRemy::SetupStateTranstitionTests()
{
    AddPushTest(0, 1, GotRunEvent, 0.0f);
    AddPushTest(0, 2, GotPushEvent, 0.0f);

    AddPopTest(1, AreWeThereYet, 0.1f);
    AddPopTest(2, IsAnimationStopped, 0.5f);
}

S32 zCompLogicStirringRemy::GetInitialStateEnum()
{
    return 0;
}

zCompStateMachineTable* zCompLogicStirringRemy::GetStateTable()
{
    return gStirringRemyStateMachineTable;
}

void zCompLogicStirringRemy::Attached()
{
    zCompStateMachine::Attached();

    mp_count = 0;
    for (S32 i = 0; i < 100; i++) {
        char name[32];
        sprintf(name, "REMY MP %d", i);
        zMovePoint* zmp = (zMovePoint*)zSceneFindObject(xStrHash(name));
        if (zmp) {
            iprintf("Found Remy MP %d\n", i);
            mp[mp_count++] = zmp;
            xASSERT_L(312, mp_count < 100);
        }
    }
    iprintf("Movepoint list:  %d points\n", mp_count);
}

bool zCompLogicStirringRemy::SystemEvent(xBase*, xBase*, U32 to_event, const F32* to_param, xBase*, U32 to_param_widget_id)
{
    switch (to_event) {
    case eEventNPCMoveTo:
        runIdx = (S32)to_param[0];
        if (runIdx < 0)
            runIdx = 0;
        if (runIdx > mp_count - 1)
            runIdx = mp_count - 1;
        if (curIdx != runIdx) {
            iprintf("Event:  NPC Move To %d (curIdx = %d)\n", runIdx, curIdx);
            gotRun = true;
        }
        break;
    case eEventHit:
        gotPush = true;
        break;
    case eEventAttachTo:
        replyToID = to_param_widget_id;
        break;
    }

    return false;
}
