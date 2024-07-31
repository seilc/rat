#ifndef XRUMBLEEMITTER_H
#define XRUMBLEEMITTER_H

#include "xBase.h"
#include "xDynAsset.h"
#include "xVec3.h"

namespace xRumble {

struct effectAsset : xDynAsset
{
    F32 time;
    F32 intensity;
    U32 id;
    U8 priority;
    U8 type;
    U8 rumbleInPause;
    U8 pad;
    F32 param1;
    F32 param2;
    F32 shakeMagnitude;
    F32 shakeCycleMax;
    F32 shakeRotationalMagnitude;
    bool shakeY;

    F32 GetIntensity(F32 currentTime, F32 totalTime) const;
};

struct emitterBase : xBase
{
    effectAsset* pEffectAsset;
    
    virtual bool GetIntensity(const xVec3& playerPos, F32& intensity) const;
    virtual void SetIntensity(F32 intensity) const;
};

}

#endif
