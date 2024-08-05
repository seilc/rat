#ifndef XLIGHTEFFECT_H
#define XLIGHTEFFECT_H

#include "xBase.h"
#include "xDynAsset.h"
#include "xColor.h"

struct xLight;

class xLightEffectBase : public xBase
{
public:
    virtual void StartEffect(xLight& light, bool resetEffect) = 0;
    virtual void StopEffect(xLight& light, bool resetColor) = 0;
    virtual void UpdateEffect(xLight& light, F32 dt) = 0;

#ifdef DEBUGRELEASE
    virtual const char* GetEffectName() { return "Unknown"; }
#endif
};

struct xLightEffectFlickerAsset : xDynAsset
{
    static const U16 VERSION = 1;

    xFColor maxColorVariation;
    F32 flickerRate;
    F32 randomFlickerRate;
    F32 blendTime;
    U32 flags;
};

class xLightEffectFlicker : xLightEffectBase
{
public:
    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);

#ifdef DEBUGRELEASE
    void InitDebugTweak();
#endif

    virtual void StartEffect(xLight& light, bool resetEffect);
    virtual void StopEffect(xLight& light, bool resetColor);
    virtual void UpdateEffect(xLight& light, F32 dt);
#ifdef DEBUGRELEASE
    char effectName[64];
    virtual const char* GetEffectName() { return effectName; }
#endif

private:
    xLightEffectFlickerAsset* tasset;
};

struct xLightEffectStrobeAsset : xDynAsset
{
    static const U16 VERSION = 1;

    xFColor minColor;
    F32 fadeUpTime;
    F32 fadeDownTime;
    F32 timeMax;
    F32 timeMin;
    F32 randomTimeMax;
    F32 randomTimeMin;
};

class xLightEffectStrobe : xLightEffectBase
{
public:
    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);

#ifdef DEBUGRELEASE
    void InitDebugTweak();
#endif

    virtual void StartEffect(xLight& light, bool resetEffect);
    virtual void StopEffect(xLight& light, bool resetColor);
    virtual void UpdateEffect(xLight& light, F32 dt);

#ifdef DEBUGRELEASE
    char effectName[64];
    virtual const char* GetEffectName() { return effectName; }
#endif

private:
    xLightEffectStrobeAsset* tasset;
};

bool xLightEffectIsValid(xBase* base);

#endif
