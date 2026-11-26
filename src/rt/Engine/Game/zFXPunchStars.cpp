#include "xCommon.h"
#include "zFXPunchStars.h"

#include "containers.h"
#include "xCam.h"
#include "xDebugTweak.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xMath.h"
#include "xParticleBatch.h"
#include "xPtankPool.h"
#include "xRand.h"
#include "xstransvc.h"
#include "zGlobals.h"
#include "zHashes.h"
#include "zScene.h"

bool zFX::punch_stars::need_setup;
zFX::punch_stars::config zFX::punch_stars::default_config;

void zFX::punch_stars::setup()
{
    need_setup = true;

    default_config.min_life = 0.5f;
    default_config.max_life = 0.5f;
    default_config.min_size = 0.2f;
    default_config.max_size = 0.2f;
    default_config.emit_rate = 100.0f;
    default_config.emit_radius = 0.2f;
    default_config.gravity = 0.1f;
    default_config.slow = 500.0f;
    default_config.fade_start = 0.2f;
    default_config.gravity = 0.0f;
    default_config.r = 255;
    default_config.g = 255;
    default_config.b = 255;
    default_config.cinematic_only = 0;
    default_config.glow = 0.2f;
    default_config.min_rot = -5.0f;
    default_config.max_rot = 5.0f;
    default_config.size_delta = 2.0f;
    default_config.cone_rnd = 0.25f;
    default_config.cone_vel = 1.0f;
}

S32 zFX::punch_stars::system_id = -1;

void zFX::punch_stars::scene_setup()
{
    RwTexture* texture = (RwTexture*)xSTFindAsset(HASH_PunchStars, NULL);

    xParticleBatchSystem batch_system;
    batch_system.context = NULL;
    batch_system.update = update;
    batch_system.pool_type = PGT_POS_COLOR_SIZE_ROT_UV2;
    batch_system.rs.flags = 0;
    batch_system.rs.src_blend = rwBLENDSRCALPHA;
    batch_system.rs.dst_blend = rwBLENDINVSRCALPHA;
    batch_system.rs.texture = texture;
    batch_system.stride = sizeof(particle);

    system_id = xPARTICLEBATCHADDSYSTEM_NAMED(batch_system, "punch_stars");
}

void zFX::punch_stars::add_tweaks(const char* prefix, config* cfg)
{
    xAUTOTWEAK(prefix, "cfg->min_life", &cfg->min_life, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->max_life", &cfg->max_life, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->slow", &cfg->slow, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->max_size", &cfg->max_size, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->min_size", &cfg->min_size, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->r", &cfg->r, 0, 255, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->g", &cfg->g, 0, 255, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->b", &cfg->b, 0, 255, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->emit_rate", &cfg->emit_rate, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->emit_radius", &cfg->emit_radius, 0.0f, 10.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->glow", &cfg->glow, 0.0f, 1.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->fade_start", &cfg->fade_start, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->min_rot", &cfg->min_rot, -10000.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->gravity", &cfg->gravity, -10000.0f, 10000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->max_rot", &cfg->max_rot, -10000.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->size_delta", &cfg->size_delta, -10000.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->vel_right", &cfg->vel_right, -10.0f, 10.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->vel_up", &cfg->vel_up, -10.0f, 10.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->vel_at", &cfg->vel_at, -10.0f, 10.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->nrm_right", &cfg->nrm_right, -10.0f, 10.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->nrm_up", &cfg->nrm_up, -10.0f, 10.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->nrm_at", &cfg->nrm_at, -10.0f, 10.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->cone_rnd", &cfg->cone_rnd, -10.0f, 10.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "cfg->cone_vel", &cfg->cone_vel, -10.0f, 10.0f, NULL, NULL, 0, true);
}

void zFX::punch_stars::emit(const xVec3& position, const xVec3&, F32& emit_remaining, const xVec3& velocity, F32 dt,
                            config* cfg)
{
    if (need_setup) {
        need_setup = false;
        scene_setup();
    }

    F32 time_passed = (1.0f - emit_remaining) / cfg->emit_rate;
    emit_remaining += cfg->emit_rate * dt;

    S32 emit = emit_remaining;

    while (emit > 0) {
        particle* p;
        S32 count = xParticleBatchEmit(system_id, emit, (U8**)&p);
        if (count == 0) {
            emit_remaining = 0.0f;
            break;
        }

        particle* end = p + count;

        while (p != end) {
            if (p == end) {
                break;
            }

            p->life = xurand() * (cfg->max_life - cfg->min_life) + cfg->min_life - time_passed;
            p->size = xurand() * (cfg->max_size - cfg->min_size) + cfg->min_size;
            p->size += cfg->size_delta * time_passed;
            p->vel = velocity;
            p->rot = PI * (xurand() - 0.5f);
            p->rot_delta = xurand() * (cfg->max_rot - cfg->min_rot) + cfg->min_rot;
            p->texture_index = 4.0f * xurand();
            xVec3AddScaled(p->pos, position, velocity, time_passed);
            p->cfg = cfg;

            dt -= 1.0f / cfg->emit_rate;
            time_passed += 1.0f / cfg->emit_rate;
            p++;
        }

        emit_remaining -= count;
        emit -= count;
    }
}

S32 zFX::punch_stars::update(U8* mem, S32 count, ptank_pool& pool_, F32 dt, void*)
{
    ptank_pool__pos_color_size_rot_uv2& pool = (ptank_pool__pos_color_size_rot_uv2&)pool_;
    S32 cinematic_not_running = globals.cmgr == NULL ? TRUE : FALSE;

    particle* p = (particle*)mem;
    particle* end = p + count;

    while (p != end) {
        p->life -= dt;

        if (p->life < 0.0f || (cinematic_not_running & p->cfg->cinematic_only)) {
            *p = *--end;
            continue;
        }

        p->vel.y += p->cfg->gravity * dt;
        xVec3AddScaled(p->pos, p->vel, dt);

        p->rot += p->rot_delta * dt;
        p->rot = xDangleClamp(p->rot);

        p->size += p->cfg->size_delta * dt;
        if (p->size < 0.0f) {
            p->size = 0.0f;
        }

        xIF_SPHERE_VISIBLE(p->pos.m_RwV3d, p->size) {
            pool.next();
            if (!pool.valid()) {
                end = p;
                break;
            }

            *pool.pos = p->pos;

            pool.color->b = p->cfg->b;
            pool.color->g = p->cfg->g;
            pool.color->r = p->cfg->r;

            if (p->texture_index > 1) {
                pool.uv->y = 0.5f;
            } else {
                pool.uv->y = 0.0f;
            }

            if (p->texture_index % 2 == 1) {
                pool.uv->x = 0.5f;
            } else {
                pool.uv->x = 0.0f;
            }

            pool.uv[1].assign(pool.uv->x + 0.5f, pool.uv->y + 0.5f);

            pool.SetRot(p->rot);

            F32 alpha;
            if (p->life < p->cfg->fade_start) {
                alpha = p->life / p->cfg->fade_start;
            } else {
                alpha = 1.0f;
            }

            pool.color->a = 255.0f * (alpha * p->cfg->glow);

            pool.size->assign(p->size, p->size);
        }

        p++;
    }

    return end - (particle*)mem;
}
