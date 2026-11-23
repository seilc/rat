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

#define MAX_LIGHTWEIGHT_SYSTEMS 10

class zLightweightManager
{
public:
    static void Setup();
    static void Exit();
    static void Reset();
    static void Update(F32 dt);
    static void Render();
    static void PostRender2d();
    static void PostRender3d();
    static void SceneSetup();
    static void RegisterLightweight(zLightweightSystemBase* newSystem);

    static S32 systemCount;
    static zLightweightSystemBase* system[MAX_LIGHTWEIGHT_SYSTEMS];
};

#endif
