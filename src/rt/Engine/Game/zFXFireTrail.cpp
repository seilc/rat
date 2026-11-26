#include "xCommon.h"
#include "zFXFireTrail.h"

#include "xCam.h"
#include "xDebugTweak.h"
#include "xGlobals.h"
#include "xMath.h"
#include "xParticleBatch.h"
#include "xPtankPool.h"
#include "xRand.h"
#include "xstransvc.h"
#include "xString.h"

fire_trail::config fire_trail::default_config;

void fire_trail::setup()
{
    default_config.emit_rate = 300.0f;
    default_config.glow = 0.2f;
    default_config.max_life = 0.3f;
    default_config.max_size = 0.1f;
    default_config.min_life = 0.3f;
    default_config.min_size = 0.1f;
    default_config.size_delta = 6.0f;
    default_config.slow = 0.0f;
    default_config.velocity = 2.0f;
    default_config.gravity = 0.1f;
    default_config.fade_start = 0.1f;
    default_config.color = g_WHITE;
    default_config.system_emit_time = 0.1f;
    default_config.min_rot = -2.0f;
    default_config.max_rot = 2.0f;
    default_config.start_speed = 0.0f;

    need_setup = true;
}

bool fire_trail::need_setup = true;
S32 fire_trail::system_id;

void fire_trail::scene_setup()
{
    RwTexture* texture = (RwTexture*)xSTFindAsset(xStrHash("ExplosionFlash_Tan_64"), NULL);

    if (!texture) {
        return;
    }

    xParticleBatchSystem batch_system;
    batch_system.context = NULL;
    batch_system.update = update;
    batch_system.pool_type = PGT_POS_COLOR_SIZE_ROT;
    batch_system.rs.flags = ptank_pool::FLAG_BRIGHT;
    batch_system.rs.src_blend = rwBLENDONE;
    batch_system.rs.dst_blend = rwBLENDONE;
    batch_system.rs.texture = texture;
    batch_system.stride = sizeof(particle);

    system_id = xPARTICLEBATCHADDSYSTEM_L(batch_system, 53);

    need_setup = false;
}

void fire_trail::add_tweaks(const char* prefix, config* fire)
{
    xAUTOTWEAK(prefix, "fire->min_life", &fire->min_life, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->max_life", &fire->max_life, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->slow", &fire->slow, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->max_size", &fire->max_size, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->min_size", &fire->min_size, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->velocity", &fire->velocity, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->color.r", &fire->color.r, 0, 255, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->color.g", &fire->color.g, 0, 255, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->color.b", &fire->color.b, 0, 255, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->emit_rate", &fire->emit_rate, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->system_emit_time", &fire->system_emit_time, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->glow", &fire->glow, 0.0f, 1.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->fade_start", &fire->fade_start, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->min_rot", &fire->min_rot, -10000.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->max_rot", &fire->max_rot, -10000.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->size_delta", &fire->size_delta, -10000.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->size_delta2", &fire->size_delta2, -10000.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "fire->start_speed", &fire->start_speed, 0.0f, 10000.0f, NULL, NULL, 0, true);
}

void fire_trail::emit(const xVec3& position, const xVec3& normal, F32& emit_remaining, const xVec3& velocity, F32 dt,
                      xVec3*, config* cfg)
{
    if (need_setup) {
        scene_setup();
    }

    F32 time_passed = emit_remaining / cfg->emit_rate;
    emit_remaining += xParticleBatchRequest(system_id, cfg->emit_rate * dt, 0);

    dt -= time_passed;

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
            p->life = xurand() * (cfg->max_life - cfg->min_life) + cfg->min_life - dt;
            p->size = xurand() * (cfg->max_size - cfg->min_size) + cfg->min_size;
            p->size += cfg->size_delta * dt / 2.0f;
            p->vel.Scale(normal, cfg->start_speed);
            p->rot = PI * (xurand() - 0.5f);
            p->rot_delta = xurand() * (cfg->max_rot - cfg->min_rot) + cfg->min_rot;
            p->size_delta = cfg->size_delta;
            p->size_delta += cfg->size_delta2 * dt / 2.0f;
            p->size += p->size_delta * dt / 2.0f;

            xVec3 vel;
            vel.x = cfg->velocity * (xurand() - 0.5f);
            vel.y = cfg->velocity * (xurand() - 0.5f);
            vel.z = cfg->velocity * (xurand() - 0.5f);

            F32 dir_dot = vel.dot(normal);
            if (dir_dot < 0.0f) {
                vel.AddScale(normal, -2.0f * dir_dot);
            }

            p->vel += vel;

            F32 vel_length = p->vel.length();
            if (vel_length != 0.0f) {
                p->vel.Scale(MAX(vel_length - cfg->slow * dt, 0.0f) / vel_length);
            }

            p->pos.AddScale(position, velocity, time_passed);
            p->cfg = cfg;
            p->pos.AddScale(p->pos, p->vel, dt);

            xASSERT_L(144, cfg->emit_rate);
            dt -= 1.0f / cfg->emit_rate;
            time_passed += 1.0f / cfg->emit_rate;
            p++;
        }

        emit -= count;
    }
}

S32 fire_trail::update(U8* mem, S32 count, ptank_pool& pool_, F32 dt, void*)
{
    ptank_pool__pos_color_size_rot& pool = (ptank_pool__pos_color_size_rot&)pool_;

    particle* p = (particle*)mem;
    particle* end = p + count;

    while (p != end) {
        p->life -= dt;

        if (p->life < 0.0f) {
            *p = *--end;
            continue;
        }

        F32 vel_length = p->vel.length();
        if (vel_length != 0.0f) {
            p->vel.Scale(MAX(vel_length - p->cfg->slow * dt, 0.0f) / vel_length);
        }

        p->pos.AddScale(p->vel, dt);

        p->rot += p->rot_delta * dt;
        p->rot = xDangleClamp(p->rot);

        p->size_delta += p->cfg->size_delta2 * dt;
        p->size += p->size_delta * dt;

        xIF_SPHERE_VISIBLE(p->pos.m_RwV3d, p->size) {
            pool.next();
            if (!pool.valid()) {
                end = p;
                break;
            }

            *pool.pos = p->pos;

            pool.SetRot(p->rot);

            F32 fade;
            if (p->life < p->cfg->fade_start)
                fade = p->life / p->cfg->fade_start;
            else
                fade = 1.0f;

            pool.color->b = p->cfg->color.b * fade;
            pool.color->g = p->cfg->color.g * fade;
            pool.color->r = p->cfg->color.r * fade;
            pool.color->a = 255.0f * p->cfg->glow;

            pool.size->assign(p->size, p->size);
        }

        p++;
    }

    return end - (particle*)mem;
}
