#include "xCommon.h"
#include "zCompSteeringGeneric.h"

#include "xDebugTweak.h"
#include "xDraw.h"
#include "xRand.h"
#include "zBase.h"
#include "zEvent.h"
#include "zICompNPCEntity.h"
#include "zNPCHelper.h"
#include "zNPCWalls.h"
#include "zScene.h"
#if RELEASE
#include "zGlobals.h"
#endif

#include <float.h>
#include <stdio.h>

static zCompSteeringGenericParams gDefaultSteeringBasicParams;

#if RELEASE
void zCompSteeringGenericParams::AddTweaks(const char* parentName, const char* branchName)
{
    char prefix[256];
    sprintf(prefix, "%s|%s|", parentName, branchName);

    xAUTOTWEAK(prefix, "__MaxAcc", &maxAcc, 0.0f, 100.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "__Max Speed", &maxSpeed, 0.0f, 100.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "__Turn Spring K", &turnSpringK, 0.0f, 100.0f, NULL, NULL, 0, false);

    xAUTOTWEAK_BOOL(prefix, "_WallAvoidence On", &wallAvoidenceOn, NULL, NULL, 0, false);
    xAUTOTWEAK_BOOL(prefix, "_SphereAvoidence On", &sphereAvoidenceOn, NULL, NULL, 0, false);
    xAUTOTWEAK_BOOL(prefix, "_Wander On", &wanderOn, NULL, NULL, 0, false);
    xAUTOTWEAK_BOOL(prefix, "_Gravity On", &gravityOn, NULL, NULL, 0, false);

    xAUTOTWEAK(prefix, "Avoidence Time", &avoidenceTime, 0.0f, 10.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "AvoidenceWall Ratio", &wallAvoidenceRatio, 0.0f, 10.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "AvoidenceSphere Ratio", &sphereAvoidenceRatio, 0.0f, 10.0f, NULL, NULL, 0, false);

    xAUTOTWEAK(prefix, "Wander Ratio", &wanderRatio, 0.0f, 10.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "Wander FOV", &wanderFOV, 0.0f, 1.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "Wander Jitter per Sec", &wanderJitterPerSec, 0.0f, 1000.0f, NULL, NULL, 0, false);
}
#elif DEBUG
// UNUSED: needed to match (speculative code)
void zCompSteeringGenericParams::AddTweaks(const char* name)
{
    AddTweak(name, "Steering", "__MaxAcc", &maxAcc, 0.0f, 100.0f);
    AddTweak(name, "Steering", "__Max Speed", &maxSpeed, 0.0f, 100.0f);
    AddTweak(name, "Steering", "__Turn Spring K", &turnSpringK, 0.0f, 100.0f);

    AddTweak(name, "Steering", "_WallAvoidence On", &wallAvoidenceOn);
    AddTweak(name, "Steering", "_SphereAvoidence On", &sphereAvoidenceOn);
    AddTweak(name, "Steering", "_Wander On", &wanderOn);
    AddTweak(name, "Steering", "_Gravity On", &gravityOn);

    AddTweak(name, "Steering", "WallAvoidence Ratio", &wallAvoidenceRatio, 0.0f, 10.0f);
    AddTweak(name, "Steering", "WallAvoidence Feeler Length", &wallFeelerLength, 0.0f, 100.0f);

    AddTweak(name, "Steering", "SphereAvoidence Ratio", &sphereAvoidenceRatio, 0.0f, 10.0f);
    AddTweak(name, "Steering", "SphereAvoidence Range", &sphereAvoidenceRange, 0.0f, 100.0f);
    AddTweak(name, "Steering", "SphereAvoidence Break Ratio", &sphereAvoidenceBreakRatio, 0.0f, 1.0f);

    AddTweak(name, "Steering", "Wander Ratio", &wanderRatio, 0.0f, 10.0f);
    AddTweak(name, "Steering", "Wander FOV", &wanderFOV, 0.0f, 1.0f);
    AddTweak(name, "Steering", "Wander Jitter per Sec", &wanderJitterPerSec, 0.0f, 1000.0f);
}
#endif

void zSteeringGenericDynamics::Initialize(zICompNPCEntity* npcEnt)
{
    oldPos = npcEnt->GetOldPosition();
    pos = npcEnt->GetPosition();
    oldVel = npcEnt->GetOldVelocity();
    vel = npcEnt->GetVelocity();

    heading = npcEnt->GetHeading();

    boundRadius = npcEnt->bound.sph.r;
}

zCompSteeringGeneric::zCompSteeringGeneric()
{
}

#if RELEASE
void zCompSteeringGeneric::Attached()
{
    const zNPCAssetRATS* npcAsset = owner->GetNPCAsset();

    if (steeringParameters == NULL)
        steeringParameters = &gDefaultSteeringBasicParams;

    prevWanderDir = xVec3::m_UnitAxisZ;
    prevRandomizerDir = xVec3::m_UnitAxisZ;

    if (npcAsset->npcWalls) {
        xBase* b = zSceneFindObject(npcAsset->npcWalls);
        if (!b || b->baseType != eBaseTypeNPCWalls) {
            xFAIL_MSG_L(122, "The widget attached to the NPC for NPCwalls is not a type eBaseTypeNPCWalls!");
            return;
        }

        npcWalls = (zNPCWalls*)b;
    } else {
        npcWalls = NULL;
    }

    destination = NULL;
    facingPos = NULL;
    swCircle.PrepareXZ(xVec3::m_Null, 0.0f, xVec3::m_UnitAxisX, 0.0f);
    wallID = -1;
}

void zCompSteeringGeneric::SetParameters(const zCompParameters* parameters)
{
    steeringParameters = (const zCompSteeringGenericParams*)parameters;
    destination = NULL;
    facingPos = NULL;
}
#else
void zCompSteeringGeneric::Attached()
{
    zNPCBase* ownerNPC = owner;
    const zNPCAssetRATS* npcAsset = ownerNPC->GetNPCAsset();

    if (steeringParameters == NULL)
        steeringParameters = &gDefaultSteeringBasicParams;

    prevWanderDir = xVec3::m_UnitAxisZ;
    prevRandomizerDir = xVec3::m_UnitAxisZ;

    if (npcAsset->npcWalls) {
        xBase* b = zSceneFindObject(npcAsset->npcWalls);
        if (!b || b->baseType != eBaseTypeNPCWalls) {
            xFAIL_MSG_L(121, "The widget attached to the NPC for NPCwalls is not a type eBaseTypeNPCWalls!");
            return;
        }

        npcWalls = (zNPCWalls*)b;
    } else {
        npcWalls = NULL;
    }

    destination = NULL;
    facing = NULL;
}
#endif

void zCompSteeringGeneric::Detached()
{
}

#if RELEASE
void zCompSteeringGeneric::Reset()
{
}
#endif

void zCompSteeringGeneric::Update(xScene*, F32)
{
}

bool zCompSteeringGeneric::SystemEvent(xBase*, xBase*, U32 to_event, const F32*, xBase* to_param_widget, U32)
{
    switch (to_event) {
    case eEventSwitchNPCWalls:
        if (!to_param_widget || to_param_widget->baseType != eBaseTypeNPCWalls) {
            xFAIL_MSG_L(DEBUG ? 152 : 167, "Switch NPC Walls event sent with a wrong to_param_widget parameter");
            return true;
        }

        npcWalls = (zNPCWalls*)to_param_widget;
        return true;
    }

    return false;
}

#if DEBUG || RELEASE
namespace
{
#if RELEASE
bool renderWallAvoidenceForce = true;
#else
bool renderWallAvoidenceForce;
#endif
bool renderSphereAvoidenceForce;
bool renderWanderForce;
#if RELEASE
bool renderDestForce = true;
#else
bool renderDestForce;
#endif
bool renderTotalForce;
} // namespace

void zCompSteeringGeneric::AddStaticTweaks()
{
    const char* prefix = "Components|Steering Generic|";

    xAUTOTWEAK_BOOL(prefix, "renderWallAvoidenceForce", &renderWallAvoidenceForce, NULL, NULL, 0, true);
    xAUTOTWEAK_BOOL(prefix, "renderSphereAvoidenceForce", &renderSphereAvoidenceForce, NULL, NULL, 0, true);
    xAUTOTWEAK_BOOL(prefix, "renderWanderForce", &renderWanderForce, NULL, NULL, 0, true);
    xAUTOTWEAK_BOOL(prefix, "renderDestForce", &renderDestForce, NULL, NULL, 0, true);
    xAUTOTWEAK_BOOL(prefix, "renderTotalForce", &renderTotalForce, NULL, NULL, 0, true);
}

#if RELEASE
void zCompSteeringGeneric::DebugRenderForce(const xVec3* entPos, xVec3* Force, bool render, xColor color)
{
    if (!render)
        return;

    static xVec3 ArrowDrawOffset = { 0.0f, 0.2f, 0.0f };

    xVec3 HeadingArrowStart;
    HeadingArrowStart.Add(*entPos, ArrowDrawOffset);
    xDrawSetColor(color);

    xVec3 ForceMult;
    ForceMult.Scale(*Force, 1.0f);
    xDrawArrow(&HeadingArrowStart, &ForceMult, false, 1.0f);
}
#else
void zCompSteeringGeneric::DebugRenderForce(const xVec3* entPos, xVec3* Force, bool, xColor color)
{
    static xVec3 ArrowDrawOffset = { 0.0f, 0.2f, 0.0f };

    xVec3 HeadingArrowStart;
    HeadingArrowStart.Add(*entPos, ArrowDrawOffset);
    xDrawSetColor(color);

    xVec3 ForceMult;
    ForceMult.Scale(*Force, 1.0f);
    xDrawArrow(&HeadingArrowStart, &ForceMult, false, 1.0f);
}
#endif
#endif

#if RELEASE
void zCompSteeringGeneric::ApplySteering(zICompNPCEntity* npcEnt, F32 dt)
{
    xASSERT_L(221, npcEnt);

    zSteeringGenericDynamics steeringDynamics;
    steeringDynamics.Initialize(npcEnt);

    xVec3 acc;
    CalculateAcc(acc, steeringDynamics, dt);
    npcEnt->ApplyAcc(acc);

    npcEnt->UpdateVelocity(Params()->maxSpeed, dt);

    xVec3 dVel;
    if (CalculateVelConstraints(dVel, steeringDynamics, dt))
        npcEnt->ApplyVelConstraints(dVel);

    npcEnt->UpdatePosition(dt);

    xVec3 dPos, facing;
    F32 velCorrectionRatio;
    if (CalculatePosConstraints(dPos, velCorrectionRatio, steeringDynamics, dt))
        npcEnt->ApplyPosConstraints(dPos, velCorrectionRatio);

    CalculateFacing(facing, steeringDynamics, dt);

    npcEnt->TurnToFaceXZ(&facing, dt, Params()->turnSpringK);
}

void zCompSteeringGeneric::CalculateAcc(xVec3& acc, zSteeringGenericDynamics& steeringDynamics, F32 dt)
{
    acc = xVec3::m_Null;
    F32 accMag = 0.0f;

    const zCompSteeringGenericParams* params = Params();

    if (params->wallAvoidenceOn) {
        xVec3 WallAvoidenceAcc;
        F32 WallAvoidenceMag = GetWallAvoidenceAcc(WallAvoidenceAcc, steeringDynamics, dt);
        DebugRenderForce(steeringDynamics.pos, &WallAvoidenceAcc, renderWallAvoidenceForce, g_WHITE);

        if (AccumulateVec(acc, accMag, params->maxAcc, WallAvoidenceAcc, WallAvoidenceMag))
            return;
    }

    xVec3 destAcc;
    F32 destMag;
    switch (params->steeringType) {
    case eSteeringType_Seek:
        destMag = GetSeekAcc(destAcc, steeringDynamics, dt);
        break;
    case eSteeringType_Turn:
        destAcc = xVec3::m_Null;
        destMag = 0.0f;
        break;
    default:
        destAcc = xVec3::m_Null;
        destMag = 0.0f;
        xFAIL_MSG_L(307, "This steering type is not implemented yet!");
    }

    DebugRenderForce(steeringDynamics.pos, &destAcc, renderDestForce, g_BLUE);

    if (AccumulateVec(acc, accMag, params->maxAcc, destAcc, destMag))
        return;

    if (params->wanderOn) {
        xVec3 WanderAcc;
        F32 WanderMag = GetWanderAcc(WanderAcc, steeringDynamics, dt);
        DebugRenderForce(steeringDynamics.pos, &WanderAcc, renderWanderForce, g_GREEN);

        if (AccumulateVec(acc, accMag, params->maxAcc, WanderAcc, WanderMag))
            return;
    }
}

bool zCompSteeringGeneric::CalculateVelConstraints(xVec3& dVel, zSteeringGenericDynamics& steeringDynamics, F32 dt)
{
    dVel = xVec3::m_Null;

    bool nonZeroDVel = false;
    const zCompSteeringGenericParams* params = Params();

    if (params->gravityOn) {
        if (globals.sceneCur->flags & 0x1) {
            dVel.y -= 0.05f * dt;
            nonZeroDVel = true;
        }
    }

    if (params->frictionOn) {
        xVec3 oldVelXZ;
        oldVelXZ.x = steeringDynamics.oldVel->x;
        oldVelXZ.y = 0.0f;
        oldVelXZ.z = steeringDynamics.oldVel->z;

        F32 oldSpeed2 = oldVelXZ.length2();
        if (oldSpeed2 != 0.0f) {
            F32 accMag = params->frictionK;
            F32 oldSpeed = xsqrt(oldSpeed2);

            if (dt > oldSpeed / accMag) {
                dVel.x = -oldVelXZ.x;
                dVel.z = -oldVelXZ.z;
            } else {
                oldVelXZ.Scale(1.0f / oldSpeed);
                dVel.Scale(oldVelXZ, -accMag);
                dVel.Scale(dVel, dt);
            }

            nonZeroDVel = true;
        }
    }

    return nonZeroDVel;
}

bool zCompSteeringGeneric::CalculatePosConstraints(xVec3& dPos, F32& velCorrectionRatio, zSteeringGenericDynamics& steeringDynamics, F32 dt)
{
    dPos = xVec3::m_Null;

    bool nonZeroDPos = GetSweptCircleCollDPos(dPos, velCorrectionRatio, steeringDynamics, dt);
    return nonZeroDPos;
}

void zCompSteeringGeneric::CalculateFacing(xVec3& facing, zSteeringGenericDynamics& steeringDynamics, F32)
{
    switch (Params()->steeringType) {
    case eSteeringType_Seek:
    case eSteeringType_Turn:
        if (facingPos)
            facing.Sub(*facingPos, *steeringDynamics.pos);
        else
            facing = *steeringDynamics.vel;
        break;
    default:
        facing = *steeringDynamics.vel;
        xFAIL_MSG_L(421, "This steering type is not implemented yet!");
    }
}
#else
void zCompSteeringGeneric::ApplySteering(zICompNPCEntity* npcEnt, F32 dt)
{
    xASSERT_L(206, npcEnt);

    zSteeringGenericDynamics steeringDynamics;
    steeringDynamics.Initialize(npcEnt);

    xVec3 acc;
    CalculateAcc(acc, steeringDynamics, dt);
    npcEnt->ApplyAcc(acc);

    npcEnt->UpdateVelocity(Params()->maxSpeed, dt);

    xVec3 dVel;
    CalculateVelConstraints(dVel, steeringDynamics, dt);
    npcEnt->ApplyVelConstraints(dVel);

    npcEnt->UpdatePosition(dt);

    xVec3 dPos;
    CalculatePosConstraints(dPos, steeringDynamics, dt);
    npcEnt->ApplyPosConstraints(dPos);

    xVec3 facing;
    CalculateFacing(facing, steeringDynamics, dt);

    npcEnt->TurnToFaceXZ(&facing, dt, Params()->turnSpringK);
}

void zCompSteeringGeneric::CalculateAcc(xVec3& acc, zSteeringGenericDynamics& steeringDynamics, F32 dt)
{
    acc = xVec3::m_Null;
    F32 accMag = 0.0f;

    const zCompSteeringGenericParams* params = Params();

    xVec3 destAcc;
    F32 destMag;
    switch (params->steeringType) {
    case eSteeringType_Seek:
        destMag = GetSeekAcc(destAcc, steeringDynamics, dt);
        break;
    default:
        destAcc = xVec3::m_Null;
        destMag = 0.0f;
        xFAIL_MSG_L(268, "This steering type is not implemented yet!");
    }

#if DEBUG
    DebugRenderForce(steeringDynamics.pos, &destAcc, renderDestForce, g_BLUE);
#endif

    if (AccumulateVec(acc, accMag, params->maxAcc, destAcc, destMag))
        return;

    if (params->wanderOn) {
        xVec3 WanderAcc;
        F32 WanderMag = GetWanderAcc(WanderAcc, steeringDynamics, dt);
#if DEBUG
        DebugRenderForce(steeringDynamics.pos, &WanderAcc, renderWanderForce, g_GREEN);
#endif

        if (AccumulateVec(acc, accMag, params->maxAcc, WanderAcc, WanderMag))
            return;
    }
}

void zCompSteeringGeneric::CalculateVelConstraints(xVec3& dVel, zSteeringGenericDynamics&, F32)
{
    dVel = xVec3::m_Null;

    const zCompSteeringGenericParams* params = Params();
}

void zCompSteeringGeneric::CalculatePosConstraints(xVec3& dPos, zSteeringGenericDynamics&, F32)
{
    dPos = xVec3::m_Null;
}

void zCompSteeringGeneric::CalculateFacing(xVec3& facing, zSteeringGenericDynamics& steeringDynamics, F32)
{
    facing = *steeringDynamics.vel;
}
#endif

bool zCompSteeringGeneric::AccumulateVec(xVec3& curTotalVec, F32& curTotalMag, F32 maxMag, const xVec3& vecToAdd, F32 magToAdd)
{
    if (curTotalMag + EPSILON > maxMag)
        return true;

    if (magToAdd < EPSILON)
        return false;

    if (curTotalMag + magToAdd > maxMag) {
        F32 wantedMag = maxMag - curTotalMag;

        xVec3 truncatedVector;
        truncatedVector.Scale(vecToAdd, wantedMag / magToAdd);

        curTotalVec += truncatedVector;
        curTotalMag += wantedMag;
        return true;
    }

    curTotalVec += vecToAdd;
    curTotalMag += magToAdd;
    return false;
}

F32 zCompSteeringGeneric::GetWanderAcc(xVec3& wanderAcc, zSteeringGenericDynamics& steeringDynamics, F32 dt)
{
    const zCompSteeringGenericParams* params = Params();

    wanderAcc = xVec3::m_Null;

    F32 JitterThisTimeSlice = params->wanderJitterPerSec * dt;

    xVec3 RandomVector;
    RandomVector.x = xrand_RandomFloatRange(-1.0f, 1.0f);
    RandomVector.y = 0.0f;
    RandomVector.z = xrand_RandomFloatRange(-1.0f, 1.0f);
    RandomVector.Scale(JitterThisTimeSlice);

    prevWanderDir += RandomVector;

    prevWanderDir.normalize();

    F32 magForward = params->maxAcc * params->wanderRatio;
#if RELEASE
    xVec3 wanderForceLocal;
    wanderForceLocal.Scale(prevWanderDir, magForward * params->wanderFOV);
#else
    F32 magRandom = magForward * params->wanderFOV;

    xVec3 wanderForceLocal;
    wanderForceLocal.Scale(prevWanderDir, magRandom);
#endif

    wanderForceLocal.z += magForward;

    zNPCHelper::RotateLocalToWorld(wanderAcc, wanderForceLocal, *steeringDynamics.heading);

    return wanderAcc.length();
}

#if RELEASE
F32 zCompSteeringGeneric::GetWallAvoidenceAcc(xVec3& avoidenceForce, zSteeringGenericDynamics&, F32)
{
    avoidenceForce = xVec3::m_Null;

    if (!swCircle.hitSomething)
        return 0.0f;

    if (npcWalls == NULL || wallID < 0)
        return 0.0f;

    F32 avoidenceForceScale = (1.0f - swCircle.curDist / swCircle.dist) * Params()->maxAcc;
    xVec2* wallNormal = npcWalls->GetWallNormal(wallID);
    avoidenceForce.x = wallNormal->x * avoidenceForceScale;
    avoidenceForce.y = 0.0f;
    avoidenceForce.z = wallNormal->y * avoidenceForceScale;

    return avoidenceForceScale;
}

F32 zCompSteeringGeneric::GetVelCorrectionAcc(xVec3& corAcc, const xVec3& desiredVel, const zSteeringGenericDynamics& steeringDynamics, F32 dt)
{
    corAcc = xVec3::m_Null;

    xVec3 diffVelocity;
    diffVelocity.Sub(desiredVel, *steeringDynamics.vel);
    diffVelocity.y = 0.0f;
    F32 diffVelocityMag = diffVelocity.length();
    if (diffVelocityMag < EPSILON)
        return 0.0f;

    diffVelocity *= 1.0f / diffVelocityMag;

    F32 accMag;
    if (dt < EPSILON) accMag = FLT_MAX; else accMag = diffVelocityMag / dt;

    if (accMag > Params()->maxAcc)
        accMag = Params()->maxAcc;

    corAcc.Scale(diffVelocity, accMag);
    return diffVelocityMag;
}

F32 zCompSteeringGeneric::GetSeekAcc(xVec3& seekAcc, zSteeringGenericDynamics& steeringDynamics, F32 dt)
{
    seekAcc = xVec3::m_Null;

    xASSERT_L(701, destination);
    if (destination) {
        xVec3 desiredVelocity;
        desiredVelocity.Sub(*destination, *steeringDynamics.pos);
        desiredVelocity.at_normalize();
        desiredVelocity.Scale(Params()->maxSpeed);

        return GetVelCorrectionAcc(seekAcc, desiredVelocity, steeringDynamics, dt);
    }

    return 0.0f;
}

bool zCompSteeringGeneric::GetSweptCircleCollDPos(xVec3& collDPos, F32& velCorrectionRatio, zSteeringGenericDynamics& steeringDynamics, F32 dt)
{
    collDPos = xVec3::m_Null;
    velCorrectionRatio = 1.0f;

    xVec3 dir;
    dir.Sub(*steeringDynamics.pos, *steeringDynamics.oldPos);
    F32 dPosLength = dir.safe_normalize(*steeringDynamics.heading);

    xASSERT_L(804, dt<Params()->avoidenceTime);
    F32 sweepDist = steeringDynamics.vel->length();
    sweepDist *= Params()->avoidenceTime;

    swCircle.PrepareXZ(*steeringDynamics.oldPos, steeringDynamics.boundRadius, dir, sweepDist);

    if (npcWalls)
        npcWalls->IntersectsSweptCircle(swCircle, wallID);

    if (swCircle.hitSomething && swCircle.curDist < dPosLength) {
        collDPos.Scale(dir, -(dPosLength - swCircle.curDist));
        velCorrectionRatio = swCircle.curDist / dPosLength;
        return true;
    }

    return false;
}
#else
F32 zCompSteeringGeneric::GetSeekAcc(xVec3& seekAcc, zSteeringGenericDynamics& steeringDynamics, F32 dt)
{
    const zCompSteeringGenericParams* params = Params();

    seekAcc = xVec3::m_Null;

    xASSERT_L(740, destination);
    if (destination) {
        xVec3 desiredVelocity;
        desiredVelocity.Sub(*destination, *steeringDynamics.pos);
        desiredVelocity.at_normalize();
        desiredVelocity.Scale(Params()->maxSpeed);

        xVec3 diffVelocity;
        diffVelocity.Sub(desiredVelocity, *steeringDynamics.vel);
        diffVelocity.y = 0.0f;
        F32 diffVelocityMag = diffVelocity.length();
        if (diffVelocityMag < EPSILON)
            return 0.0f;

        diffVelocity *= 1.0f / diffVelocityMag;

        F32 accMag;
        if (dt < EPSILON)
            accMag = FLT_MAX;
        else
            accMag = diffVelocityMag / dt;

        if (accMag > params->maxAcc)
            accMag = params->maxAcc;

        seekAcc.Scale(diffVelocity, accMag);
        return diffVelocityMag;
    }

    return 0.0f;
}
#endif
