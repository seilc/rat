#include "xCommon.h"
#include "zCompLogicSwarmBug.h"

#include "xEntMotion.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xRand.h"
#include "xSndMgr.h"
#include "zCommonPlayer.h"
#include "zCompSound.h"
#include "zCompSteeringFlocking.h"
#include "zCompSteeringFlockingBrain.h"
#include "zEvent.h"
#include "zICompNPCEntity.h"
#include "zNPCAssetSwarm.h"
#include "zScene.h"

#include <float.h>

static zCompSteeringFlockingParams gSwarmBugSteeringParams;
static zCompSoundParams gSwarmBugSoundParams;
static zCompStateMachineTable gSwarmBugStateMachineTable[] = {
    { 0, zCompState_New<zSwarmBugWander> },
    { -1, NULL }
};

#if DEBUG
void zCompLogicSwarmBug::AddStaticTweaks()
{
    gSwarmBugSteeringParams.AddTweaks("NPC|SwarmBug");
}
#endif

void zCompLogicSwarmBug::SetupStateTranstitionTests()
{
}

S32 zCompLogicSwarmBug::GetInitialStateEnum()
{
    return 0;
}

zCompStateMachineTable* zCompLogicSwarmBug::GetStateTable()
{
    return gSwarmBugStateMachineTable;
}

bool zCompLogicSwarmBug::SystemEvent(xBase*, xBase*, U32 to_event, const F32* to_param, xBase* to_param_widget, U32)
{
    zNPCBase* ownerNPC = owner;
    zNPCBase* neighbourNPCs;
    S32 numberNeighbourNPCs;

    ownerNPC->GetChildren(&neighbourNPCs, &numberNeighbourNPCs);

    switch (to_event) {
    case eEventSetAttractor:
        if (to_param_widget == NULL || !xBaseIsEntity(to_param_widget)) {
            xFAIL_MSG_L(DEBUG ? 104 : 96, "Set attractor event sent with a wrong to_param_widget parameter");
        }
        gSwarmBugSteeringParams.seekCheckObjectUsage = to_param[0] > 0.0f;

        for (S32 i = 0; i < numberNeighbourNPCs; i++) {
            neighbourNPCs[i].GetSteeringComponent()->SetAttractor((xEnt*)to_param_widget);
        }
        return true;
    case eEventSetRepeller:
        if (to_param_widget == NULL || !xBaseIsEntity(to_param_widget)) {
            xFAIL_MSG_L(DEBUG ? 119 : 111, "Set repeller event sent with a wrong to_param_widget parameter");
        }
        gSwarmBugSteeringParams.fleeCheckObjectUsage = to_param[0] > 0.0f;

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

void zCompLogicSwarmBug::AllAttached()
{
    zNPCBase* ownerNPC = owner;
    zNPCBase* neighbourNPCs;
    S32 numberNeighbourNPCs;

    ownerNPC->GetChildren(&neighbourNPCs, &numberNeighbourNPCs);

    zNPCAssetSwarm* swarmAsset = (zNPCAssetSwarm*)ownerNPC->GetNPCAsset();

    gSwarmBugSteeringParams.seekCheckObjectUsage = (swarmAsset->flags & k_ZNPCASSETSWARM_SEEK_CHECK_OBJECT_USAGE) != 0;
    gSwarmBugSteeringParams.fleeCheckObjectUsage = (swarmAsset->flags & k_ZNPCASSETSWARM_FLEE_CHECK_OBJECT_USAGE) != 0;

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
            gSwarmBugSoundParams.loopingSoundGroup = xSndMgrGetSoundGroup(swarmAsset->loopSound);
            if (compSteeringFlocking) {
                gSwarmBugSoundParams.loopingSoundPosition = compSteeringFlocking->GetFlockingCenter();
            }
        }
        if (swarmAsset->oneShotSound) {
            gSwarmBugSoundParams.oneShotSoundGroup = xSndMgrGetSoundGroup(swarmAsset->oneShotSound);
            gSwarmBugSoundParams.oneShotMinTime = swarmAsset->oneShotSoundMinTime;
            gSwarmBugSoundParams.oneShotMaxTime = swarmAsset->oneShotSoundMaxTime;
            if (compSteeringFlocking) {
                gSwarmBugSoundParams.oneShotSoundPosition = compSteeringFlocking->GetFlockingCenter();
            }
        }
        soundComp->SetParameters(&gSwarmBugSoundParams);
    }

    zCompStateMachine::AllAttached();
}

zSwarmBugWander::zSwarmBugWander()
{
    gSwarmBugSteeringParams.SetDefaults();

    gSwarmBugSteeringParams.alignmentMultiplier = 0.0f;
    gSwarmBugSteeringParams.seperationNeighbourRadius = 3.0f;
    gSwarmBugSteeringParams.playerAvoidenceRadius = 5.0f;
    gSwarmBugSteeringParams.playerAvoidenceMultiplier = 0.0f;
    gSwarmBugSteeringParams.cohesionMultiplier = 0.7f;

    gSwarmBugSteeringParams.maxForce = 50.0f;
    gSwarmBugSteeringParams.seekMultiplier = 0.7f;
    gSwarmBugSteeringParams.fleeMultiplier = 3.0f;
    gSwarmBugSteeringParams.fleeRadius = 10.0f;
}

void zSwarmBugWander::Enter(zNPCBase* owner)
{
    zNPCBase* ownerNPC = owner;
    zNPCBase* neighbourNPCs;
    S32 numberNeighbourNPCs;

    ownerNPC->GetChildren(&neighbourNPCs, &numberNeighbourNPCs);

    for (S32 i = 0; i < numberNeighbourNPCs; i++) {
        zICompSteering* compSteering = neighbourNPCs[i].GetSteeringComponent();
        zICompNPCEntity* compNPCEntity = neighbourNPCs[i].GetEntityComponent();

        if (compSteering) {
            compSteering->SetParameters(&gSwarmBugSteeringParams);
        }

        if (compNPCEntity) {
            compNPCEntity->SetAnimState("SWARM_L_MOVE01", 0.3f, xrand_RandomFloatRange(0.0f, compNPCEntity->GetAnimDuration()), NULL);
        }
    }
}

void zSwarmBugWander::Update(zNPCBase* owner, F32)
{
    zNPCBase* ownerNPC = owner;
    zNPCBase* neighbourNPCs;
    S32 numberNeighbourNPCs;

    ownerNPC->GetChildren(&neighbourNPCs, &numberNeighbourNPCs);

    if (numberNeighbourNPCs <= 0) {
        return;
    }

    zCommonPlayer* player = (zCommonPlayer*)CURPLAYERS[0];
    xVec3* playerPos = &player->frame->mat.pos;

    zCompSteeringFlockingBrain* flockingBrain = (zCompSteeringFlockingBrain*)owner->GetSteeringComponent();
    const xVec3* swarmPos = flockingBrain->GetFlockingCenter();

    zCompSteeringBasic* compNPCSteering = (zCompSteeringBasic*)neighbourNPCs->GetSteeringComponent();
    const xEnt* repeller = compNPCSteering->GetRepellerEnt();
    if (repeller && (xEnt*)player->carry.grabbed == repeller) {
        return;
    }

    F32 minPlayerDist2 = FLT_MAX, playerDist2;
    for (S32 i = 0; i < numberNeighbourNPCs; i++) {
        zICompNPCEntity* compNPCEntity = neighbourNPCs[i].GetEntityComponent();
        playerDist2 = compNPCEntity->GetDistance2(playerPos);
        if (playerDist2 < minPlayerDist2)
            minPlayerDist2 = playerDist2;
    }

    if (minPlayerDist2 < 1.0f) {
        zCombatDamageInfo combatDamageInfo;
        combatDamageInfo.from = owner;
        combatDamageInfo.damage = 5;
        combatDamageInfo.source = zHS_GENERAL;
        combatDamageInfo.target = zHT_GENERAL;
        combatDamageInfo.knockback = xVec3::m_Null;
        combatDamageInfo.direction = xVec3::m_Null;
        player->Damage(combatDamageInfo);
    }
}
