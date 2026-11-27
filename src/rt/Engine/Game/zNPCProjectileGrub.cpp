#include "xCommon.h"
#include "zNPCProjectileGrub.h"

#include "xClumpColl.h"
#include "xCollideSweptSphere.h"
#include "xEntMotion.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xModel.h"
#include "xScene.h"
#include "xSndMgr.h"
#include "xSpacePartitionTree.h"
#include "xstransvc.h"
#include "xString.h"
#include "xSurface.h"
#include "zBase.h"
#include "zBomb.h"
#include "zEnv.h"
#include "zGlobals.h"
#include "zNPCCommon.h"
#include "zNPCProjectileFrozen.h"
#include "zPlayer.h"
#include "zScene.h"
#include "zShrapnel.h"

#include <string.h>

namespace
{
    xTWEAKABLE F32 GRUB_SPEED = 5.0f;
    xTWEAKABLE F32 GRUB_EXPLOSION_RADIUS = 0.5f;
    xTWEAKABLE F32 GRUB_AWAY_KNOCKBACK = 10.0f;
    xTWEAKABLE F32 GRUB_UP_KNOCKBACK = 5.0f;
    xTWEAKABLE F32 GRUB_TRAIL_OFFSET = -0.3f;
    xTWEAKABLE F32 GRUB_SHADOW_OFFSET = 0.3f;
    xTWEAKABLE F32 GRUB_DECAL_SIZE = 2.0f;
    xTWEAKABLE F32 GRUB_LIFETIME = 10.0f;
    xTWEAKABLE F32 GRUB_RADIUS = 0.24f;

    const xDecal::curve_node decalCurve[5] = {
        { 0.0f, { 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f } },
        { 0.025f, { 1.0f, 1.0f, 1.0f, 0.8f, 1.0f, 1.0f } },
        { 0.4f, { 1.0f, 1.0f, 1.0f, 0.8f, 1.0f, 1.0f } },
        { 0.8f, { 1.0f, 1.0f, 1.0f, 0.27f, 1.0f, 1.0f } },
        { 1.0f, { 1.0f, 1.0f, 1.0f, 0.0f, 0.7f, 0.7f } },
    };

    xTWEAKABLE U32 FLY_SOUND_ID = xStrHash("GRUBFLY");
    xTWEAKABLE U32 LAUNCH_SOUND_ID = xStrHash("GRUBLAUNCH");
    xTWEAKABLE U32 SPLAT_SOUND_ID = xStrHash("GRUBHIT");
    const U32 GRUB_BUG_MODEL_ID = xStrHash("proj_grub_reg");
    const U32 GRUB_BUG_SURFACE_ID = xStrHash("FX_GRUB");

    zNPCProjectileGrubTrail::TrailConfig PuffConfig = {
        -HUGE, -0.2f, 0.2f, 0.8f, { 0.0f, 0.0f, 0.0f }, 20.0f, -2.0f, 2.0f, 0.5f, 1.0f, 0.6f, 0.0f, 1.0f, 0.0f, false
    };

    zNPCProjectileGrubTrail::TrailConfig ShortTrailConfig = {
        -HUGE, -0.15f, 0.1f, 0.6f, { 0.0f, 0.0f, 0.0f }, 40.0f, -2.0f, 2.0f, 0.7f, 0.25f, 0.5f, 0.0f, 1.0f, 0.0f, true
    };
}

zNPCProjectileGrubArc::ArcConfig zNPCProjectileGrubArcPlayerConfig = { 0.5f, 1.0f, 0.3f, 0.6f, 5.0f, 0.12f, true, true };
zNPCProjectileGrubArc::ArcConfig zNPCProjectileGrubArcNPCConfig = { 0.5f, 1.0f, 0.8f, 1.5f, 10.0f, 0.12f, true, true };

xSndGroupHandle zNPCProjectileGrub::launchSoundGroup = ISND_INVALID_SND_GROUP_HANDLE;
xSndGroupHandle zNPCProjectileGrub::flyingSoundGroup = ISND_INVALID_SND_GROUP_HANDLE;
xSndGroupHandle zNPCProjectileGrub::splatSoundGroup = ISND_INVALID_SND_GROUP_HANDLE;

xSurface* grubScrollingSurf;

F32 zNPCProjectileGrub::GetRadius() { return GRUB_RADIUS; }

void zNPCProjectileGrub::Freeze()
{
    xASSERT_L(127, bugModel);

    xVec3 velocity;
    velocity.Scale(dir, speed);
    zNPCProjectileFreeze(*(xMat4x3*)bugModel->Mat, velocity, ezNPCProjectileFrozenType_Grub);
    Destroy();
}

void zNPCProjectileGrub::StaticSetup()
{
    flyingSoundGroup = xSndMgrGetSoundGroup(FLY_SOUND_ID);
    launchSoundGroup = xSndMgrGetSoundGroup(LAUNCH_SOUND_ID);
    splatSoundGroup = xSndMgrGetSoundGroup(SPLAT_SOUND_ID);

    grubScrollingSurf = (xSurface*)xSceneResolvID(globals.sceneCur, GRUB_BUG_SURFACE_ID);
    xASSERT_FMT_L(142, grubScrollingSurf, "Missing surface.  Get boot and repack.");
    grubScrollingSurf->type = 2;

    zNPCProjectileGrubTrail::SceneSetup();
}

void zNPCProjectileGrub::Reset()
{
    Destroy();
}

void zNPCProjectileGrub::Update(F32 dt)
{
    lifetime -= dt;
    if (lifetime <= 0.0f) {
        Destroy();
        return;
    }

    xVec3 velocity, dpos;
    velocity.Scale(dir, speed);
    dpos.Scale(velocity, dt);
    Collide(dpos, dt);
    if (!IsActive())
        return;

    position += dpos;

    xVec3 trail_pos;
    xVec3Copy(&trail_pos, &position);
    trail_pos.AddScale(((xMat4x3*)bugModel->Mat)->at, GRUB_TRAIL_OFFSET);

    if (lifetime > GRUB_LIFETIME - 0.5f) {
        xVec3 vel0 = { 0.0f, 0.0f, 0.0f };
        zNPCProjectileGrubTrail::Emit(emit_remaining, startPos, vel0, dt, -1.0f, &PuffConfig);
    }

    zNPCProjectileGrubTrail::Emit(emit_remaining, trail_pos, velocity, dt, 0.4f, &ShortTrailConfig);

    zNPCProjectileGrubSparks::Emit(spark_emit_remaining, trail_pos, velocity, dt, NULL);

    if (flyingSound == XSNDMGR_INVALID_SND_HANDLE && flyingSoundGroup != ISND_INVALID_SND_GROUP_HANDLE)
        flyingSound = xSndMgrPlay(flyingSoundGroup, xSnd_FOLLOW, &position, &velocity, NULL, NULL, NULL);

    xMat4x3 shadowMat;
    xMat4x3Copy(&shadowMat, (xMat4x3*)bugModel->Mat);
    shadowMat.pos.AddScale(((xMat4x3*)bugModel->Mat)->at, GRUB_SHADOW_OFFSET);
    xShadowSimple_Add(&simpShadow, &shadowMat, 0.5f, 1.0f);
}

void zNPCProjectileGrub::Render()
{
    xASSERT_L(200, bugModel);
    xMat4x3* frame = (xMat4x3*)bugModel->Mat;
    xVec3Copy(&frame->pos, &position);

    xMat3x3 rotMat;
    xMat3x3RotZ(&rotMat, 2.0f * PI * globals.update_dt);
    xMat3x3Mul(frame, &rotMat, frame);
    xModelRender(bugModel);
}

void zNPCProjectileGrub::Destroy()
{
    active = false;
    if (bugModel) {
        xModelInstanceFree_L(bugModel, true, 214);
        bugModel = NULL;
    }

    if (flyingSound != XSNDMGR_INVALID_SND_HANDLE && xSndMgrIsLooping(flyingSound))
        xSndMgrStop(flyingSound);
}

bool zNPCProjectileGrub::Setup(void* configData)
{
    xASSERT_L(224, configData);
    GrubConfig* config = (GrubConfig*)configData;

    owner = config->owner;
    if (!bugModel) {
        RpAtomic* data = xModelFindAtomic(GRUB_BUG_MODEL_ID, NULL);
        xASSERT_FMT_L(231, data, "Can't find model for grub");
        if (!data)
            return false;

        bugModel = xModelInstanceAlloc(data, NULL, 0, 0, NULL);
        xModelInstanceSetIDSafe_L(236, bugModel, GRUB_BUG_MODEL_ID);
        if (!bugModel)
            return false;

        zEnv* zen = globals.sceneCur->zen;
        xEnvAsset* easset = zen->easset;

        xASSERT_L(243, easset->objectLightKit);
        xModelSetLightKit(bugModel, (xLightKit*)xSTFindAsset(easset->objectLightKit, NULL), 0.0f);
    }

    position = config->startPos;
    startPos = config->startPos;
    dir = config->dir;
    dir.safe_normalize(g_NZ3);

    lifetime = GRUB_LIFETIME;

    speed = config->speed <= 0.0f ? GRUB_SPEED : config->speed;

    if (bugModel) {
        xMat4x3* frame = (xMat4x3*)bugModel->Mat;

        xVec3 up = { 0.0f, 1.0f, 0.0f };
        xVec3Copy(&frame->pos, &position);
        xVec3Copy(&frame->at, &dir);
        frame->right.cross(up, dir);
        frame->up.cross(dir, frame->right);
        bugModel->Surf = grubScrollingSurf;
    }

    memset(&decalContext, 0, sizeof(decalContext));
    decalResponseCurve.reset(6, decalCurve, ARRAY_LENGTH(decalCurve));

    decalContext.flags = xDecal::emit_context::FLAG_ANIM_COLOR | xDecal::emit_context::FLAG_ANIM_UV;
    decalContext.blend_mode = xDecal::BLEND_MODE_AVERAGE;
    decalContext.curve = &decalResponseCurve;
    decalContext.uv[0].assign(0.0f, 0.0f);
    decalContext.uv[1].assign(1.0f, 1.0f);
    decalContext.texture = (RwTexture*)xSTFindAsset(xStrHash("fx_grub_splat_reg"), NULL);

    decalContext.life = 5.0f;
    decalContext.max_dist = 3.0f;
    decalContext.size.assign(GRUB_DECAL_SIZE, GRUB_DECAL_SIZE);

    xShadowSimple_CacheInit(&simpShadow, 0x50);

    damage = config->damage;
    emit_remaining = 0.0f;
    spark_emit_remaining = 0.0f;
    arc_emit_remaining = 0.0f;
    launchGoo = (zShrapnelAsset*)xSTFindAsset(xStrHash("grub_launch_goo"), NULL);
    hitGoo = (zShrapnelAsset*)xSTFindAsset(xStrHash("grub_hit_goo"), NULL);

    xVec3 vel = { 0.0f, 0.0f, 0.0f };
    if (launchGoo && false)
        launchGoo->initCB(launchGoo, bugModel, NULL, &vel, NULL);

    flyingSound = XSNDMGR_INVALID_SND_HANDLE;
    xSndMgrPlay(launchSoundGroup, 0, NULL, NULL, NULL, NULL, NULL);
    active = true;
    return true;
}

void zNPCProjectileGrub::Collide(const xVec3& dpos, F32)
{
    xVec3 velocity; velocity.Scale(dir, speed);
    if (xfeq0(velocity.length2()))
        return;

    xSweptSphere sws;
    memset(&sws, 0, sizeof(sws));

    xVec3 start = position, end;
    end.Add(position, dpos);

    U32 old_filter_flags = xClumpColl_SetFilterFlags(0, 12, 12);

    xSweptSpherePrepare(&sws, &start, &end, GetRadius(), 1);

    S32 result = xSweptSphereToStatDyn(&sws, xglobals->sceneCur, owner, k_XENT_COLLTYPE_PC, 0) ||
                 xSweptSphereToEnv(&sws, xglobals->sceneCur->env);

    xClumpColl_RestoreFilterFlags(old_filter_flags);

    if (result) {
        xSweptSphereGetResults(&sws);

        Explode(true, NULL);

        xMat4x3 mat;
        xVec3Copy(&mat.pos, &sws.worldContact);
        mat.pos.AddScale(sws.worldNormal, 0.5f * decalContext.max_dist);

        xVec3 at;
        xVec3Copy(&at, &sws.worldNormal);
        xMat3x3LookVec(&mat, &at);
        xDecal::emit(decalContext, mat, k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_ENV);

        return;
    }

    if (xSweptSphereToNPC(&sws, xglobals->sceneCur, owner, k_XENT_COLLTYPE_PC, 0)) {
        xSweptSphereGetResults(&sws);

        Explode(false, (xEnt*)sws.optr);
        return;
    }

    for (S32 i = 0; i < CURPLAYERS.size(); i++) {
        const xSphere* extra_spheres[zPlayer::MAX_EXTRA_SPHERES];
        S32 num_spheres = CURPLAYERS[i]->GetCollisionSpheres(extra_spheres);
        for (S32 j = 0; j < num_spheres; j++)
            if (xSweptSphereToSphere(&sws, extra_spheres[j])) {
                Explode(true, CURPLAYERS[i]);
                return;
            }
    }

    xClumpColl_RestoreFilterFlags(old_filter_flags);
}

void zNPCProjectileGrub::Explode(bool inflictDamage, const xEnt* ent)
{
    if (inflictDamage)
        sphere_damage(owner, position, GRUB_EXPLOSION_RADIUS, damage, damage, damage, GRUB_AWAY_KNOCKBACK,
                      GRUB_UP_KNOCKBACK, zHS_ENEMY_DAMAGE, zHT_GENERAL);

    if (ent && ent->baseType == eBaseTypePlayer)
        zNPCProjectileGrubArc::Emit(arc_emit_remaining, &zNPCProjectileGrubArcPlayerConfig, ent->model, NULL, -1.0f, -1);
    else if (ent && ent->baseType == eBaseTypeNPC)
        zNPCProjectileGrubArc::Emit(arc_emit_remaining, &zNPCProjectileGrubArcNPCConfig, ent->model, NULL, -1.0f, -1);

    xVec3 vel = { 0.0f, 0.0f, 0.0f };
    xMat4x3* frame = (xMat4x3*)bugModel->Mat;
    xMat4x3 fxFrame;
    xMat4x3Copy(&fxFrame, frame);
    fxFrame.pos.AddScale(fxFrame.at, 0.5f);
    zNPCProjectileGrubHitExplosionEmit(fxFrame);
    zNPCProjectileGrubSparks::Emit(spark_emit_remaining, fxFrame.pos, vel, 1.0f, &burstSparkConfig);

    if (hitGoo)
        hitGoo->initCB(hitGoo, bugModel, NULL, &vel, NULL);
    if (flyingSound != XSNDMGR_INVALID_SND_HANDLE && xSndMgrIsLooping(flyingSound))
        xSndMgrStop(flyingSound);
    xSndMgrPlay(splatSoundGroup, 0, NULL, NULL, NULL, NULL, NULL);
    active = false;
}

bool zNPCProjectileGrub::LaunchFromEnt(xEnt* ent, const xVec3* localOffset, F32 speed, xEnt* refObj)
{
    GrubConfig config;
    memset(&config, 0, sizeof(config));

    config.owner = ent;
    config.damage = 20;
    config.speed = speed;

    xMat4x3* mat;
    if (refObj)
        mat = (xMat4x3*)refObj->frame;
    else
        mat = (xMat4x3*)ent->frame;

    xVec3 offset;
    xMat3x3RMulVec(&offset, mat, localOffset);
    xVec3Add(&config.startPos, &mat->pos, &offset);
    xVec3Copy(&config.dir, &mat->at);

    return Setup(&config);
}

#if DEBUG || RELEASE
void zNPCProjectileGrub::AddStaticTweaks()
{
}
#endif
