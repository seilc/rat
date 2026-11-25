#include "xCommon.h"
#include "zParSysSnowsplosion.h"

#include "iMath.h"
#include "xCam.h"
#include "xColor.h"
#include "xDebugTweak.h"
#include "xGlobals.h"
#include "xPTankPool.h"
#include "xParticleBatch.h"
#include "xRand.h"
#include "xSpacePartitionTree.h"
#include "xstransvc.h"
#include "xString.h"

namespace {

S32 systemID = -1;

struct Particle
{
    xVec3 pos;
    F32 size;
    xVec3 dir;
    F32 lifeTime;
    F32 speed;
    const zParSysSnowsplosionConfig* config;
    F32 pad[2];

    void Update(F32 dt)
    {
        lifeTime -= dt;

        speed -= config->deceleration * dt;
        if (speed > 0.0f) {
            pos.AddScale(dir, speed * dt);
        }

        pos.AddScale(xVec3::m_UnitAxisY, -config->speedDown * dt);
    }

    void Setup(const xVec3& position, const zParSysSnowsplosionConfig& cfg)
    {
        pos = position;
        config = &cfg;

        F32 theta = xrand_RandomFloatRange(0.0f, 2.0f * PI), phi;
        if (config->spawnDown) {
            phi = xrand_RandomFloatRange(0.0f, 2.0f * PI);
        } else {
            phi = xrand_RandomFloatRange(-PI / 2, PI / 2);
        }

        dir.x = isin(phi) * icos(theta);
        dir.y = icos(phi);
        dir.z = isin(phi) * isin(theta);
        xASSERT_FMT_L(73, config->spawnDown || dir.y >= 0.0f, "The code that prevents downward spawning doesn't work...");

        speed = xrand_RandomFloatRange(config->minSpeed, config->maxSpeed);
        size = xrand_RandomFloatRange(config->minSize, config->maxSize);
        lifeTime = xrand_RandomFloatRange(config->minLife, config->maxLife);
    }
};

S32 ParticleUpdate(U8* mem, S32 count, ptank_pool& apool, F32 dt, void*)
{
    ptank_pool__pos_color_size& pool = (ptank_pool__pos_color_size&)apool;
    Particle* p = (Particle*)mem;
    Particle* end = p + count;

    while (p != end) {
        p->Update(dt);

        if (p->lifeTime < 0.0f) {
            *p = *--end;
            continue;
        }

        xIF_SPHERE_VISIBLE(p->pos.m_RwV3d, p->size) {
            pool.next();
            if (!pool.valid()) {
                end = p;
                break;
            }

            *pool.pos = p->pos;
            pool.size->assign(p->size, p->size);
            *pool.color = g_WHITE;
            pool.color->a = p->config->startAlpha;

            if (p->lifeTime < p->config->fadeStartTime) {
                pool.color->a = p->config->startAlpha * p->lifeTime / p->config->fadeStartTime;
            }
        }

        p++;
    }

    return end - (Particle*)mem;
}

}

#if DEBUG || RELEASE
void zParSysSnowsplosionConfig::AddTweaks(const char* prefix)
{
    xAUTOTWEAK(prefix, "Life|Min", &minLife, 0.0f, 1000.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "Life|Max", &maxLife, 0.0f, 1000.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "Size|Min", &minSize, 0.0f, 1000.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "Size|Max", &maxSize, 0.0f, 1000.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "Speed|Min", &minSpeed, 0.0f, 1000.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "Speed|Max", &maxSpeed, 0.0f, 1000.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "Particles|Min", &minParticles, 0, 255, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "Particles|Max", &maxParticles, 0, 255, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "deceleration", &deceleration, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fadeStartTime", &fadeStartTime, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "speedDown", &speedDown, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "startAlpha", &startAlpha, 0, 255, NULL, NULL, 0, true);
}
#endif

void zParSysSnowsplosionSceneSetup()
{
    RwTexture* texture = (RwTexture*)xSTFindAsset(xStrHash("fx_nuke_fro_flake"), NULL);

    xParticleBatchSystem batchSystem;

    batchSystem.context = NULL;
    batchSystem.update = ParticleUpdate;
    batchSystem.pool_type = PGT_POS_COLOR_SIZE;
    batchSystem.order_group = 'HOBO';
    batchSystem.order_index = 7;
    batchSystem.rs.flags = ptank_pool::FLAG_ORDERED;
    batchSystem.rs.src_blend = rwBLENDSRCALPHA;
    batchSystem.rs.dst_blend = rwBLENDINVSRCALPHA;
    batchSystem.rs.texture = texture;
    batchSystem.stride = sizeof(Particle);
    systemID = xPARTICLEBATCHADDSYSTEM_NAMED(batchSystem, "zParSysSnowsplosion");
}

void zParSysSnowsplosionEmit(const xVec3& pos, const zParSysSnowsplosionConfig& config)
{
    S32 emitCount = xrand_RandomRange(config.minParticles, config.maxParticles);
    emitCount = xParticleBatchRequest(systemID, emitCount, 0);

    while (emitCount > 0) {
        Particle* p;
        S32 count = xParticleBatchEmit(systemID, emitCount, (U8**)&p);
        if (count == 0) {
            break;
        }

        Particle* end = p + count;
        while (p < end) {
            p->Setup(pos, config);
            p++;
        }

        emitCount -= count;
    }
}
