#include "xCommon.h"
#include "zParticleSystemRain.h"

#include "containers.h"
#include "xCam.h"
#include "xColor.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xMath3.h"
#include "xPtankPool.h"
#include "xRand.h"
#include "xSpacePartitionTree.h"
#include "xstransvc.h"
#include "zParticleLocator.h"

#include <stdio.h>

struct zParticleGeneratorRain : zParticleGenerator
{
    void update(F32 dt);
    void create(zParticleSystemRain& system, zParticleAsset& asset);
    virtual void reset();
    virtual void activate(xEnt*);
    virtual void deactivate(xEnt*);
    virtual void resume();
    virtual void pause();
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

    zParticleSystemRain* system;
    xEnt* attach_to;
    F32 emitted;
    zParticleLocator locator;
    S32 batch_id;
    bool firstEmit;
    bool killParticles;
#if DEBUG || RELEASE
    debug debugInfo;
#endif

    friend class zParticleSystemRain;
};

namespace {

xTWEAKABLE F32 FADE_TIME = 0.2f;
xTWEAKABLE F32 MAX_RENDER_DISTANCE = 60.0f;

}

zParticleGenerator* zParticleSystemRain::create_generator(zParticleAsset& asset)
{
    zParticleGeneratorRain* gen = new (eMemStaticType_0, eMemMgrTag_Particle) zParticleGeneratorRain;
    gen->create(*this, asset);
    return gen;
}

void zParticleSystemRain::scene_enter()
{
    batch_group.create();
}

void zParticleSystemRain::update(F32 dt)
{
    update_generators<zParticleGeneratorRain>(dt);
}

S32 zParticleSystemRain::get_asset_size() const
{
    return sizeof(asset_data);
}

S32 zParticleSystemRain::update_particles(U8* mem, S32 count, ptank_pool& apool, F32 dt, void* context)
{
    FPTANK_BEGIN(color_mat, apool);
    particle_instance* p = (particle_instance*)mem, *end = p + count;

    xASSERT_L(48, context);
    zParticleGeneratorRain& g = *(zParticleGeneratorRain*)context;

    if (g.killParticles) {
        return 0;
    }

    asset_data& a = *(asset_data*)(g.asset + 1);
    xASSERT_FMT_L(56, a.gravity > 0.0f, "Rain gravity must be positive!!");
    F32 gravity = a.gravity;
    F32 height = g.locator.get_height();
    F32 fadeDist = gravity * FADE_TIME;
    F32 dropLength = a.dropLength;
    F32 dropWidth = a.dropWidth;

    const xMat4x3& camMat = xglobals->cam->mat;
    const xVec3& camPos = camMat.pos;

    while (p != end) {
        F32 distTraveled = p->yOrig - p->loc.y;
        if (distTraveled > height) {
            end--;
            *p = *end;
            continue;
        }

        xIF_SPHERE_VISIBLE(p->loc.m_RwV3d, dropLength) {
            if (!(xVec3DistFast(&p->loc, &camPos) > MAX_RENDER_DISTANCE)) {
                FPTANK_NEXT(color_mat);

                xVec3& _up_ = pool_mat->up;
                xVec3& _at_ = pool_mat->at;
                xVec3& _right_ = pool_mat->right;

                _up_.assign(0.0f, 1.0f, 0.0f);
                _at_.Sub(camMat.pos, p->loc);

                xVec3Cross(&_right_, &_up_, &_at_);
                xVec3Cross(&_at_, &_right_, &_up_);

                xVec3Normalize(&_at_, &_at_);
                xVec3Normalize(&_right_, &_right_);

                _right_.Scale(dropWidth);
                _up_.Scale(dropLength);

                pool_mat->pos = p->loc;

                F32 alpha = 1.0f;
                if (distTraveled < fadeDist)
                    alpha = distTraveled / fadeDist;
                else if (distTraveled > height - fadeDist)
                    alpha = (height - distTraveled) / fadeDist;

                *pool_color = g_WHITE;
                pool_color->a = 255.0f * alpha;
            }
        }

        p->loc.y -= gravity * dt;
        p++;
    }

    FPTANK_END(color_mat);
    return p - (particle_instance*)mem;
}

void zParticleGeneratorRain::update(F32 dt)
{
    if (!locator.active())
        return;
    locator.update(dt);

    const zParticleSystemRain::asset_data& a = *(zParticleSystemRain::asset_data*)(asset + 1);
    F32 height = locator.get_height();

    if (firstEmit) {
        emitted += xParticleBatchRequest(batch_id, height * (asset->rate * locator.get_area() * 0.01f) / a.gravity, 0);
    } else {
        emitted += xParticleBatchRequest(batch_id, dt * (asset->rate * locator.get_area() * 0.01f), 0);
    }

    S32 emit = emitted;
    if (emit <= 0)
        return;
    emitted -= emit;

#if DEBUG || RELEASE
    debugInfo.attemptedEmit = emit;
#endif

    for (; emit > 0;) {
        U8* mem;
        S32 have = xParticleBatchEmit(batch_id, emit, &mem);
        if (!have)
            break;
        zParticleSystemRain::particle_instance* p = (zParticleSystemRain::particle_instance*)mem, *endp = p + have;

        locator.make_locations(p, sizeof(zParticleSystemRain::particle_instance), have, 0);

        for (; p != endp; p++) {
            p->yOrig = p->loc.y;

            if (firstEmit)
                p->loc.y -= height * xrand_GenRandFloat();
        }

        emit -= have;
    }

    if (firstEmit) {
        firstEmit = false;
    }

#if DEBUG || RELEASE
    debugInfo.failedEmit = emit;
#endif
}

void zParticleGeneratorRain::create(zParticleSystemRain& system, zParticleAsset& asset)
{
    zParticleGenerator::create(system, asset);
    this->system = &system;
    locator.create();

    attach_to = get_attach_entity(asset);

    xParticleBatchSystem batch;
    batch.pool_type = PGT_COLOR_MAT;
    batch.rs.texture = !asset.texture ? NULL : (RwTexture*)xSTFindAsset(asset.texture, NULL);
    batch.rs.src_blend = rwBLENDSRCALPHA;
    batch.rs.dst_blend = rwBLENDINVSRCALPHA;
    batch.rs.flags = ptank_pool::FLAG_ORDERED;
    batch.order_group = 'HOBO';
    batch.order_index = 1;
    batch.stride = sizeof(zParticleSystemRain::particle_instance);
    batch.update = zParticleSystemRain::update_particles;
    batch.context = this;

    batch_id = xPARTICLEBATCHGROUP_CREATE_SYSTEM_NAMED(system.batch_group, batch, "Rain");

    reset();
}

void zParticleGeneratorRain::reset()
{
    if (flags & 0x2) {
        pause();
    }

    zParticleGenerator::reset();
    emitted = 0.0f;
    locator.restart();
    locator.deactivate();
    if (flags & 0x1) {
        locator.activate(*asset, attach_to);
        resume();
    }
}

void zParticleGeneratorRain::activate(xEnt*)
{
    zParticleGenerator::activate(NULL);
    locator.activate(*asset, attach_to);

    xASSERT_FMT_L(230, asset->volume_type == zParticleAsset::VOLUME_BOX || asset->volume_type == zParticleAsset::VOLUME_CIRCLE, "Must use a Box or Circle volume with Rain.");
    resume();
}

void zParticleGeneratorRain::deactivate(xEnt*)
{
    pause();
    locator.deactivate();
    zParticleGenerator::deactivate(NULL);
}

void zParticleGeneratorRain::resume()
{
    zParticleGenerator::resume();

    locator.setEmitFromEdge(true);
    firstEmit = true;
    killParticles = false;
}

void zParticleGeneratorRain::pause()
{
    zParticleGenerator::pause();
    killParticles = true;
}

bool zParticleGeneratorRain::cullUpdate()
{
    xSphere sphere;
    locator.get_bounding_sphere(sphere);

    if (xVec3DistFast(&sphere.center, &xglobals->cam->mat.pos) > MAX_RENDER_DISTANCE + sphere.r) {
    } else {
        return false;
    }

    return true;
}

#if DEBUG || RELEASE
void zParticleGeneratorRain::add_tweaks(const char* prefix)
{
    zParticleGenerator::add_tweaks(prefix);
    system->batch_group.add_tweaks(&batch_id, prefix);
    locator.add_tweaks(prefix, *asset, attach_to);

    char buffer[128];
    sprintf(buffer, "%s\001system|", prefix);
    prefix = buffer;

    zParticleSystemRain::asset_data& a = *(zParticleSystemRain::asset_data*)(asset + 1);

    xAUTOTWEAK(prefix, "a. gravity", &a.gravity, 0.001f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. dropWidth", &a.dropWidth, 0.001f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "a. dropLength", &a.dropLength, 0.001f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "FADE_TIME", &FADE_TIME, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "MAX_RENDER_DISTANCE", &MAX_RENDER_DISTANCE, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "debugInfo.attemptedEmit", &debugInfo.attemptedEmit, 0.0f, 1e9f, NULL, NULL, 1, true);
    xAUTOTWEAK(prefix, "debugInfo.failedEmit", &debugInfo.failedEmit, 0.0f, 1e9f, NULL, NULL, 1, true);
}

void zParticleGeneratorRain::debug_render(bool all)
{
    zParticleGenerator::debug_render(all);
    if (all && locator.active()) {
        locator.debug_render();
    }
}
#endif
