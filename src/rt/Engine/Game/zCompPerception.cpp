#include "xCommon.h"
#include "zCompPerception.h"

#include "xGlobals.h"
#include "zCommonPlayer.h"
#include "zCompPerceptionParams.h"
#include "zICompNPCEntity.h"
#include "zNPCHelper.h"
#include "zScene.h"

#if RELEASE
static zCompPerceptionParams gDefaultPerceptionParams;

zCompPerceptionParams::zCompPerceptionParams()
{
    perceivedEntity = 0;
    setToTemplate = 0;
    detectedUpdateFrequency = 0.5f;
    lostUpdateFrequency = 0.5f;
    perceptionMemoryTime = 5.0f;
    flags = 0;
    sphereR = 1.0f;
    sphereRatio = 1.25f;
    soundSphereR = 1.0f;
    soundSphereRatio = 1.25f;
    soundSphereMinNoise = 0.5f;
    cylinderR = 1.0f;
    cylinderHUp = 1.0f;
    cylinderHDown = 1.0f;
    cylinderRatio = 1.25f;
    coneTheta = 30.0f;
    coneD = 1.0f;
    coneRatio = 1.25f;
    triTheta = 30.0f;
    triD = 1.0f;
    triHUp = 1.0f;
    triHDown = 1.0f;
    triRatio = 1.25f;
    losConeTheta = 30.0f;
    detectionAction = 0;
    lostAction = 0;
}

zCompPerceptionParams* zCompPerceptionParams::GetParamsFromAsset(U32 aid)
{
    U32 assetSize;
    zCompPerceptionParams* params = (zCompPerceptionParams*)xSTFindAsset(aid, &assetSize);
    xASSERT_L(79, assetSize == sizeof(zCompPerceptionParams));
    return params;
}
#endif

class zCompPerceptionDynamics
{
    void CalculateDiff()
    {
        diffVector.Sub(*perceivedObjPos, *npcPos);
        distance2 = diffVector.length2();
        diffCalculated = true;
    }

    void CalculateDiffXZ()
    {
        diffVectorXZ.Sub(*perceivedObjPos, *npcPos);
        diffY = diffVectorXZ.y;
        diffVectorXZ.y = 0.0f;
        distanceXZ2 = diffVectorXZ.length2();
        diffXZCalculated = true;
    }

    void CalculateTanTheta()
    {
        if (!diffCalculated)
            CalculateDiff();

        xVec3 diffLocal;
        zNPCHelper::RotateWorldToLocal(diffLocal, diffVector, *npcAt);

        F32 oneOverZ = 1.0f / diffLocal.z;
        tanThetaXZ = diffLocal.x * oneOverZ;
        tanThetaYZ = diffLocal.y * oneOverZ;
        tanTheta2 = xsqr(tanThetaXZ) + xsqr(tanThetaYZ);
        tanThetaCalculated = true;
    }

    inline void CalculateInNPCWalls();

public:
    bool Reset(zICompNPCEntity* npcEnt, xEnt* perceivedObj)
    {
        if (!perceivedObj)
            return false;
        if (!npcEnt)
            return false;

        this->npcEnt = npcEnt;
        this->perceivedObj = perceivedObj;
        npcPos = &npcEnt->frame->mat.pos;
        npcAt = &npcEnt->frame->mat.at;
        perceivedObjPos = xEntGetPos(perceivedObj);

        diffCalculated = false;
        diffXZCalculated = false;
        tanThetaCalculated = false;
        inNPCWallsCalculated = false;
        return true;
    }

    F32 GetDistance2()
    {
        if (!diffCalculated)
            CalculateDiff();
        return distance2;
    }

    F32 GetDiffY()
    {
        if (!diffXZCalculated)
            CalculateDiffXZ();
        return diffY;
    }

    F32 GetDistanceXZ2()
    {
        if (!diffXZCalculated)
            CalculateDiffXZ();
        return distanceXZ2;
    }

    F32 GetTanTheta2()
    {
        if (!tanThetaCalculated)
            CalculateTanTheta();
        return tanTheta2;
    }

private:
    zICompNPCEntity* npcEnt;
    xEnt* perceivedObj;
    xVec3* npcPos;
    xVec3* npcAt;
    xVec3* perceivedObjPos;
    xVec3 diffVector;
    F32 distance2;
    xVec3 diffVectorXZ;
    F32 distanceXZ2;
    F32 diffY;
    F32 tanTheta2;
    F32 tanThetaXZ;
    F32 tanThetaYZ;
    bool inNPCWalls;
    bool diffCalculated;
    bool diffXZCalculated;
    bool tanThetaCalculated;
    bool inNPCWallsCalculated;
};

zCompPerception::zCompPerception()
{
    params = NULL;

    updateTimer = 0.0f;
    perceivedTimer = 0.0f;
    lostTimer = 0.0f;

    perceivedEntity = NULL;

    perceived = false;
    remembered = false;
    perceptionType = eCompPerceptionType_None;

    tanConeHTheta = 0.0f;
    tanTriHTheta = 0.0f;
    tanLosHTheta = 0.0f;
}

void zCompPerception::Attached()
{
    zNPCBase* ownerNPC = owner;
    const zNPCAssetRATS* npcAsset = ownerNPC->GetNPCAsset();

    if (!params)
#if RELEASE
        params = &gDefaultPerceptionParams;
#else
        params = zCompPerceptionParams::spNPCGenericDefault;
#endif

    if (npcAsset->npcPerception)
        params = zCompPerceptionParams::GetParamsFromAsset(npcAsset->npcPerception);
}

void zCompPerception::AllAttached()
{
    if (!params->perceivedEntity) {
        perceivedEntity = CURPLAYERS[0];
    } else {
        xBase* b = zSceneFindObject(params->perceivedEntity);

        if (!b || !xBaseIsEntity(b)) {
            xFAIL_MSG_L(DEBUG ? 258 : 314, "There is a problem with the perceived object!");
            return;
        }

        perceivedEntity = (xEnt*)b;
    }

    tanConeHTheta = itan((PI / 180.0f) * params->coneTheta);
    tanTriHTheta = itan((PI / 180.0f) * params->triTheta);
    tanLosHTheta = itan((PI / 180.0f) * params->losConeTheta);
}

void zCompPerception::Detached()
{
    updateTimer = 0.0f;
    perceivedTimer = 0.0f;
    lostTimer = 0.0f;
    perceivedEntity = NULL;

    perceived = false;
    remembered = false;
    perceptionType = eCompPerceptionType_None;

    tanConeHTheta = 0.0f;
    tanTriHTheta = 0.0f;
    tanLosHTheta = 0.0f;
}

#if RELEASE
void zCompPerception::Reset()
{
    updateTimer = 0.0f;
    perceivedTimer = 0.0f;
    lostTimer = 0.0f;
    perceived = false;
    remembered = false;
    perceptionType = eCompPerceptionType_None;
}
#endif

void zCompPerception::Perceived(eCompPerceptionType perceptionType, xEnt* perceivedEnt, F32 dt)
{
    perceivedLocation = *xEntGetPos(perceivedEnt);
    perceivedTimer += dt;
    lostTimer = 0.0f;

    perceived = true;
    remembered = true;
    this->perceptionType = perceptionType;
}

void zCompPerception::NotPerceived(F32 dt)
{
    perceivedTimer = 0.0f;
    lostTimer += dt;
    perceived = false;

    if (remembered)
        remembered = lostTimer < params->perceptionMemoryTime;

    perceptionType = eCompPerceptionType_None;
}

void zCompPerception::PreUpdate(xScene*, F32 dt)
{
    updateTimer += dt;

    if (remembered) {
        if (updateTimer < params->detectedUpdateFrequency)
            return;
    } else {
        if (updateTimer < params->lostUpdateFrequency)
            return;
    }

    updateTimer = 0.0f;

    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    xASSERT_L(DEBUG ? 335 : 402, compNPCEntity);

    zCompPerceptionDynamics perceptionDynamics;
    if (!perceptionDynamics.Reset(compNPCEntity, perceivedEntity))
        return;

    if (params->flags & k_ZCOMPPERCEPTIONPARAMS_SPHERE) {
        if (CheckSpherePerception(perceptionDynamics)) {
            Perceived(eCompPerceptionType_Sphere, perceivedEntity, dt);
            return;
        }
    }

    if (params->flags & k_ZCOMPPERCEPTIONPARAMS_SOUND_SPHERE) {
        if (CheckSoundSpherePerception(perceptionDynamics)) {
            Perceived(eCompPerceptionType_SoundSphere, perceivedEntity, dt);
            return;
        }
    }

    if (params->flags & k_ZCOMPPERCEPTIONPARAMS_CYLINDER) {
        if (CheckCylinderPerception(perceptionDynamics)) {
            Perceived(eCompPerceptionType_Cylinder, perceivedEntity, dt);
            return;
        }
    }

    if (params->flags & k_ZCOMPPERCEPTIONPARAMS_CONE) {
        if (CheckConePerception(perceptionDynamics)) {
            Perceived(eCompPerceptionType_Cone, perceivedEntity, dt);
            return;
        }
    }

    if (params->flags & k_ZCOMPPERCEPTIONPARAMS_TRI) {
        if (CheckTriPerception(perceptionDynamics)) {
            Perceived(eCompPerceptionType_Tri, perceivedEntity, dt);
            return;
        }
    }

    NotPerceived(dt);
}

bool zCompPerception::CheckSpherePerception(zCompPerceptionDynamics& percpDyn)
{
    F32 checkDistance = params->sphereR;

    if (remembered)
        checkDistance *= params->sphereRatio;

    F32 checkDistance2 = xsqr(checkDistance);

    return percpDyn.GetDistance2() < checkDistance2;
}

bool zCompPerception::CheckSoundSpherePerception(zCompPerceptionDynamics& percpDyn)
{
    if (params->perceivedEntity) {
        xFAIL_MSG_L(DEBUG ? 430 : 497, "sound sphere perception works against only player!");
        return false;
    }

    F32 noise = ((zCommonPlayer*)CURPLAYERS[0])->GetNoiseLevel();

    if (noise < params->soundSphereMinNoise)
        return false;

    F32 checkDistance = params->soundSphereR;

    if (remembered)
        checkDistance *= params->soundSphereRatio;

    F32 checkDistance2 = xsqr(checkDistance);

    return percpDyn.GetDistance2() < checkDistance2;
}

bool zCompPerception::CheckCylinderPerception(zCompPerceptionDynamics& percpDyn)
{
    F32 cylinderR = params->cylinderR, cylinderHUp = params->cylinderHUp, cylinderHDown = params->cylinderHDown;

    if (remembered) {
        F32 cylinderRatio = params->cylinderRatio;
        cylinderR *= cylinderRatio;
        cylinderHUp *= cylinderRatio;
        cylinderHDown *= cylinderRatio;
    }

    F32 diffY = percpDyn.GetDiffY();
    if (diffY < -cylinderHDown || diffY > cylinderHUp)
        return false;

    F32 cylinderR2 = xsqr(cylinderR);
    return percpDyn.GetDistanceXZ2() < cylinderR2;
}

bool zCompPerception::CheckConePerception(zCompPerceptionDynamics& percpDyn)
{
    F32 hTheta = tanConeHTheta, coneD = params->coneD;

    if (remembered) {
        F32 coneRatio = params->coneRatio;
        hTheta *= coneRatio;
        coneD *= coneRatio;
    }

    F32 coneD2 = xsqr(coneD);
    if (percpDyn.GetDistance2() > coneD2)
        return false;

    F32 hTheta2 = xsqr(hTheta);

    return percpDyn.GetTanTheta2() < hTheta2;
}

bool zCompPerception::CheckTriPerception(zCompPerceptionDynamics& percpDyn)
{
    F32 hTheta = tanTriHTheta, triD = params->triD, triHUp = params->triHUp, triHDown = params->triHDown;

    if (remembered) {
        F32 triRatio = params->triRatio;
        hTheta *= triRatio;
        triD *= triRatio;
        triHUp *= triRatio;
        triHDown *= triRatio;
    }

    F32 diffY = percpDyn.GetDiffY();
    if (diffY < -triHDown || diffY > triHUp)
        return false;

    F32 triD2 = xsqr(triD);
    if (percpDyn.GetDistanceXZ2() > triD2)
        return false;

    F32 hTheta2 = xsqr(hTheta);

    return percpDyn.GetTanTheta2() < hTheta2;
}

#if DEBUG
void zCompPerception::AddStaticTweaks()
{
}
#endif
