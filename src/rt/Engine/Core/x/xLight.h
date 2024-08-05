#ifndef XLIGHT_H
#define XLIGHT_H

#include "xBase.h"
#include "xLightAsset.h"
#include "iLight.h"

struct xModelInstance;
struct xLightEffectBase;

struct xLightEffectFlickerData
{
    xFColor initialColor;
    F32 time;
};

struct xLightEffectStrobeData
{
    xFColor maxColor;
    F32 time;
    U8 stage;
};

struct xLight : xBase
{
    U32 flags;
    xLightAsset* tasset;
    iLight light;
    xBase* attachedTo;
    xVec3 attachedOffset;
    xModelInstance* lightCard;
    F32 lightCardScale;
    xFColor startColor;
    xFColor endColor;
    F32 transTimeLeft;
    F32 totalTransTime;
    xLightEffectBase* lightEffect;
    bool lightEffectRunning;
    F32 lightEffectSpeed;
    union
    {
        xLightEffectFlickerData flickerData;
        xLightEffectStrobeData strobeData;
    };
#ifdef DEBUGRELEASE
    char effectName[64];
    char attachedToName[64];
    U8 drawDebugSpherePos;
    U8 drawDebugSphereRange;
    char fileName[256];
    U32 lineNumber;
#endif
};

void xLightSetColor(xLight* xl, const xFColor& col, F32 transitionTime);
void xLightGetColor(xLight* xl, xFColor& col);

#endif
