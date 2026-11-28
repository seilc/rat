#include "xCommon.h"
#include "zParticleSystemPuffer.h"

#include "containers.h"
#include "xCam.h"
#include "xColor.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xMath.h"
#include "xModel.h"
#include "xPtankPool.h"
#include "xRand.h"
#include "xSpacePartitionTree.h"
#include "xstransvc.h"
#include "zEvent.h"
#include "zHashes.h"
#include "zParticleLocator.h"
#include "zShrapnelAsset.h"

#include <stdio.h>

struct zParticleGeneratorPuffer : zParticleGenerator
{
    void update(F32 dt);
    void create(zParticleSystemPuffer& system, zParticleAsset& asset);
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

    zParticleSystemPuffer* system;
    xEnt* attach_to;
    F32 emitted;
    zParticleLocator locator;
    S32 batch_id;
    F32 timer;
    F32 nextTimeEvent;
    bool isParticlesOn;
    xVec3 curLoc;
    F32 shrapEmitted;
    zShrapnelAsset* shrapAsset;
    xModelInstance* tempModel;
#if DEBUG || RELEASE
    debug debugInfo;
#endif

    friend class zParticleSystemPuffer;
};

zParticleGenerator* zParticleSystemPuffer::create_generator(zParticleAsset& asset)
{
    zParticleGeneratorPuffer* gen = new (eMemStaticType_0, eMemMgrTag_Particle) zParticleGeneratorPuffer;
    gen->create(*this, asset);
    return gen;
}

void zParticleSystemPuffer::scene_enter()
{
    batch_group.create();
}

void zParticleSystemPuffer::update(F32 dt)
{
    update_generators<zParticleGeneratorPuffer>(dt);
}

S32 zParticleSystemPuffer::get_asset_size() const
{
    return sizeof(asset_data);
}

S32 zParticleSystemPuffer::update_particles(U8* mem, S32 count, ptank_pool& apool, F32 dt, void* context)
{
    FPTANK_BEGIN(pos_color_size_rot_uv2, apool);
    particle_instance* p = (particle_instance*)mem, *end = p + count;

    xASSERT_L(46, context);
    zParticleGeneratorPuffer& g = *(zParticleGeneratorPuffer*)context;

    asset_data& a = *(asset_data*)(g.asset + 1);
    F32 size = a.size;
    F32 lifetime = a.lifetime;
    F32 birthAlpha = a.birthAlpha;
    F32 deathAlpha = a.deathAlpha;
    F32 alphaRange = birthAlpha - deathAlpha;

    while (p != end) {
        p->age -= dt;
        if (p->age <= 0.0f) {
            end--;
            *p = *end;
            continue;
        }

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

            F32 alpha = MAX(0.0f, MIN(255.0f, alphaRange * (p->age / lifetime) + deathAlpha));

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

void zParticleGeneratorPuffer::update(F32 dt)
{
    if (!locator.active())
        return;

    locator.update(dt);

    const zParticleSystemPuffer::asset_data& a = *(zParticleSystemPuffer::asset_data*)(asset + 1);
    bool startEmit = false;

    timer += dt;
    if (timer >= nextTimeEvent) {
        timer = 0.0f;
        isParticlesOn = !isParticlesOn;
        if (isParticlesOn) {
            nextTimeEvent = a.minTimeOn + (a.maxTimeOn - a.minTimeOn) * xrand_GenRandFloat();
            startEmit = true;
            locator.make_locations(&curLoc, sizeof(xVec3), 1, 0);
        } else
            nextTimeEvent = a.minTimeOff + (a.maxTimeOff - a.minTimeOff) * xrand_GenRandFloat();
    }

    if (isParticlesOn) {
        emitted += xParticleBatchRequest(batch_id, asset->rate * dt, 0);
        S32 emit = emitted;

#if DEBUG || RELEASE
        debugInfo.attemptedEmit = emit;
#endif

        if (emit > 0) {
            emitted -= emit;

            F32 lifetime = a.lifetime, drift = a.drift, rotSpeed = a.rotSpeed, gravity = -a.gravity;

            for (; emit > 0;) {
                U8* mem;
                S32 have = xParticleBatchEmit(batch_id, emit, &mem);
                if (!have)
                    break;

                zParticleSystemPuffer::particle_instance* p = (zParticleSystemPuffer::particle_instance*)mem, *endp = p + have;

                for (; p != endp; p++) {
                    p->loc = curLoc;
                    p->vel.x = drift * xrand_RandomBipolarUnitFloat();
                    p->vel.z = drift * xrand_RandomBipolarUnitFloat();
                    p->vel.y = gravity;
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
    }

    if (startEmit) {
        if (shrapAsset && a.shrap.numShrap > 0) {
            S32 emit = a.shrap.numShrap;
            for (; emit > 0; emit--) {
                F32 initX = a.shrap.randVel * xrand_RandomBipolarUnitFloat();
                F32 initZ = a.shrap.randVel * xrand_RandomBipolarUnitFloat();
                xVec3 initVel = { initX, -a.shrap.initDownVel, initZ };

                xVec3Copy((xVec3*)&tempModel->Mat->pos, &curLoc);
                shrapAsset->initCB(shrapAsset, tempModel, NULL, &initVel, NULL);
            }
        }

        zEntEvent(this, eEventEmitted);
    }
}

void zParticleGeneratorPuffer::create(zParticleSystemPuffer& system, zParticleAsset& asset)
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
    batch.stride = sizeof(zParticleSystemPuffer::particle_instance);
    batch.update = zParticleSystemPuffer::update_particles;
    batch.context = this;

    batch_id = xPARTICLEBATCHGROUP_CREATE_SYSTEM_NAMED(system.batch_group, batch, "Puffer");

    const zParticleSystemPuffer::asset_data& a = *(zParticleSystemPuffer::asset_data*)(&asset + 1);

    if (a.shrap.shrapID) {
        shrapAsset = (zShrapnelAsset*)xSTFindAsset(a.shrap.shrapID, NULL);
        xASSERT_FMT_L(237, shrapAsset, "Could not find shrapnel object for Puffer effect 0x%X", a.shrap.shrapID);

        RpAtomic* m = (RpAtomic*)xSTFindAsset(HASH_light_card_b_tl, NULL);
        xASSERT_L(240, m);
        tempModel = xModelInstanceAlloc(m, NULL, 0, 0, NULL);
        xModelInstanceSetID_L(242, tempModel, HASH_light_card_b_tl);
    }

    reset();
}

void zParticleGeneratorPuffer::reset()
{
    zParticleGenerator::reset();
    emitted = 0.0f;
    shrapEmitted = 0.0f;
    isParticlesOn = false;
    nextTimeEvent = 0.0f;
    timer = 0.0f;
    locator.restart();
    locator.deactivate();
    if (flags & 0x1) {
        locator.activate(*asset, attach_to);
        resume();
    }
}

void zParticleGeneratorPuffer::activate(xEnt*)
{
    isParticlesOn = false;
    nextTimeEvent = 0.0f;
    timer = 0.0f;
    zParticleGenerator::activate(NULL);
    locator.activate(*asset, attach_to);
}

void zParticleGeneratorPuffer::deactivate(xEnt*)
{
    isParticlesOn = false;
    locator.deactivate();
    zParticleGenerator::deactivate(NULL);
}

bool zParticleGeneratorPuffer::cullUpdate()
{
    xVec3 center;
    locator.get_center(center);
    if (xVec3DistFast(&center, &xglobals->cam->mat.pos) > asset->emitDistance)
        return true;

    return false;
}

#if DEBUG || RELEASE
void zParticleGeneratorPuffer::add_tweaks(const char* prefix)
{
    zParticleGenerator::add_tweaks(prefix);
    system->batch_group.add_tweaks(&batch_id, prefix);
    locator.add_tweaks(prefix, *asset, attach_to);

    char buffer[128];
    sprintf(buffer, "%s\001system|", prefix);
    prefix = buffer;

    zParticleSystemPuffer::asset_data& a = *(zParticleSystemPuffer::asset_data*)(asset + 1);

    xAUTOTWEAK(prefix, "a. minTimeOn", &a.minTimeOn, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. maxTimeOn", &a.maxTimeOn, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. minTimeOff", &a.minTimeOff, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. maxTimeOff", &a.maxTimeOff, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. gravity", &a.gravity, -1e9f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. drift", &a.drift, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. size", &a.size, 0.001f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. rotSpeed", &a.rotSpeed, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. lifetime", &a.lifetime, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. birthAlpha", &a.birthAlpha, 0.0f, 255.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. deathAlpha", &a.deathAlpha, 0.0f, 255.0f, NULL, NULL, 0, true);

    xAUTOTWEAK(prefix, "a. shrap.numShrap", &a.shrap.numShrap, 0, 1000, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. shrap.initDownVel", &a.shrap.initDownVel, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. shrap.randVel", &a.shrap.randVel, 0.0f, 1e9f, NULL, NULL, 0, true);

    xAUTOTWEAK(prefix, "debugInfo.attemptedEmit", &debugInfo.attemptedEmit, 0.0f, 1e9f, NULL, NULL, 1, true);
    xAUTOTWEAK(prefix, "debugInfo.failedEmit", &debugInfo.failedEmit, 0.0f, 1e9f, NULL, NULL, 1, true);
}

void zParticleGeneratorPuffer::debug_render(bool all)
{
    zParticleGenerator::debug_render(all);
    if (all && locator.active())
        locator.debug_render();
}
#endif
