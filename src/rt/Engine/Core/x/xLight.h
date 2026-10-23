#ifndef XLIGHT_H
#define XLIGHT_H

#include "xBase.h"
#include "xLightAsset.h"
#include "iLight.h"

struct xModelInstance;
class xLightEffectBase;

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
#if DEBUG || RELEASE
    char effectName[64];
    char attachedToName[64];
    U8 drawDebugSpherePos;
    U8 drawDebugSphereRange;
    char fileName[256];
    U32 lineNumber;
#endif
};

#if DEBUG || RELEASE
xLight* xLightCreateTempInternal(bool useLightCard, const char* fileName, U32 lineNumber);
void xLightDestroyTempInternal(xLight* xl, const char* fileName, U32 lineNumber);
#define xLightCreateTemp(useLightCard, line) xLightCreateTempInternal((useLightCard), __FILE__, (line))
#define xLightDestroyTemp(xl, line) xLightDestroyTempInternal((xl), __FILE__, (line))
#else
xLight* xLightCreateTempInternal(bool useLightCard);
void xLightDestroyTempInternal(xLight* xl);
#define xLightCreateTemp(useLightCard, line) xLightCreateTempInternal((useLightCard))
#define xLightDestroyTemp(xl, line) xLightDestroyTempInternal((xl))
#endif
void xLightOn(xLight* xl, bool on, F32 transitionTime);
void xLightSetColor(xLight* xl, F32 r, F32 g, F32 b, F32 transitionTime);
void xLightSetColor(xLight* xl, const xFColor& col, F32 transitionTime);
void xLightGetColor(xLight* xl, xFColor& col);
void xLightSetPos(xLight* xl, const xVec3& pos);
void xLightSetRadius(xLight* xl, F32 radius);

#endif
