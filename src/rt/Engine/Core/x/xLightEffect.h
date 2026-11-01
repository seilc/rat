#ifndef XLIGHTEFFECT_H
#define XLIGHTEFFECT_H

#include "xBase.h"
#include "xLightEffectAsset.h"

struct xLight;

class xLightEffectBase : public xBase
{
public:
    virtual void StartEffect(xLight& light, bool resetEffect) = 0;
    virtual void StopEffect(xLight& light, bool resetColor) = 0;
    virtual void UpdateEffect(xLight& light, F32 dt) = 0;

#if DEBUG || RELEASE
    virtual const char* GetEffectName() { return "Unknown"; }
#endif
};

class xLightEffectFlicker : xLightEffectBase
{
public:
    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);

#if DEBUG || RELEASE
    void InitDebugTweak();
#endif

    virtual void StartEffect(xLight& light, bool resetEffect);
    virtual void StopEffect(xLight& light, bool resetColor);
    virtual void UpdateEffect(xLight& light, F32 dt);
#if DEBUG || RELEASE
    char effectName[64];
    virtual const char* GetEffectName() { return effectName; }
#endif

private:
    xLightEffectFlickerAsset* tasset;
};

class xLightEffectStrobe : xLightEffectBase
{
public:
    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);

#if DEBUG || RELEASE
    void InitDebugTweak();
#endif

    virtual void StartEffect(xLight& light, bool resetEffect);
    virtual void StopEffect(xLight& light, bool resetColor);
    virtual void UpdateEffect(xLight& light, F32 dt);

#if DEBUG || RELEASE
    char effectName[64];
    virtual const char* GetEffectName() { return effectName; }
#endif

private:
    xLightEffectStrobeAsset* tasset;
};

bool xLightEffectIsValid(xBase* base);

#endif
