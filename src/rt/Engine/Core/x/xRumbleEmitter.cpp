#include "xRumbleEmitter.h"

#include "zEvent.h"
#include "xTRC.h"
#include "xRumbleManager.h"
#include "xDebugTweak.h"
#include "xGlobals.h"
#include "zScene.h"
#include "zPlayer.h"

namespace {

void emitter_EventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase*, U32)
{
    switch (toEvent) {
    case eEventStartRumbleEffect:
    {
        xRumble::emitterBase* pEffect = (xRumble::emitterBase*)to;
        S32 startidx = 0;
        while (xTRCPadFindFirst(startidx, TRC_PadActive)) {
            S32 padPort = xTRCPadGetPadPort(startidx);
            xASSERT(51, padPort >= 0 && padPort < k_MAX_TRC_PADS);
            xRumble::Manager::Get().Add(&mPad[padPort], pEffect);
            startidx++;
        }
        break;
    }
    case eEventStopRumbleEffect:
    {
        xASSERTM(60, (toParam && (toParam[0] >= 0.0f) && (toParam[0] < 4.0f)),
                   "No such pad exists, so of course it can't rumble");
        
        xRumble::emitterBase* pEffect = (xRumble::emitterBase*)to;
        S32 startidx = 0;
        while (xTRCPadFindFirst(startidx, TRC_PadActive)) {
            S32 padPort = xTRCPadGetPadPort(startidx);
            xASSERT(68, padPort >= 0 && padPort < k_MAX_TRC_PADS);
            xRumble::Manager::Get().Stop(&mPad[padPort], pEffect);
            startidx++;
        }
        break;
    }
    }
}

}

namespace xRumble {

void effectAsset::AddTweaks(const char* prefix)
{
    xAUTOTWEAKRANGE(prefix, "time", &time, 0, 100, NULL, NULL, 0, false);
    xAUTOTWEAKRANGE(prefix, "intensity", &intensity, 0, 1, NULL, NULL, 0, false);
    xAUTOTWEAKRANGE(prefix, "priority", &priority, 0, 255, NULL, NULL, 0, false);
    xAUTOTWEAKRANGE(prefix, "param1", &param1, 0, 255, NULL, NULL, 0, false);
    xAUTOTWEAKRANGE(prefix, "Screen Shake|magnitude", &shakeMagnitude, 0, 100, NULL, NULL, 0, false);
    xAUTOTWEAKRANGE(prefix, "Screen Shake|cycle max", &shakeCycleMax, 0, 100, NULL, NULL, 0, false);
    xAUTOTWEAKRANGE(prefix, "Screen Shake|Rot mag", &shakeRotationalMagnitude, 0, 100, NULL, NULL, 0, false);
    xAUTOTWEAKBOOL(prefix, "Screen Shake|shake Y axis", &shakeY, NULL, NULL, 0, false);
}

F32 effectAsset::GetIntensity(F32 currentTime, F32 totalTime) const
{
    switch (type) {
    case 0:
        return 1.0f;
    case 1:
    {
        F32 pulsePeriod = param1 + param2;
        if (currentTime - pulsePeriod * (U32)(currentTime / pulsePeriod) < param1) {
            return 1.0f;
        } else {
            return 0.0f;
        }
    }
    case 2:
        return (1.0f - icos(2.0f * PI * param1 * currentTime)) / 2.0f;
    case 3:
        xASSERTM(119, totalTime >= 0.0f, "You cannot use fade in/out with infinite length effects");
        if (totalTime - currentTime <= param1) {
            return (totalTime - currentTime) / param1;
        } else {
            return 1.0f;
        }
    case 4:
        xASSERTM(130, totalTime >= 0.0f, "You cannot use fade in/out with infinite length effects");
        if (currentTime < param1) {
            return currentTime / param1;
        } else {
            return 1.0f;
        }
    default:
        xASSERTM(140, FALSE, "Invalid rumble type");
        return 0.0f;
    }
}

void effect::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate(150, asset, xRumble::effectAsset, "effect:Rumble");

    new (&data) effect((effectAsset*)&asset);
}

effect::effect(effectAsset* pAsset)
    : emitterBase()
{
    xBaseInit(this, pAsset);

    pEffectAsset = pAsset;
    eventFunc = emitter_EventCB;
    
    if (linkCount) {
        link = (xLinkAsset*)((U8*)pAsset + sizeof(effectAsset));
    }

    AddTweaks();
}

void effect::AddTweaks()
{
    char prefix[256];
    sprintf(prefix, "FX|Rumble|Effect|%s|", xSceneID2Name(xglobals->sceneCur, id));
    
    pEffectAsset->AddTweaks(prefix);
}

void boxEmitter::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate(184, asset, xRumble::boxEmitterAsset, "effect:Rumble Box Emitter");

    new (&data) boxEmitter((boxEmitterAsset*)&asset);
}

boxEmitter::boxEmitter(boxEmitterAsset* pAsset)
    : emitterBase()
{
    xBaseInit(this, pAsset);

    U32 size;
    pEffectAsset = (effectAsset*)xSTFindAsset(pAsset->effectID, &size);

    this->pAsset = pAsset;
    eventFunc = emitter_EventCB;

    if (linkCount) {
        link = (xLinkAsset*)((U8*)pAsset + sizeof(boxEmitterAsset));
    }
}

inline bool inBox(const xVec3& v, const xVec3& min, const xVec3& max)
{
    return v.x >= min.x && v.y >= min.y && v.z >= min.z &&
           v.x <= max.x && v.y <= max.y && v.z <= max.z;
}

bool boxEmitter::GetIntensity(const xVec3& playerPos, F32& intensity) const
{
    intensity = 0.0f;

    S32 charIndex = 0;
    if (xglobals->players[1]->Get_humanPlayer() && !xglobals->players[1]->IsDead()) {
        charIndex = 1;
    }

    if (pAsset->onlyOnFloor && !(xglobals->players[charIndex]->collis->colls[0].flags & k_HIT_IT)) {
        return false;
    }

    if (inBox(playerPos, pAsset->minCorner, pAsset->maxCorner)) {
        intensity = pEffectAsset->intensity;
        return true;
    }

    return false;
}

void sphericalEmitter::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate(236, asset, xRumble::sphericalEmitterAsset, "effect:Rumble Spherical Emitter");

    new (&data) sphericalEmitter((sphericalEmitterAsset*)&asset);
}

sphericalEmitter::sphericalEmitter(sphericalEmitterAsset* pAsset)
    : emitterBase()
{
    xBaseInit(this, pAsset);

    this->pAsset = pAsset;
    eventFunc = emitter_EventCB;

    U32 size;
    pEffectAsset = (effectAsset*)xSTFindAsset(pAsset->effectID, &size);

    radiusSqrd = pAsset->radius * pAsset->radius;
    xASSERTM(248, pAsset->radius, "The radius cannot be 0");

    if (linkCount) {
        link = (xLinkAsset*)((U8*)pAsset + sizeof(sphericalEmitterAsset));
    }
}

bool sphericalEmitter::GetIntensity(const xVec3& playerPos, F32& intensity) const
{
    intensity = 0.0f;

    F32 distSqrd = xVec3Dist2(&playerPos, &pAsset->position);

    if (distSqrd > radiusSqrd) {
        return false;
    }

    if (pAsset->onlyRumbleOnY && !xeq(pAsset->position.y / playerPos.y, 1.0f, 0.1f)) {
        return false;
    }

    S32 charIndex = 0;
    if (xglobals->players[1]->Get_humanPlayer() && !xglobals->players[1]->IsDead()) {
        charIndex = 1;
    }
    
    if (pAsset->onlyOnFloor && !xglobals->players[charIndex]->IsFloorColliding()) {
        return false;
    }

    if (pAsset->fallOff) {
        intensity = pEffectAsset->intensity * (1.0f - xsqrt(distSqrd) / pAsset->radius);
    } else {
        intensity = pEffectAsset->intensity;
    }

    return true;
}

void sphericalEmitter::UpdatePosition(const xVec3& playerPos) const
{
    xASSERT(291, pAsset);
    pAsset->position = playerPos;
}

#ifndef NONMATCHING
static void __unused(sphericalEmitterAsset* a)
{
    *a = *a;
    xFAILM(0, "Unable to find FREEZABLE_OBJECT_RUMBLE_EMIT. Please repack BOOT.SDF");
    xFAILM(0, "Unable to find FREEZABLE_OBJECT_RUMBLE. Please repack BOOT.SDF");
    xFAIL_ONCE_M(0, "OnlyOnFloor is ignored by the FreezableObject Rumble Effect.");
}
#endif

}
