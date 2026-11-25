#include "xCommon.h"
#include "zRocket.h"

#include "xCam.h"
#include "xDebugTweak.h"
#include "xEntMotion.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xMath.h"
#include "xParticleBatch.h"
#include "xPtankPool.h"
#include "xRand.h"
#include "xstransvc.h"
#include "xString.h"
#include "zCommonPlayer.h"
#include "zNPCCommon.h"
#include "zShrapnel.h"

namespace z {

#if DEBUG || RELEASE
void smoke_trail::add_tweaks(const char* prefix, smoke_config* smoke)
{
    xAUTOTWEAK(prefix, "smoke->SMOKE_ALPHA_DELTA", &smoke->SMOKE_ALPHA_DELTA, -1000.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "smoke->SMOKE_SIZE_DELTA", &smoke->SMOKE_SIZE_DELTA, -1000.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "smoke->SMOKE_START_SIZE", &smoke->SMOKE_START_SIZE, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "smoke->SMOKE_VELOCITY", &smoke->SMOKE_VELOCITY, 0, 0, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "smoke->VELOCITY_RANDOM", &smoke->VELOCITY_RANDOM, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "smoke->SMOKE_EMIT_RATE", &smoke->SMOKE_EMIT_RATE, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "smoke->SMOKE_ROTATION_DELTA_MIN", &smoke->SMOKE_ROTATION_DELTA_MIN, -1000.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "smoke->SMOKE_ROTATION_DELTA_MAX", &smoke->SMOKE_ROTATION_DELTA_MAX, -1000.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "smoke->SMOKE_COLOR", &smoke->SMOKE_COLOR, 0.0f, 1.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "smoke->START_ALPHA", &smoke->START_ALPHA, 0.0f, 1.0f, NULL, NULL, 0, true);
}
#endif

smoke_trail::smoke_config smoke_trail::default_config;
S32 smoke_trail::system_id = -1;
static bool need_setup = true;

void smoke_trail::setup()
{
    need_setup = true;

    default_config.floor = -HUGE;
    default_config.SMOKE_ALPHA_DELTA = -0.5f;
    default_config.SMOKE_SIZE_DELTA = 0.7f;
    default_config.SMOKE_START_SIZE = 0.7f;
    default_config.SMOKE_VELOCITY.assign(0.0f, 0.0f, 0.0f);
    default_config.SMOKE_EMIT_RATE = 3.5f;
    default_config.SMOKE_ROTATION_DELTA_MIN = -2.0f;
    default_config.SMOKE_ROTATION_DELTA_MAX = 2.0f;
    default_config.SMOKE_COLOR = 0.7f;
    default_config.VELOCITY_RANDOM = 0.5f;
    default_config.START_ALPHA = 1.0f;
    default_config.distance_emit = true;
}

void smoke_trail::scene_setup()
{
    RwTexture* texture = (RwTexture*)xSTFindAsset(xStrHash("rocket_smoke"), NULL);

    xParticleBatchSystem batch_system;
    batch_system.context = NULL;
    batch_system.update = update;
    batch_system.pool_type = PGT_POS_COLOR_SIZE_ROT;
    batch_system.rs.flags = 0;
    batch_system.rs.src_blend = rwBLENDSRCALPHA;
    batch_system.rs.dst_blend = rwBLENDINVSRCALPHA;
    batch_system.rs.texture = texture;
    batch_system.stride = sizeof(particle);

    system_id = xPARTICLEBATCHADDSYSTEM_NAMED(batch_system, "zRocket:smoke_trail");

    need_setup = false;

#if DEBUG || RELEASE
    add_tweaks("NPC OLD|systems|", &default_config);
#endif
}

S32 smoke_trail::update(U8* mem, S32 count, ptank_pool& pool_, F32 dt, void*)
{
    ptank_pool__pos_color_size_rot& pool = (ptank_pool__pos_color_size_rot&)pool_;

    particle* p = (particle*)mem;
    particle* end = p + count;

    while (p != end) {
        p->a += p->cfg->SMOKE_ALPHA_DELTA * dt;

        if (p->a <= 0.0f) {
            *p = *--end;
            continue;
        }

        p->pos.AddScale(p->vel, dt);
        p->pos.y = MAX(p->size / 2.0f + p->cfg->floor, p->pos.y);

        p->size += p->cfg->SMOKE_SIZE_DELTA * dt;
        p->rot += p->rot_delta * dt;
        p->rot = xrmod(p->rot + PI) - PI;

        xIF_SPHERE_VISIBLE(p->pos.m_RwV3d, p->size) {
            pool.next();
            if (!pool.valid()) {
                return p - (particle*)mem;
            }

            *pool.pos = p->pos;

            pool.SetRot(p->rot);

            pool.color->b = 255.0f * p->c;
            pool.color->g = 255.0f * p->c;
            pool.color->r = 255.0f * p->c;
            pool.color->a = 255.0f * p->a;

            pool.size->assign(p->size, p->size);
        }

        p++;
    }

    return end - (particle*)mem;
}

void smoke_trail::emit(F32& emit_remaining, const xVec3 position, const xVec3& velocity, F32 dt, smoke_config* cfg)
{
    if (need_setup) {
        scene_setup();
    }

    F32 emit_rate;
    if (cfg->distance_emit) {
        emit_rate = cfg->SMOKE_EMIT_RATE * velocity.length();
        if (emit_rate < cfg->SMOKE_EMIT_RATE)
            emit_rate = cfg->SMOKE_EMIT_RATE;
    } else {
        emit_rate = cfg->SMOKE_EMIT_RATE;
    }

    F32 time_passed = emit_remaining / emit_rate;
    emit_remaining += xParticleBatchRequest(system_id, emit_rate * dt, 0);

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
            p->a = cfg->SMOKE_ALPHA_DELTA * dt + cfg->START_ALPHA;
            p->c = cfg->SMOKE_COLOR;
            p->size = cfg->SMOKE_SIZE_DELTA * dt + cfg->SMOKE_START_SIZE;
            p->vel = cfg->SMOKE_VELOCITY;
            p->vel.x += cfg->VELOCITY_RANDOM * (xurand() - 0.5f);
            p->vel.y += cfg->VELOCITY_RANDOM * (xurand() - 0.5f);
            p->vel.z += cfg->VELOCITY_RANDOM * (xurand() - 0.5f);
            p->rot = 2.0f * PI * xurand() - PI;
            p->rot_delta = xurand() * (cfg->SMOKE_ROTATION_DELTA_MAX - cfg->SMOKE_ROTATION_DELTA_MIN) + cfg->SMOKE_ROTATION_DELTA_MIN;
            p->cfg = cfg;

            p->pos.AddScale(position, velocity, time_passed);
            p->pos.AddScale(p->pos, p->vel, dt);

            time_passed += 1.0f / emit_rate;
            dt -= 1.0f / emit_rate;
            p++;
        }

        emit -= count;
    }
}

} // namespace z
