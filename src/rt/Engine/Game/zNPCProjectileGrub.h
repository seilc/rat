#ifndef ZNPCPROJECTILEGRUB_H
#define ZNPCPROJECTILEGRUB_H

#include "xDecal.h"
#include "xMath.h"
#include "xShadowSimple.h"
#include "xSnd.h"
#include "zNPCProjectile.h"
#include "zNPCProjectileGrubFX.h"

struct xEnt;
struct xModelInstance;
struct zShrapnelAsset;

struct GrubConfig
{
    xEnt* owner;
    xVec3 startPos;
    xVec3 dir;
    S32 damage;
    F32 speed;
};

#define GRUB_MAX_PROJECTILES 40

class zNPCProjectileGrub : public zNPCProjectileBase<zNPCProjectileGrub, eProjectileGrub, GRUB_MAX_PROJECTILES>
{
public:
    virtual F32 GetRadius();
    virtual void Freeze();
    static void StaticSetup();
    virtual void Reset();
    virtual void Update(F32 dt);
    virtual void Render();
    virtual void Destroy();
    virtual bool Setup(void* configData);
    void Collide(const xVec3& dpos, F32);
    virtual void Explode(bool inflictDamage, const xEnt* ent);
    virtual bool LaunchFromEnt(xEnt* ent, const xVec3* localOffset, F32 speed, xEnt* refObj);
#if DEBUG || RELEASE
    static void AddStaticTweaks();
#endif

private:
    static xSndGroupHandle launchSoundGroup;
    static xSndGroupHandle flyingSoundGroup;
    static xSndGroupHandle splatSoundGroup;

    xEnt* owner;
    xModelInstance* bugModel;
    xVec3 startPos;
    xVec3 dir;
    S32 damage;
    F32 lifetime;
    F32 speed;
    F32 emit_remaining;
    F32 spark_emit_remaining;
    F32 arc_emit_remaining;
    zShrapnelAsset* launchGoo;
    zShrapnelAsset* hitGoo;
    xDecal::emit_context decalContext;
    xResponseCurve decalResponseCurve;
    xSndHandle flyingSound;
    xShadowSimpleCache simpShadow;
};

extern zNPCProjectileGrubArc::ArcConfig zNPCProjectileGrubArcPlayerConfig;
extern zNPCProjectileGrubArc::ArcConfig zNPCProjectileGrubArcNPCConfig;

#endif
