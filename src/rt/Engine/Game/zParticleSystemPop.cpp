#include "xCommon.h"
#include "zParticleSystemPop.h"

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

struct zParticleGeneratorPop : zParticleGenerator
{
    static const S32 MAX_LOCATORS = 40;

    void update(F32 dt);
    void create(zParticleSystemPop& system, zParticleAsset& asset);
    virtual void reset();
    zParticleLocator* findLocator(xEnt* ent); // UNUSED
    virtual void activate(xEnt* ent);
    void activateEnt(xEnt* ent);
    virtual void deactivate(xEnt* ent);
    void deactivateEnt(xEnt* ent);
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

    zParticleSystemPop* system;
    xEnt* attach_to;
    F32 emitted;
    zParticleLocator* locators[MAX_LOCATORS];
    S32 numLocators;
    S32 batch_id;
#if DEBUG || RELEASE
    debug debugInfo;
#endif

    friend class zParticleSystemPop;
};

enum
{
    MAX_LOCATORS_ACTIVE = 128
};

static zParticleLocator* sLocatorPool;
static S32 sLocatorsUsed;

zParticleGenerator* zParticleSystemPop::create_generator(zParticleAsset& asset)
{
    zParticleGeneratorPop* gen = new (eMemStaticType_0, eMemMgrTag_Particle) zParticleGeneratorPop;
    gen->create(*this, asset);
    return gen;
}

void zParticleSystemPop::scene_enter()
{
    batch_group.create();
    sLocatorPool = new (eMemStaticType_0, eMemMgrTag_Particle) zParticleLocator[MAX_LOCATORS_ACTIVE];
    sLocatorsUsed = 0;
}

void zParticleSystemPop::scene_exit()
{
    sLocatorPool = NULL;
    sLocatorsUsed = 0;
}

void zParticleSystemPop::update(F32 dt)
{
    update_generators<zParticleGeneratorPop>(dt);
}

S32 zParticleSystemPop::get_asset_size() const
{
    return sizeof(asset_data);
}

S32 zParticleSystemPop::update_particles(U8* mem, S32 count, ptank_pool& apool, F32 dt, void* context)
{
    FPTANK_BEGIN(pos_color_size_rot_uv2, apool);
    particle_instance* p = (particle_instance*)mem, *end = p + count;

    xASSERT_L(57, context);
    zParticleGeneratorPop& g = *(zParticleGeneratorPop*)context;

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

void zParticleGeneratorPop::update(F32 dt)
{
    if (!numLocators) {
        return;
    }

    for (S32 i = 0; i < numLocators; i++) {
        if (i == 0 && !attach_to) {
            continue;
        }

        zParticleLocator* locator = locators[i];
        locator->update(dt);

        xVec3 center;
        locator->get_center(center);
        if (xVec3DistFast(&center, &xglobals->cam->mat.pos) > asset->emitDistance) {
            continue;
        }

        const zParticleSystemPop::asset_data& a = *(zParticleSystemPop::asset_data*)(asset + 1);
        emitted += xParticleBatchRequest(batch_id, asset->rate * dt, 0);
        S32 emit = emitted;

#if DEBUG || RELEASE
        debugInfo.attemptedEmit = emit;
#endif

        if (emit <= 0)
            continue;

        emitted -= emit;

        xASSERT_FMT_L(149, a.gravity > 0.0f, "Pop gravity must be positive!!");

        F32 lifetime = a.lifetime, drift = a.drift, rotSpeed = a.rotSpeed, gravity = -a.gravity;

        for (; emit > 0;) {
            U8* mem;
            S32 have = xParticleBatchEmit(batch_id, emit, &mem);
            if (!have)
                break;

            zParticleSystemPop::particle_instance* p = (zParticleSystemPop::particle_instance*)mem, *endp = p + have;

            locator->make_locations(p, sizeof(zParticleSystemPop::particle_instance), have, 0);

            for (; p != endp; p++) {
                xVec3& vel = p->vel;
                vel.Sub(p->loc, center);
                vel.safe_normalize(xVec3::m_UnitAxisY);
                vel.Scale(a.outVel);

                vel.x += drift * xrand_RandomBipolarUnitFloat();
                vel.z += drift * xrand_RandomBipolarUnitFloat();
                vel.y += gravity;

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

void zParticleGeneratorPop::create(zParticleSystemPop& system, zParticleAsset& asset)
{
    zParticleGenerator::create(system, asset);
    this->system = &system;
    for (S32 i = 1; i < MAX_LOCATORS; i++)
        locators[i] = NULL;

    locators[0] = allocLocator();
    locators[0]->create();
    numLocators = 1;

    attach_to = get_attach_entity(asset);

    xParticleBatchSystem batch;
    batch.pool_type = PGT_POS_COLOR_SIZE_ROT_UV2;
    batch.rs.texture = !asset.texture ? NULL : (RwTexture*)xSTFindAsset(asset.texture, NULL);
    batch.rs.src_blend = rwBLENDSRCALPHA;
    batch.rs.dst_blend = rwBLENDINVSRCALPHA;
    batch.rs.flags = ptank_pool::FLAG_ORDERED;
    batch.order_group = 'HOBO';
    batch.order_index = 1;
    batch.stride = sizeof(zParticleSystemPop::particle_instance);
    batch.update = zParticleSystemPop::update_particles;
    batch.context = this;

    batch_id = xPARTICLEBATCHGROUP_CREATE_SYSTEM_NAMED(system.batch_group, batch, "Pop");

    const zParticleSystemPop::asset_data& a = *(zParticleSystemPop::asset_data*)(&asset + 1);

    reset();
}

void zParticleGeneratorPop::reset()
{
    emitted = 0.0f;

    locators[0]->restart();

    deactivate(NULL);
    zParticleGenerator::reset();
    if (flags & 0x1) {
        locators[0]->activate(*asset, attach_to);
        resume();
    }
}

zParticleLocator* allocLocator()
{
    xASSERT_L(249, sLocatorsUsed < MAX_LOCATORS_ACTIVE);
    xASSERT_L(250, sLocatorsUsed >= 0);
    zParticleLocator* locator = sLocatorPool + sLocatorsUsed;

    sLocatorsUsed++;
    return locator;
}

void freeLocator(zParticleLocator* locator)
{
    xASSERT_L(259, (locator >= sLocatorPool) && (locator < (sLocatorPool + sLocatorsUsed)));
    sLocatorsUsed--;
    zParticleLocator* tailLocator = sLocatorPool + sLocatorsUsed;
    if (locator != tailLocator) {
        // BUG: pointers to the moved tail go stale
        *locator = *tailLocator;
    }
}

void zParticleGeneratorPop::activate(xEnt* ent)
{
    zParticleGenerator::activate(NULL);
    if (ent) {
        activateEnt(ent);
    }
    else if (locators[0]) {
        if (!locators[0]->activate(*asset, attach_to))
            deactivate(ent);
    } else {
        xFAIL_MSG_L(292, "Trying to activate a Popper effect without an attached ent.");
    }
}

void zParticleGeneratorPop::activateEnt(xEnt* ent)
{
    zParticleLocator* locator = allocLocator();
    locators[numLocators++] = locator;
    locator->create();

    if (!locator->activate(*asset, ent))
        deactivateEnt(ent);
}

void zParticleGeneratorPop::deactivate(xEnt* ent)
{
    if (ent) {
        deactivateEnt(ent);
    } else {
        locators[0]->deactivate();
        for (S32 i = 1; i < numLocators; i++) {
            zParticleLocator* locator = locators[i];
            locator->deactivate();
            freeLocator(locator);
            locators[i] = NULL;
        }
        numLocators = 1;
        zParticleGenerator::deactivate(NULL);
    }
}

void zParticleGeneratorPop::deactivateEnt(xEnt* ent)
{
    S32 i;
    for (i = 1; i < numLocators; i++) {
        if (locators[i]->ent == ent)
            break;
    }

    if (i == numLocators) {
        return;
    }

    zParticleLocator* locator = locators[i];
    locator->deactivate();
    freeLocator(locator);
    if (i != numLocators)
        locators[i] = locators[numLocators - 1];
    locators[numLocators - 1] = NULL;
    numLocators--;

    zParticleLocator* tailLocator = sLocatorPool + sLocatorsUsed;
    if (locator != tailLocator) {
        *locator = *tailLocator;
    }
}

#if DEBUG || RELEASE
void zParticleGeneratorPop::add_tweaks(const char* prefix)
{
    zParticleGenerator::add_tweaks(prefix);
    system->batch_group.add_tweaks(&batch_id, prefix);
    if (locators[0]) {
        locators[0]->add_tweaks(prefix, *asset, attach_to);
    }

    char buffer[128];
    sprintf(buffer, "%s\001system|", prefix);
    prefix = buffer;

    zParticleSystemPop::asset_data& a = *(zParticleSystemPop::asset_data*)(asset + 1);

    xAUTOTWEAK_MEMBER(prefix, a., gravity, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK_MEMBER(prefix, a., drift, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK_MEMBER(prefix, a., outVel, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK_MEMBER(prefix, a., size, 0.001f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK_MEMBER(prefix, a., rotSpeed, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK_MEMBER(prefix, a., lifetime, 0.0f, 1e9f, NULL, NULL, 0, true);
    xAUTOTWEAK_MEMBER(prefix, a., maxAlpha, 0.0f, 255.0f, NULL, NULL, 0, true);
    xAUTOTWEAK_MEMBER(prefix, a., maxAlpha, 0.0f, 255.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "debugInfo.attemptedEmit", &debugInfo.attemptedEmit, 0.0f, 1e9f, NULL, NULL, 1, true);
    xAUTOTWEAK(prefix, "debugInfo.failedEmit", &debugInfo.failedEmit, 0.0f, 1e9f, NULL, NULL, 1, true);
}

void zParticleGeneratorPop::debug_render(bool all)
{
    zParticleGenerator::debug_render(all);
    for (S32 i = 0; i < numLocators; i++) {
        zParticleLocator* locator = locators[i];
        if (all && locator->active()) {
            locator->debug_render();
        }
    }
}
#endif
