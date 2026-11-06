#ifndef XLIGHT_H
#define XLIGHT_H

#include "xBase.h"
#include "xLightAsset.h"
#include "iLight.h"

struct xModelInstance;
struct xEnv;
class xLightEffectBase;

#define f_XLIGHT_ON (1u<<0)
#define f_XLIGHT_USED (1u<<1)

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
    bool drawDebugSpherePos;
    bool drawDebugSphereRange;
    char fileName[256];
    U32 lineNumber;
#endif
};

void xLightResetAll();
void xLightSceneInit();
void xLightSceneExit();
void xLightSceneReset();
void xLightInit(xBase& data, xDynAsset& asset, size_t asset_size);
void xLightInit(xBase* b, xLightAsset* tasset);
void xLightInitTempLights();

#if DEBUG || RELEASE
xLight* xLightCreateTempInternal(bool useLightCard, const char* fileName, U32 lineNumber);
void xLightDestroyTempInternal(xLight* xl, const char* fileName, U32 lineNumber);
#define xLightCreateTemp_L(useLightCard, line) xLightCreateTempInternal((useLightCard), __FILE__, (line))
#define xLightDestroyTemp_L(xl, line) xLightDestroyTempInternal((xl), __FILE__, (line))
#else
xLight* xLightCreateTempInternal(bool useLightCard);
void xLightDestroyTempInternal(xLight* xl);
#define xLightCreateTemp_L(useLightCard, line) xLightCreateTempInternal((useLightCard))
#define xLightDestroyTemp_L(xl, line) xLightDestroyTempInternal((xl))
#endif

#define xLightCreateTemp(useLightCard) xLightCreateTemp_L(useLightCard, __LINE__)
#define xLightDestroyTemp(xl) xLightDestroyTemp_L(xl, __LINE__)

void xLightResolveLinks();
void xLightDestroyAll();
void xLightDestroy(xLight* xl);
void xLightReset(xLight* xl);
void xLightEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
void xLightUpdateAll(F32 dt);
void xLightRenderAll();
void xLightAddLocalEnv(xEnv* env);
void xLightRemoveLocalEnv(xEnv* env);
void xLightPrepLocal();
void xLightAddLocal(xModelInstance* minst);
void xLightRemoveLocal(xModelInstance* minst);
void xLightSetEffect(xLight* xl, U32 effectID);
void xLightResetEffect(xLight* xl);
void xLightOn(xLight* xl, bool on, F32 transitionTime);
void xLightToggle(xLight* xl, F32 transitionTime);
void xLightSetColor(xLight* xl, F32 r, F32 g, F32 b, F32 transitionTime);
void xLightSetColor(xLight* xl, const xFColor& col, F32 transitionTime);
void xLightGetColor(xLight* xl, xFColor& col);
void xLightSetPos(xLight* xl, const xVec3& pos);
void xLightSetRadius(xLight* xl, F32 radius);
void xLightSetLightCardScale(xLight* xl, F32 scale); // UNUSED
void xLightFlash(const xVec3& pos, const xFColor& color, F32 radius, F32 upTime, F32 maxTime, F32 downTime);
#if DEBUG || RELEASE
void xLightDebugRender();
void xLightDebugReset(xLight* light);
#endif

#endif
