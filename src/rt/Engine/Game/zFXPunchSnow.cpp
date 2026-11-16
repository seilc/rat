#include "xCommon.h"
#include "zFXPunchSnow.h"

#include "xCam.h"
#include "xGlobals.h"
#include "xMath.h"
#include "xParticleBatch.h"
#include "xPtankPool.h"
#include "xRand.h"
#include "xstransvc.h"
#include "zGlobals.h"
#include "zHashes.h"

// UNUSED: needed to match (code from RotU GC)
void zFX::punch_snow::scene_setup()
{
    RwTexture* texture = (RwTexture*)xSTFindAsset(HASH_snowflakes, NULL);

    xParticleBatchSystem batch_system;
    batch_system.context = NULL;
    batch_system.update = update;
    batch_system.pool_type = PGT_POS_COLOR_SIZE_ROT_UV2;
    batch_system.rs.flags = 0;
    batch_system.rs.src_blend = rwBLENDSRCALPHA;
    batch_system.rs.dst_blend = rwBLENDINVSRCALPHA;
    batch_system.rs.texture = texture;
    batch_system.stride = sizeof(particle);

    system_id = xPARTICLEBATCHADDSYSTEM_NAMED(batch_system, "punch_snow");
}

// UNUSED: needed to match (code from RotU GC)
void zFX::punch_snow::emit(const xVec3& position, const xVec3&, F32& emit_remaining, const xVec3& velocity, F32 dt,
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
            p->vel = velocity;
            p->rot = PI * (xurand() - 0.5f);
            p->rot_delta = xurand() * (cfg->max_rot - cfg->min_rot) + cfg->min_rot;
            p->texture_index = 4.0f * xurand();
            p->vel.length();
            p->vel.y += cfg->gravity * dt;
            xVec3AddScaled(p->pos, position, velocity, time_passed);
            p->cfg = cfg;
            xVec3AddScaled(p->pos, p->pos, p->vel, dt);

            dt -= 1.0f / cfg->emit_rate;
            time_passed += 1.0f / cfg->emit_rate;
            p++;
        }

        emit_remaining -= count;
        emit -= count;
    }
}

// UNUSED: needed to match (code from RotU GC)
S32 zFX::punch_snow::update(U8* mem, S32 count, ptank_pool& pool_, F32 dt, void*)
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

        RwSphere worldsph;
        worldsph.center = p->pos.m_RwV3d;
        worldsph.radius = p->size;

        if (RwCameraFrustumTestSphere(xglobals->screen->icam, &worldsph) != rwSPHEREOUTSIDE) {
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
