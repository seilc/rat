#include "xCommon.h"
#include "zCompLogicSwarmOwl.h"

#include "xGlobals.h"
#include "xRand.h"
#include "xSndMgr.h"
#include "zCompSound.h"
#include "zCompSteeringFlocking.h"
#include "zCompSteeringFlockingBrain.h"
#include "zEvent.h"
#include "zICompNPCEntity.h"
#include "zNPCAssetSwarm.h"
#include "zPlayer.h"
#include "zScene.h"

static zCompSteeringFlockingParams gSwarmOwlSteeringParams;
static zCompSoundParams gSwarmOwlSoundParams;
static zCompStateMachineTable gSwarmOwlStateMachineTable[] = {
    { 0, zCompState_New<zSwarmOwlWander> },
    { 1, zCompState_New<zSwarmOwlBlock> },
    { 2, zCompState_New<zSwarmOwlAttracted> },
    { -1, NULL }
};

#if DEBUG
void zCompLogicSwarmOwl::AddStaticTweaks()
{
    gSwarmOwlSteeringParams.AddTweaks("NPC|Swarm");
}
#endif

void zCompLogicSwarmOwl::CreateAnimTable(xAnimTable* table)
{
    xAnimTableNewState(table, "SWARM_L_IDLE01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "SWARM_L_MOVE01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "SWARM_L_MOVE_ALERT01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);

    zICompLogic::CreateAnimTable(table);
}

void zCompLogicSwarmOwl::SetupStateTranstitionTests()
{
}

S32 zCompLogicSwarmOwl::GetInitialStateEnum()
{
    return 0;
}

zCompStateMachineTable* zCompLogicSwarmOwl::GetStateTable()
{
    return gSwarmOwlStateMachineTable;
}

bool zCompLogicSwarmOwl::SystemEvent(xBase*, xBase*, U32 to_event, const F32* to_param, xBase* to_param_widget, U32)
{
    zNPCBase* ownerNPC = owner;
    zNPCBase* neighbourNPCs;
    S32 numberNeighbourNPCs;

    ownerNPC->GetChildren(&neighbourNPCs, &numberNeighbourNPCs);

    switch (to_event) {
    case eEventSetAttractor:
        if (to_param_widget == NULL || !xBaseIsEntity(to_param_widget)) {
            xFAIL_MSG_L(DEBUG ? 114 : 106, "Set attractor event sent with a wrong to_param_widget parameter");
        }
        gSwarmOwlSteeringParams.seekCheckObjectUsage = to_param[0] > 0.0f;

        for (S32 i = 0; i < numberNeighbourNPCs; i++) {
            neighbourNPCs[i].GetSteeringComponent()->SetAttractor((xEnt*)to_param_widget);
        }

        if (GetCurrentStateEnum() != 2) {
            SwitchLastState(2);
        }
        return true;
    case eEventSetRepeller:
        if (to_param_widget == NULL || !xBaseIsEntity(to_param_widget)) {
            xFAIL_MSG_L(DEBUG ? 133 : 125, "Set repeller event sent with a wrong to_param_widget parameter");
        }
        gSwarmOwlSteeringParams.fleeCheckObjectUsage = to_param[0] > 0.0f;

        for (S32 i = 0; i < numberNeighbourNPCs; i++) {
            neighbourNPCs[i].GetSteeringComponent()->SetRepeller((xEnt*)to_param_widget);
        }
        return true;
    case eEventDisableAttractor:
        for (S32 i = 0; i < numberNeighbourNPCs; i++) {
            neighbourNPCs[i].GetSteeringComponent()->SetAttractor(NULL);
        }
        return true;
    case eEventDisableRepeller:
        for (S32 i = 0; i < numberNeighbourNPCs; i++) {
            neighbourNPCs[i].GetSteeringComponent()->SetRepeller(NULL);
        }
        return true;
    default:
        return false;
    }
}

void zCompLogicSwarmOwl::AllAttached()
{
    zNPCBase* ownerNPC = owner;
    zNPCBase* neighbourNPCs;
    S32 numberNeighbourNPCs;

    ownerNPC->GetChildren(&neighbourNPCs, &numberNeighbourNPCs);

    zNPCAssetSwarm* swarmAsset = (zNPCAssetSwarm*)ownerNPC->GetNPCAsset();

    gSwarmOwlSteeringParams.seekCheckObjectUsage = (swarmAsset->flags & k_ZNPCASSETSWARM_SEEK_CHECK_OBJECT_USAGE) != 0;
    gSwarmOwlSteeringParams.fleeCheckObjectUsage = (swarmAsset->flags & k_ZNPCASSETSWARM_FLEE_CHECK_OBJECT_USAGE) != 0;

    for (S32 i = 0; i < numberNeighbourNPCs; i++) {
        zICompSteering* compSteering = neighbourNPCs[i].GetSteeringComponent();
        if (compSteering) {
            if (swarmAsset->attractor) {
                compSteering->SetAttractor((xEnt*)zSceneFindObject(swarmAsset->attractor));
            }
            if (swarmAsset->repeller)
                compSteering->SetRepeller((xEnt*)zSceneFindObject(swarmAsset->repeller));
        }
    }

    zICompSound* soundComp = owner->GetSoundComponent();
    zCompSteeringFlockingBrain* compSteeringFlocking = (zCompSteeringFlockingBrain*)owner->GetSteeringComponent();
    if (soundComp) {
        if (swarmAsset->loopSound) {
            gSwarmOwlSoundParams.loopingSoundGroup = xSndMgrGetSoundGroup(swarmAsset->loopSound);
            if (compSteeringFlocking) {
                gSwarmOwlSoundParams.loopingSoundPosition = compSteeringFlocking->GetFlockingCenter();
            }
        }
        if (swarmAsset->oneShotSound) {
            gSwarmOwlSoundParams.oneShotSoundGroup = xSndMgrGetSoundGroup(swarmAsset->oneShotSound);
            gSwarmOwlSoundParams.oneShotMinTime = swarmAsset->oneShotSoundMinTime;
            gSwarmOwlSoundParams.oneShotMaxTime = swarmAsset->oneShotSoundMaxTime;
            if (compSteeringFlocking) {
                gSwarmOwlSoundParams.oneShotSoundPosition = compSteeringFlocking->GetFlockingCenter();
            }
        }
        soundComp->SetParameters(&gSwarmOwlSoundParams);
    }

    zCompStateMachine::AllAttached();
}

void zSwarmOwlWander::Enter(zNPCBase* owner)
{
    zNPCBase* ownerNPC = owner;
    zNPCBase* neighbourNPCs;
    S32 numberNeighbourNPCs;

    ownerNPC->GetChildren(&neighbourNPCs, &numberNeighbourNPCs);

    for (S32 i = 0; i < numberNeighbourNPCs; i++) {
        zICompSteering* compSteering = neighbourNPCs[i].GetSteeringComponent();
        zICompNPCEntity* compNPCEntity = neighbourNPCs[i].GetEntityComponent();

        if (compSteering) {
            gSwarmOwlSteeringParams.seperationNeighbourRadius = 2.0f;
            compSteering->SetParameters(&gSwarmOwlSteeringParams);
            compSteering->SetAttractor(NULL);
        }

        if (compNPCEntity) {
            compNPCEntity->SetAnimState("SWARM_L_MOVE01", 0.3f, xrand_RandomFloatRange(0.0f, compNPCEntity->GetAnimDuration()), NULL);
        }
    }
}

void zSwarmOwlWander::Update(zNPCBase* owner, F32)
{
    zNPCBase* ownerNPC = owner;
    zNPCBase* neighbourNPCs;
    S32 numberNeighbourNPCs;

    ownerNPC->GetChildren(&neighbourNPCs, &numberNeighbourNPCs);

    if (numberNeighbourNPCs <= 0) {
        return;
    }

    xVec3* playerPos = &CURPLAYERS[0]->frame->mat.pos;

    zCompSteeringFlockingBrain* flockingBrain = (zCompSteeringFlockingBrain*)owner->GetSteeringComponent();
    const xVec3* swarmPos = flockingBrain->GetFlockingCenter();

    if (swarmPos->Distance2(*playerPos) < 100.0f) {
        SwitchState(1);
    }
}

void zSwarmOwlBlock::Enter(zNPCBase* owner)
{
    zNPCBase* ownerNPC = owner;
    zNPCBase* neighbourNPCs;
    S32 numberNeighbourNPCs;

    ownerNPC->GetChildren(&neighbourNPCs, &numberNeighbourNPCs);

    for (S32 i = 0; i < numberNeighbourNPCs; i++) {
        zICompSteering* compSteering = neighbourNPCs[i].GetSteeringComponent();
        zICompNPCEntity* compNPCEntity = neighbourNPCs[i].GetEntityComponent();

        if (compSteering) {
            gSwarmOwlSteeringParams.seperationNeighbourRadius = 5.0f;
            compSteering->SetParameters(&gSwarmOwlSteeringParams);
            compSteering->SetAttractor(CURPLAYERS[0]);
        }

        if (compNPCEntity) {
            compNPCEntity->SetAnimState("SWARM_L_MOVE_ALERT01", 0.3f, xrand_RandomFloatRange(0.0f, compNPCEntity->GetAnimDuration()), NULL);
            compNPCEntity->SetAnimSpeed(0.7f);
        }
    }
}

void zSwarmOwlBlock::Update(zNPCBase* owner, F32)
{
    xVec3* playerPos = &CURPLAYERS[0]->frame->mat.pos;

    zCompSteeringFlockingBrain* flockingBrain = (zCompSteeringFlockingBrain*)owner->GetSteeringComponent();
    const xVec3* swarmPos = flockingBrain->GetFlockingCenter();

    if (swarmPos->Distance2(*playerPos) > 225.0f) {
        SwitchState(0);
    }
}

void zSwarmOwlAttracted::Enter(zNPCBase* owner)
{
    zNPCBase* ownerNPC = owner;
    zNPCBase* neighbourNPCs;
    S32 numberNeighbourNPCs;

    ownerNPC->GetChildren(&neighbourNPCs, &numberNeighbourNPCs);

    for (S32 i = 0; i < numberNeighbourNPCs; i++) {
        zICompSteering* compSteering = neighbourNPCs[i].GetSteeringComponent();
        zICompNPCEntity* compNPCEntity = neighbourNPCs[i].GetEntityComponent();

        if (compSteering) {
            gSwarmOwlSteeringParams.seperationNeighbourRadius = 2.0f;
            compSteering->SetParameters(&gSwarmOwlSteeringParams);
        }

        if (compNPCEntity) {
            compNPCEntity->SetAnimState("SWARM_L_MOVE01", 0.3f, xrand_RandomFloatRange(0.0f, compNPCEntity->GetAnimDuration()), NULL);
        }
    }
}

void zSwarmOwlAttracted::Update(zNPCBase*, F32)
{
}
