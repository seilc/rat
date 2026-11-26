#ifndef ZNPCPROJECTILEMETALSHEET_H
#define ZNPCPROJECTILEMETALSHEET_H

#include "xBound.h"
#include "xMath3.h"
#include "xShadowSimple.h"
#include "xSnd.h"
#include "zNPCProjectile.h"

struct xModelInstance;
struct zShrapnelAsset;
struct RpAtomic;

struct MetalSheetConfig
{
    xEnt* owner;
    xMat4x3 startMat;
    U32 attachBone;
};

#define METAL_SHEET_MAX_PROJECTILES 20

class zNPCProjectileMetalSheet
    : public zNPCProjectileBase<zNPCProjectileMetalSheet, eProjectileMetalSheet, METAL_SHEET_MAX_PROJECTILES>
{
public:
    static void StaticSetup();
#if DEBUG || RELEASE
    static void AddStaticTweaks();
#endif

    virtual void Reset();
    void PlayDestroySound();
    virtual void Update(F32 dt);
    virtual void Render();
    virtual void Destroy();
    virtual bool Setup(void* configData);
    void FlyAway(const xVec3& dir); // UNUSED
    void GetCurrentDirection(xVec3& dir); // UNUSED
    void Collide(const xVec3& dpos, F32);
    void Hit();
    virtual void Freeze() {}
    virtual bool IsActive() const { return active; }

private:
    static RpAtomic* modelFile;
    static xBound modelOOBBound;
    static F32 radius;
    static zShrapnelAsset* explosion_shrapnel;

    xEnt* owner;
    U32 modelID;
    xModelInstance* model;
    bool active;
    U32 attachBone;
    F32 lifetime;
    xVec3 direction;
    xBound bound;
    U32 currentState;
    xVec3 prevPosition;
    xVec3 prevPrevPosition;
    xShadowSimpleCache simpShadow;
    xSndHandle sheetSoundHandle;
};

#endif
