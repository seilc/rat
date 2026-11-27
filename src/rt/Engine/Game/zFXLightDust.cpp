#include "xCommon.h"
#include "zFXLightDust.h"

#include "containers.h"
#include "xCam.h"
#include "xDebugTweak.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xMath.h"
#include "xParticleBatch.h"
#include "xPtankPool.h"
#include "xRand.h"
#include "xString.h"
#include "xstransvc.h"
#include "zGlobals.h"
#include "zScene.h"

bool zFX::light_dust::need_setup;
zFX::light_dust::config zFX::light_dust::default_config;
S32 zFX::light_dust::system_id = -1;

void zFX::light_dust::setup()
{
    need_setup = true;

    default_config.min_life = 0.5f;
    default_config.max_life = 0.5f;
    default_config.min_size = 0.2f;
    default_config.max_size = 0.2f;
    default_config.velocity_min = 50.0f;
    default_config.velocity_max = 50.0f;
    default_config.emit_rate = 100.0f;
    default_config.gravity = 0.1f;
    default_config.slow = 500.0f;
    default_config.fade_start = 0.2f;
    default_config.gravity = 0.0f;
    default_config.r = 255;
    default_config.g = 255;
    default_config.b = 255;
    default_config.cinematic_only = 0;
    default_config.system_emit_time = 0.1f;
    default_config.glow = 0.2f;
    default_config.min_rot = -5.0f;
    default_config.max_rot = 5.0f;
    default_config.size_delta = 2.0f;
    default_config.start_velocity.assign(0.0f, 0.0f, 0.0f);
    default_config.velocity_dir_scale.assign(1.0f, 1.0f, 1.0f);
}

void zFX::light_dust::scene_setup()
{
    RwTexture* texture = (RwTexture*)xSTFindAsset(xStrHash("fx_bot_dust"), NULL);

    xParticleBatchSystem batch_system;
    batch_system.context = NULL;
    batch_system.update = update;
    batch_system.pool_type = PGT_POS_COLOR_SIZE_ROT_UV2;
    batch_system.rs.flags = 0;
    batch_system.rs.src_blend = rwBLENDSRCALPHA;
    batch_system.rs.dst_blend = rwBLENDINVSRCALPHA;
    batch_system.rs.texture = texture;
    batch_system.stride = sizeof(particle);

    system_id = xPARTICLEBATCHADDSYSTEM_NAMED(batch_system, "zNPCBotLeap:light_dust");
}

void zFX::light_dust::add_tweaks(const char* prefix, config* dust)
{
    xAUTOTWEAK(prefix, "dust->min_life", &dust->min_life, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->max_life", &dust->max_life, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->slow", &dust->slow, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->max_size", &dust->max_size, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->min_size", &dust->min_size, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->velocity_min", &dust->velocity_min, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->velocity_max", &dust->velocity_max, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->r", &dust->r, 0, 255, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->g", &dust->g, 0, 255, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->b", &dust->b, 0, 255, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->emit_rate", &dust->emit_rate, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->system_emit_time", &dust->system_emit_time, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->glow", &dust->glow, 0.0f, 1.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->fade_start", &dust->fade_start, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->min_rot", &dust->min_rot, -10000.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->gravity", &dust->gravity, -10000.0f, 10000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->max_rot", &dust->max_rot, -10000.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->size_delta", &dust->size_delta, -10000.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->start_velocity", &dust->start_velocity, 0, 0, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "dust->velocity_dir_scale", &dust->velocity_dir_scale, 0, 0, NULL, NULL, 0, true);
}

void zFX::light_dust::emit(const xVec3& position, const xVec3& normal, F32& emit_remaining, const xVec3& velocity,
                           F32 dt, config* cfg, F32 velocity_scale, F32 emit_scale)
{
    if (need_setup) {
        need_setup = false;
        scene_setup();
    }

    F32 denom = cfg->emit_rate * emit_scale, time_passed;
    if (denom != 0.0f)
        time_passed = (1.0f - emit_remaining) / (cfg->emit_rate * emit_scale);
    else
        time_passed = 1.0f - emit_remaining;

    emit_remaining += xParticleBatchRequest(system_id, cfg->emit_rate * emit_scale * dt, 0);

    S32 emit = emit_remaining;
    emit_remaining -= emit;

    while (emit > 0) {
        particle* p;
        S32 count = xParticleBatchEmit(system_id, emit, (U8**)&p);
        if (count == 0) {
            emit_remaining = 0.0f;
            break;
        }

        particle* end = p + count;

        while (p != end) {
            p->life = xurand() * (cfg->max_life - cfg->min_life) + cfg->min_life - time_passed;
            p->size = xurand() * (cfg->max_size - cfg->min_size) + cfg->min_size;

            p->vel.assign(xurand() - 0.5f, xurand() - 0.5f, xurand() - 0.5f);
            p->vel.ScaleComponents(cfg->velocity_dir_scale);
            p->vel.AddScale(normal, -p->vel.dot(normal));
            p->vel.safe_normalize(g_O3);

            p->vel *= xurand() * (cfg->velocity_max - cfg->velocity_min) + cfg->velocity_min;
            p->vel *= velocity_scale;
            p->vel += cfg->start_velocity;
            p->rot = PI * (xurand() - 0.5f);
            p->rot_delta = xurand() * (cfg->max_rot - cfg->min_rot) + cfg->min_rot;

            F32 dir_dot = p->vel.dot(normal);
            if (dir_dot < 0.0f) {
                p->vel.AddScale(normal, -2.0f * dir_dot);
            }

            p->texture_index = 4.0f * xurand();

            F32 vel_length = p->vel.length();
            p->vel.Scale(MAX(vel_length - cfg->slow * dt, 0.0f) / vel_length);
            p->vel.y += cfg->size_delta / 2.0f;
            p->vel.y -= cfg->gravity * dt;

            p->pos.AddScale(position, velocity, time_passed);

            p->pos.y += p->size / 2.0f;
            p->cfg = cfg;

            p->pos.AddScale(p->pos, p->vel, dt);

            dt -= 1.0f / (cfg->emit_rate * emit_scale);
            time_passed += 1.0f / (cfg->emit_rate * emit_scale);
            p++;
        }

        emit -= count;
    }
}

S32 zFX::light_dust::update(U8* mem, S32 count, ptank_pool& pool_, F32 dt, void*)
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

        F32 vel_length = p->vel.length();
        if (vel_length > EPSILON) {
            p->vel.Scale(MAX(vel_length - p->cfg->slow * dt, 0.0f) / vel_length);
        } else {
            p->vel = g_O3;
        }

        p->vel.y -= p->cfg->gravity * dt;
        p->pos.AddScale(p->vel, dt);

        p->rot += p->rot_delta * dt;
        p->rot = xDangleClamp(p->rot);

        p->size += p->cfg->size_delta * dt;

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
