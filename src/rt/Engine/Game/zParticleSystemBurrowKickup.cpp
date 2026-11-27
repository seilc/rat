#include "xCommon.h"
#include "zParticleSystemBurrowKickup.h"

#include "containers.h"
#include "xCam.h"
#include "xColor.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xMath.h"
#include "xMath3.h"
#include "xModel.h"
#include "xPtankPool.h"
#include "xRand.h"
#include "xSpacePartitionTree.h"
#include "xstransvc.h"
#include "zHashes.h"
#include "zParticleLocator.h"
#include "zShrapnelAsset.h"

#include <stdio.h>

struct zParticleGeneratorBurrowKickup : zParticleGenerator
{
    void update(F32 dt);
    void create(zParticleSystemBurrowKickup& system, zParticleAsset& asset);
    virtual void reset();
    virtual void activate(xEnt*);
    virtual void deactivate(xEnt*);
    virtual bool cullUpdate();
#if DEBUG || RELEASE
    virtual void add_tweaks(const char* prefix);
    virtual void debug_render(bool all);
#endif

protected:
#if DEBUG || RELEASE
    struct debug
    {
        S32 attemptedEmit;
        S32 failedEmit;
    };
#endif

    zParticleSystemBurrowKickup* system;
    xEnt* attach_to;
    F32 emitted;
    zParticleLocator locator;
    S32 batch_id;
    F32 shrapEmitted;
    zShrapnelAsset* shrapAsset;
    xModelInstance* tempModel;
#if DEBUG || RELEASE
    debug debugInfo;
#endif

    friend class zParticleSystemBurrowKickup;
};

zParticleGenerator* zParticleSystemBurrowKickup::create_generator(zParticleAsset& asset)
{
    zParticleGeneratorBurrowKickup* gen = new (eMemStaticType_0, eMemMgrTag_Particle) zParticleGeneratorBurrowKickup;
    gen->create(*this, asset);
    return gen;
}

void zParticleSystemBurrowKickup::scene_enter()
{
    batch_group.create();
}

void zParticleSystemBurrowKickup::update(F32 dt)
{
    update_generators<zParticleGeneratorBurrowKickup>(dt);
}

S32 zParticleSystemBurrowKickup::get_asset_size() const
{
    return sizeof(asset_data);
}

S32 zParticleSystemBurrowKickup::update_particles(U8* mem, S32 count, ptank_pool& apool, F32 dt, void* context)
{
    FPTANK_BEGIN(pos_color_size_rot_uv2, apool);
    particle_instance* p = (particle_instance*)mem, *end = p + count;

    xASSERT_L(45, context);
    zParticleGeneratorBurrowKickup& g = *(zParticleGeneratorBurrowKickup*)context;

    asset_data& a = *(asset_data*)(g.asset + 1);
    F32 size = a.size;
    F32 lifetime = a.lifetime;
    F32 maxAlpha = a.maxAlpha;

    while (p != end) {
        p->age -= dt;
        if (p->age <= 0.0f) {
            end--;
            *p = *end;
            continue;
        }

        p->vel.y -= a.gravity * dt;
        p->loc.AddScale(p->vel, dt);

        p->rotation += p->rotSpeed * dt;
        while (p->rotation >= PI)
            p->rotation -= 2.0f * PI;
        while (p->rotation < -PI)
            p->rotation += 2.0f * PI;

        xIF_SPHERE_VISIBLE(p->loc.m_RwV3d, size) {
            FPTANK_NEXT(pos_color_size_rot_uv2);

            *pool_pos = p->loc;
            pool_size->assign(size, size);

            F32 alpha = maxAlpha * (p->age / lifetime);

            *pool_color = g_WHITE;
            pool_color->a = alpha;

            *pool_rot = p->rotation;

            F32 u = (p->flags & 0x4) ? 0.5f : 0.0f;
            F32 v = (p->flags & 0x8) ? 0.5f : 0.0f;
            F32 uOffset = (p->flags & 0x1) ? 0.5f : 0.0f;
            F32 vOffset = (p->flags & 0x2) ? 0.5f : 0.0f;
            pool_uv[0].assign(u + uOffset, v + vOffset);
            pool_uv[1].assign(0.5f - u + uOffset, 0.5f - v + vOffset);
        }

        p++;
    }

    FPTANK_END(pos_color_size_rot_uv2);
    return p - (particle_instance*)mem;
}

void zParticleGeneratorBurrowKickup::update(F32 dt)
{
    if (!locator.active())
        return;

    locator.update(dt);

    const zParticleSystemBurrowKickup::asset_data& a = *(zParticleSystemBurrowKickup::asset_data*)(asset + 1);

    xMat4x3 volumeFrame = locator.get_frame();
    xMat3x3 verticalMat;
    xMat3x3RotX(&verticalMat, PI / 2);
    xMat3x3Mul(&volumeFrame, &verticalMat, &volumeFrame);

    xVec3 center;
    locator.get_center(center);

    emitted += xParticleBatchRequest(batch_id, asset->rate * dt, 0);
    S32 emit = emitted;

#if DEBUG || RELEASE
    debugInfo.attemptedEmit = emit;
#endif

    if (emit > 0) {
        emitted -= emit;

        F32 lifetime = a.lifetime, rotSpeed = a.rotSpeed, gravity = -a.gravity;

        xVec3 upVel = { 0.0f, a.initUpVel, 0.0f };
        xMat3x3RMulVec(&upVel, &volumeFrame, &upVel);

        for (; emit > 0;) {
            U8* mem;
            S32 have = xParticleBatchEmit(batch_id, emit, &mem);
            if (!have)
                break;

            zParticleSystemBurrowKickup::particle_instance* p = (zParticleSystemBurrowKickup::particle_instance*)mem, *endp = p + have;

            locator.make_locations(p, sizeof(zParticleSystemBurrowKickup::particle_instance), have, 0);

            for (; p != endp; p++) {
                xVec3& vel = p->vel;
                vel.Sub(p->loc, center);
                vel.safe_normalize(xVec3::m_UnitAxisY);
                vel.Scale(a.randOutVel * xrand_GenRandFloat());
                vel.AddScale(upVel, xrand_GenRandFloat());

                p->age = lifetime;

                p->rotSpeed = rotSpeed * xrand_RandomBipolarUnitFloatWithGap(0.5f);
                p->rotation = 0.0f;

                p->flags = xrand_RandomInt32() & 0xF;
            }

            emit -= have;
        }

#if DEBUG || RELEASE
        debugInfo.failedEmit = emit;
#endif
    }

    shrapEmitted += a.shrap.shrapEmitRate * dt;
    emit = shrapEmitted;
    if (shrapAsset && emit > 0) {
        shrapEmitted -= emit;
        xVec3* posList = (xVec3*)xMEMPUSHTEMP_L(emit * sizeof(xVec3), 0, 186);
        locator.make_locations(posList, sizeof(xVec3), emit, 0);

        xVec3 upVel = { 0.0f, a.shrap.initUpVel, 0.0f };
        xMat3x3RMulVec(&upVel, &volumeFrame, &upVel);

        xVec3* posPtr = posList;
        for (; emit > 0; emit--) {
            xVec3 initVel;
            initVel.Sub(*posPtr, center);
            initVel.safe_normalize(xVec3::m_UnitAxisY);
            initVel.Scale(a.shrap.initOutVel);
            initVel.Add(upVel);

            xVec3Copy((xVec3*)&tempModel->Mat->pos, posPtr);
            shrapAsset->initCB(shrapAsset, tempModel, NULL, &initVel, NULL);
            posPtr++;
        }
        xMEMPOPTEMP(posList);
    }
}

void zParticleGeneratorBurrowKickup::create(zParticleSystemBurrowKickup& system, zParticleAsset& asset)
{
    zParticleGenerator::create(system, asset);
    this->system = &system;
    locator.create();

    attach_to = get_attach_entity(asset);

    xParticleBatchSystem batch;
    batch.pool_type = PGT_POS_COLOR_SIZE_ROT_UV2;
    batch.rs.texture = !asset.texture ? NULL : (RwTexture*)xSTFindAsset(asset.texture, NULL);
    batch.rs.src_blend = rwBLENDSRCALPHA;
    batch.rs.dst_blend = rwBLENDINVSRCALPHA;
    batch.rs.flags = ptank_pool::FLAG_ORDERED;
    batch.order_group = 'HOBO';
    batch.order_index = 1;
    batch.stride = sizeof(zParticleSystemBurrowKickup::particle_instance);
    batch.update = zParticleSystemBurrowKickup::update_particles;
    batch.context = this;

    batch_id = xPARTICLEBATCHGROUP_CREATE_SYSTEM_NAMED(system.batch_group, batch, "BurrowKickup");

    const zParticleSystemBurrowKickup::asset_data& a = *(zParticleSystemBurrowKickup::asset_data*)(&asset + 1);

    if (a.shrap.shrapID) {
        shrapAsset = (zShrapnelAsset*)xSTFindAsset(a.shrap.shrapID, NULL);
        xASSERT_FMT_L(237, shrapAsset, "Could not find shrapnel object for BurrowKickup effect 0x%X", a.shrap.shrapID);

        RpAtomic* m = (RpAtomic*)xSTFindAsset(HASH_light_card_b_tl, NULL);
        xASSERT_L(240, m);
        tempModel = xModelInstanceAlloc(m, NULL, 0, 0, NULL);
        xModelInstanceSetID_L(242, tempModel, HASH_light_card_b_tl);
    }

    reset();
}

void zParticleGeneratorBurrowKickup::reset()
{
    zParticleGenerator::reset();
    emitted = 0.0f;
    shrapEmitted = 0.0f;
    locator.restart();
    locator.deactivate();
    if (flags & 0x1) {
        locator.activate(*asset, attach_to);
        resume();
    }
}

void zParticleGeneratorBurrowKickup::activate(xEnt*)
{
    zParticleGenerator::activate(NULL);
    locator.activate(*asset, attach_to);
}

void zParticleGeneratorBurrowKickup::deactivate(xEnt*)
{
    locator.deactivate();
    zParticleGenerator::deactivate(NULL);
}

bool zParticleGeneratorBurrowKickup::cullUpdate()
{
    xVec3 center;
    locator.get_center(center);
    if (xVec3DistFast(&center, &xglobals->cam->mat.pos) > asset->emitDistance)
        return true;

    return false;
}

#if DEBUG || RELEASE
void zParticleGeneratorBurrowKickup::add_tweaks(const char* prefix)
{
    zParticleGenerator::add_tweaks(prefix);
    system->batch_group.add_tweaks(&batch_id, prefix);
    locator.add_tweaks(prefix, *asset, attach_to);

    char buffer[128];
    sprintf(buffer, "%s\001system|", prefix);
    prefix = buffer;

    zParticleSystemBurrowKickup::asset_data& a = *(zParticleSystemBurrowKickup::asset_data*)(asset + 1);

    xAUTOTWEAK(prefix, "a. gravity", &a.gravity, -1e9f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. size", &a.size, 0.001f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. rotSpeed", &a.rotSpeed, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. lifetime", &a.lifetime, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. maxAlpha", &a.maxAlpha, 0.0f, 255.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. initUpVel", &a.initUpVel, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. randOutVel", &a.randOutVel, 0.0f, 1e9f, NULL, NULL, 0, true);

    xAUTOTWEAK(prefix, "a. shrap.shrapEmitRate", &a.shrap.shrapEmitRate, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. shrap.initUpVel", &a.shrap.initUpVel, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. shrap.initOutVel", &a.shrap.initOutVel, 0.0f, 1e9f, NULL, NULL, 0, true);

    xAUTOTWEAK(prefix, "debugInfo.attemptedEmit", &debugInfo.attemptedEmit, 0.0f, 1e9f, NULL, NULL, 1, true);
    xAUTOTWEAK(prefix, "debugInfo.failedEmit", &debugInfo.failedEmit, 0.0f, 1e9f, NULL, NULL, 1, true);
}

void zParticleGeneratorBurrowKickup::debug_render(bool all)
{
    zParticleGenerator::debug_render(all);
    if (all && locator.active())
        locator.debug_render();
}
#endif
