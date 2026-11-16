#ifndef ZLIGHTWEIGHT_H
#define ZLIGHTWEIGHT_H

#include "types.h"

class zLightweightSystemBase
{
public:
    virtual void SceneSetup() {}
    inline virtual void Reset();
    inline virtual void Update(F32);
    inline virtual void Render();
    virtual void PostRender2d() const {}
    virtual void PostRender3d() const {}
    inline virtual void Exit();
};

class zLightweightManager
{
public:
    static void RegisterLightweight(zLightweightSystemBase* newSystem);
};

#endif
