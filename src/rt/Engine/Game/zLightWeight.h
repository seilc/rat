#ifndef ZLIGHTWEIGHT_H
#define ZLIGHTWEIGHT_H

#include "types.h"

class zLightweightSystemBase
{
public:
    virtual void SceneSetup() {}
    virtual void Reset() {}
    virtual void Update(F32) {}
    virtual void Render() {}
    virtual void PostRender2d() const {}
    virtual void PostRender3d() const {}
    virtual void Exit() {}
};

class zLightweightManager
{
public:
    static void RegisterLightweight(zLightweightSystemBase* newSystem);
};

#endif
