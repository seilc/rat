#include "xCommon.h"
#include "zCompSteeringFlocking.h"

#include "xDebugTweak.h"
#include "xGlobals.h"
#include "zCompSteeringFlockingBrain.h"
#include "zICompNPCEntity.h"
#include "zPlayer.h"

#include <stdio.h>

static zCompSteeringFlockingParams gDefaultFlockingParams;

#if RELEASE
// UNUSED: needed to match (speculative code)
void zCompSteeringFlockingParams::AddTweaks(const char* parentName, const char* branchName)
{
    char prefix[256];
    sprintf(prefix, "%s|%s|", parentName, branchName);

    xAUTOTWEAK(prefix, "Seperation Multiplier", &seperationMultiplier, 0.0f, 100.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "Seperation Detection Radius", &seperationNeighbourRadius, 0.0f, 1000.0f, NULL, NULL, 0, false);

    xAUTOTWEAK(prefix, "Player Avoidence Multiplier", &playerAvoidenceMultiplier, 0.0f, 100.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "Player Avoidence Radius", &playerAvoidenceRadius, 0.0f, 1000.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "Player Avoidence Force Limit", &playerAvoidenceForceLimit, 0.0f, 100.0f, NULL, NULL, 0, false);

    xAUTOTWEAK(prefix, "Alignment Multiplier", &alignmentMultiplier, 0.0f, 100.0f, NULL, NULL, 0, false);

    xAUTOTWEAK(prefix, "Cohesion Multiplier", &cohesionMultiplier, 0.0f, 100.0f, NULL, NULL, 0, false);

    zCompSteeringBasicParams::AddTweaks(parentName, branchName);
}
#elif DEBUG
void zCompSteeringFlockingParams::AddTweaks(const char* name)
{
    AddTweak(name, "Steering", "Seperation Multiplier", &seperationMultiplier, 0.0f, 100.0f);
    AddTweak(name, "Steering", "Seperation Detection Radius", &seperationNeighbourRadius, 0.0f, 1000.0f);

    AddTweak(name, "Steering", "Player Avoidence Multiplier", &playerAvoidenceMultiplier, 0.0f, 100.0f);
    AddTweak(name, "Steering", "Player Avoidence Radius", &playerAvoidenceRadius, 0.0f, 1000.0f);
    AddTweak(name, "Steering", "Player Avoidence Force Limit", &playerAvoidenceForceLimit, 0.0f, 100.0f);

    AddTweak(name, "Steering", "Alignment Multiplier", &alignmentMultiplier, 0.0f, 100.0f);

    AddTweak(name, "Steering", "Cohesion Multiplier", &cohesionMultiplier, 0.0f, 100.0f);

    zCompSteeringBasicParams::AddTweaks(name);
}
#endif

zCompSteeringFlocking::zCompSteeringFlocking()
{
}

void zCompSteeringFlocking::Attached()
{
    if (flockingParameters == NULL) {
        flockingParameters = &gDefaultFlockingParams;
    }

    zCompSteeringBasic::Attached();
}

void zCompSteeringFlocking::AllAttached()
{
    zNPCBase* ownerNPC = owner;
    zNPCBase* parentNPC = ownerNPC->GetParent();

    xASSERT_FMT_L(DEBUG ? 86 : 89, parentNPC, "NPCs that are using the flocking steering, needs a parent NPC that controls them. This NPC has none.");

    flockingBrain = (zCompSteeringFlockingBrain*)parentNPC->GetSteeringComponent();

    xASSERT_FMT_L(DEBUG ? 90 : 93, flockingBrain, "NPCs that are using the flocking steering, needs a parent NPC that have a steering brain component. This NPC has none.");
}

void zCompSteeringFlocking::Detached()
{
    zCompSteeringBasic::Detached();
}

#if RELEASE
void zCompSteeringFlocking::Reset()
{
    zCompSteeringBasic::Reset();
}
#endif

void zCompSteeringFlocking::Update(xScene* xscn, F32 dt)
{
    zCompSteeringBasic::Update(xscn, dt);
}

#if DEBUG || RELEASE
namespace
{
bool renderSeperationForce;
bool renderPlayerAvoidenceForce;
bool renderAlignmentForce;
bool renderCohesionForce;
bool renderWallAvoidenceForce;
bool renderSphereAvoidenceForce;
bool renderWanderForce;
bool renderSeekForce;
bool renderFleeForce;
} // namespace

void zCompSteeringFlocking::AddStaticTweaks()
{
    const char* prefix = "Components|Steering Flocking|";

    xAUTOTWEAK_BOOL(prefix, "renderSeperationForce", &renderSeperationForce, NULL, NULL, 0, true);
    xAUTOTWEAK_BOOL(prefix, "renderPlayerAvoidenceForce", &renderPlayerAvoidenceForce, NULL, NULL, 0, true);
    xAUTOTWEAK_BOOL(prefix, "renderAlignmentForce", &renderAlignmentForce, NULL, NULL, 0, true);
    xAUTOTWEAK_BOOL(prefix, "renderCohesionForce", &renderCohesionForce, NULL, NULL, 0, true);

    xAUTOTWEAK_BOOL(prefix, "renderWallAvoidenceForce", &renderWallAvoidenceForce, NULL, NULL, 0, true);
    xAUTOTWEAK_BOOL(prefix, "renderSphereAvoidenceForce", &renderSphereAvoidenceForce, NULL, NULL, 0, true);
    xAUTOTWEAK_BOOL(prefix, "renderWanderForce", &renderWanderForce, NULL, NULL, 0, true);
    xAUTOTWEAK_BOOL(prefix, "renderSeekForce", &renderSeekForce, NULL, NULL, 0, true);
    xAUTOTWEAK_BOOL(prefix, "renderFleeForce", &renderFleeForce, NULL, NULL, 0, true);
}
#endif

namespace
{
zNPCBase* gNeighbourNPCs;
S32 gNumberNeighbourNPCs;
} // namespace

void zCompSteeringFlocking::CalculateForce(xVec3& force, zSteeringBasicDynamics& steeringDynamics, F32 dt)
{
    const zCompSteeringFlockingParams* params = Params();

    zNPCBase* curNPC = owner;
    zNPCBase* parentSwarmNPC = curNPC->GetParent();
    parentSwarmNPC->GetChildren(&gNeighbourNPCs, &gNumberNeighbourNPCs);

    force = xVec3::m_Null;

    if (params->wallAvoidenceOn) {
        xVec3 WallAvoidenceForce;
        GetWallAvoidenceForce(WallAvoidenceForce, steeringDynamics, dt);

#if DEBUG || RELEASE
        DebugRenderForce(steeringDynamics.pos, &WallAvoidenceForce, renderWallAvoidenceForce, params->wallAvoidenceMultiplier, g_WHITE);
#endif

        if (!AccumulateForce(force, WallAvoidenceForce, params->wallAvoidenceMultiplier, false)) {
            return;
        }
    }

    if (params->sphereAvoidenceOn) {
        xVec3 SphereAvoidenceForce;
        GetSphereAvoidenceForce(SphereAvoidenceForce, steeringDynamics, dt);

#if DEBUG || RELEASE
        DebugRenderForce(steeringDynamics.pos, &SphereAvoidenceForce, renderSphereAvoidenceForce, params->sphereAvoidenceMultiplier, g_YELLOW);
#endif

        if (!AccumulateForce(force, SphereAvoidenceForce, params->sphereAvoidenceMultiplier, false)) {
            return;
        }
    }

    if (gNumberNeighbourNPCs > 1) {
        xVec3 SeperationForce;
        GetSeperationForce(SeperationForce, steeringDynamics, dt);
        SeperationForce.y = 0.0f;

#if DEBUG || RELEASE
        DebugRenderForce(steeringDynamics.pos, &SeperationForce, renderSeperationForce, params->seperationMultiplier, g_BLUE);
#endif

        if (!AccumulateForce(force, SeperationForce, params->seperationMultiplier, true)) {
            return;
        }
    }

    xVec3 PlayerAvoidenceForce;
    GetPlayerAvoidenceForce(PlayerAvoidenceForce, steeringDynamics, dt);
    PlayerAvoidenceForce.y = 0.0f;

#if DEBUG || RELEASE
    DebugRenderForce(steeringDynamics.pos, &PlayerAvoidenceForce, renderPlayerAvoidenceForce, params->playerAvoidenceMultiplier, g_GREEN);
#endif

    if (!AccumulateForce(force, PlayerAvoidenceForce, params->playerAvoidenceMultiplier, true)) {
        return;
    }

    if (gNumberNeighbourNPCs > 1) {
        xVec3 AlignmentForce;
        GetAlignmentForce(AlignmentForce, steeringDynamics, dt);
        AlignmentForce.y = 0.0f;

#if DEBUG || RELEASE
        DebugRenderForce(steeringDynamics.pos, &AlignmentForce, renderAlignmentForce, params->alignmentMultiplier, g_BLUE);
#endif

        if (!AccumulateForce(force, AlignmentForce, params->alignmentMultiplier, true)) {
            return;
        }
    }

    if (gNumberNeighbourNPCs > 1) {
        xVec3 CohesionForce;
        GetCohesionForce(CohesionForce, steeringDynamics, dt);
        CohesionForce.y = 0.0f;

#if DEBUG || RELEASE
        DebugRenderForce(steeringDynamics.pos, &CohesionForce, renderCohesionForce, params->cohesionMultiplier, g_BLUE);
#endif

        if (!AccumulateForce(force, CohesionForce, params->cohesionMultiplier, true)) {
            return;
        }
    }

    if (params->wanderOn) {
        xVec3 WanderForce;
        GetWanderForce(WanderForce, steeringDynamics, dt);

#if DEBUG || RELEASE
        DebugRenderForce(steeringDynamics.pos, &WanderForce, renderWanderForce, params->wanderMultiplier, g_GREEN);
#endif

        if (!AccumulateForce(force, WanderForce, params->wanderMultiplier, true)) {
            return;
        }
    }

    if (params->seekOn) {
        xVec3 SeekForce;
        GetSeekForce(SeekForce, steeringDynamics, dt);

#if DEBUG || RELEASE
        DebugRenderForce(steeringDynamics.pos, &SeekForce, renderSeekForce, params->seekMultiplier, g_BLUE);
#endif

        if (!AccumulateForce(force, SeekForce, params->seekMultiplier, true)) {
            return;
        }
    }

    if (params->fleeOn) {
        xVec3 FleeForce;
        GetFleeForce(FleeForce, steeringDynamics, dt);

#if DEBUG || RELEASE
        DebugRenderForce(steeringDynamics.pos, &FleeForce, renderFleeForce, params->fleeMultiplier, g_RED);
#endif

        if (!AccumulateForce(force, FleeForce, params->fleeMultiplier, true))
            return;
    }
}

void zCompSteeringFlocking::GetSeperationForce(xVec3& SeperationForce, zSteeringBasicDynamics& steeringDynamics, F32)
{
    const zCompSteeringFlockingParams* params = Params();

    SeperationForce = xVec3::m_Null;

    const xVec3* neighbourEntityPos;

    for (S32 i = 0; i < gNumberNeighbourNPCs; i++) {
        if (&gNeighbourNPCs[i] == owner) {
            continue;
        }

        zICompNPCEntity* curNeighbourEntity = gNeighbourNPCs[i].GetEntityComponent();
        neighbourEntityPos = curNeighbourEntity->GetPosition();

        F32 dist2 = neighbourEntityPos->Distance2(*steeringDynamics.pos);
        if (dist2 < xsqr(params->seperationNeighbourRadius)) {
            if (dist2 > EPSILON) {
                xVec3 SeperationFromAgent;
                SeperationFromAgent.Sub(*steeringDynamics.pos, *neighbourEntityPos);
                SeperationFromAgent.at_normalize();
                F32 dist = xsqrt(dist2);
                SeperationFromAgent.Scale(1.0f / dist);
                SeperationForce.Add(SeperationFromAgent);
            }
        }
    }
}

void zCompSteeringFlocking::GetPlayerAvoidenceForce(xVec3& PlayerAvoidenceForce, zSteeringBasicDynamics& steeringDynamics, F32)
{
    const zCompSteeringFlockingParams* params = Params();

    PlayerAvoidenceForce = xVec3::m_Null;

    xVec3* playerPos = &CURPLAYERS[0]->frame->mat.pos;
    F32 dist2 = playerPos->Distance2(*steeringDynamics.pos);
    if (dist2 < xsqr(params->playerAvoidenceRadius)) {
        if (dist2 > EPSILON) {
            xVec3 SeperationFromPlayer;
            SeperationFromPlayer.Sub(*steeringDynamics.pos, *playerPos);
            SeperationFromPlayer.at_normalize();
            F32 dist = xsqrt(dist2);
            F32 forceMag = 20.0f / dist;
            if (forceMag > params->playerAvoidenceForceLimit)
                forceMag = params->playerAvoidenceForceLimit;
            SeperationFromPlayer.Scale(forceMag);
            PlayerAvoidenceForce = SeperationFromPlayer;
        }
    }
}

void zCompSteeringFlocking::GetAlignmentForce(xVec3& AlignmentForce, zSteeringBasicDynamics& steeringDynamics, F32)
{
    AlignmentForce = xVec3::m_Null;

    const xVec3* flockAlignment = flockingBrain->GetFlockingAlignment();

    AlignmentForce.Sub(*flockAlignment, *steeringDynamics.heading);
}

void zCompSteeringFlocking::GetCohesionForce(xVec3& CohesionForce, zSteeringBasicDynamics& steeringDynamics, F32)
{
    const zCompSteeringFlockingParams* params = Params();

    CohesionForce = xVec3::m_Null;

    const xVec3* flockCenter = flockingBrain->GetFlockingCenter();

    xVec3 desiredVelocity;
    desiredVelocity.Sub(*flockCenter, *steeringDynamics.pos);

    desiredVelocity.normalize();
    desiredVelocity.Scale(Params()->maxSpeed);

    CohesionForce.Sub(desiredVelocity, *steeringDynamics.vel);
    CohesionForce.y = 0.0f;
}
