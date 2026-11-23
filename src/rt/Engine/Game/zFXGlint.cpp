#include "xCommon.h"
#include "zFXGlint.h"

#include "containers.h"
#include "xCam.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xParticleBatch.h"
#include "xPtankPool.h"
#include "xRand.h"
#include "xstransvc.h"
#include "zGlobals.h"
#include "zHashes.h"
#include "zScene.h"

bool zFX::glint::need_setup;
zFX::glint::config zFX::glint::default_config;

void zFX::glint::setup()
{
    need_setup = true;

    default_config.min_life = 0.1f;
    default_config.max_life = 0.4f;
    default_config.min_size = 0.1f;
    default_config.max_size = 0.3f;
    default_config.emit_rate = 20.0f;
    default_config.fade_in = 0.4f;
    default_config.fade_out = 0.6f;
    default_config.r = 255;
    default_config.g = 255;
    default_config.b = 255;
    default_config.cinematic_only = 0;
    default_config.size_delta = 2.0f;
}

void zFX::glint::scene_setup()
{
    RwTexture* texture = (RwTexture*)xSTFindAsset(HASH_glare_hit_64, NULL);
    xASSERT_FMT_L(57, texture != NULL, "Missing glint texture 0x%08X\n", HASH_glare_hit_64);

    xParticleBatchSystem batch_system;
    batch_system.context = NULL;
    batch_system.update = update;
    batch_system.pool_type = PGT_POS_COLOR_SIZE_ROT_UV2;
    batch_system.rs.flags = 0;
    batch_system.rs.src_blend = rwBLENDSRCALPHA;
    batch_system.rs.dst_blend = rwBLENDONE;
    batch_system.rs.texture = texture;
    batch_system.stride = sizeof(particle);

    system_id = xPARTICLEBATCHADDSYSTEM_NAMED(batch_system, "Glints");
}

S32 zFX::glint::system_id = -1;

// UNUSED: needed to match (speculative code)
void zFX::glint::add_tweaks(const char* prefix, config* glint)
{
    xAUTOTWEAK(prefix, "glint->min_life", &glint->min_life, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "glint->max_life", &glint->max_life, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "glint->max_size", &glint->max_size, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "glint->min_size", &glint->min_size, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "glint->r", &glint->r, 0, 255, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "glint->g", &glint->g, 0, 255, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "glint->b", &glint->b, 0, 255, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "glint->emit_rate", &glint->emit_rate, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "glint->fade_in", &glint->fade_in, 0.0f, 1.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "glint->fade_out", &glint->fade_out, 0.0f, 1.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "glint->size_delta", &glint->size_delta, -10000.0f, 1000.0f, NULL, NULL, 0, true);
}

void zFX::glint::emit(const xVec3& position, F32& emit_remaining, F32 dt, config* cfg)
{
    if (need_setup) {
        need_setup = false;
        scene_setup();
    }

    F32 time_passed = (1.0f - emit_remaining) / cfg->emit_rate;
    emit_remaining += xParticleBatchRequest(system_id, cfg->emit_rate * dt, 0);

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
            p->lifetime = p->life;
            p->size = xurand() * (cfg->max_size - cfg->min_size) + cfg->min_size;
            p->pos = position;
            p->cfg = cfg;

            dt -= 1.0f / cfg->emit_rate;
            time_passed += 1.0f / cfg->emit_rate;
            p++;
        }

        emit -= count;
    }
}

S32 zFX::glint::update(U8* mem, S32 count, ptank_pool& pool_, F32 dt, void*)
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

        xIF_SPHERE_VISIBLE(p->pos.m_RwV3d, p->size) {
            pool.next();
            if (!pool.valid()) {
                end = p;
                break;
            }

            *pool.pos = p->pos;

            pool.color->r = p->cfg->r;
            pool.color->g = p->cfg->g;
            pool.color->b = p->cfg->b;

            pool.uv[0].assign(0.0f, 0.0f);
            pool.uv[1].assign(1.0f, 1.0f);

            pool.SetRot(0.0f);

            F32 alpha;
            F32 fade_in = p->lifetime * p->cfg->fade_in;
            F32 fade_out = p->lifetime * p->cfg->fade_out;
            F32 life = p->lifetime - p->life;
            if (life < fade_in) {
                alpha = life / fade_in;
            } else if (life > fade_out) {
                // BUG: negative, the U8 store below wraps it
                alpha = (fade_out - life) / (p->lifetime - fade_out);
            } else {
                alpha = 1.0f;
            }

            pool.color->a = 255.0f * alpha;

            pool.size->assign(p->size, p->size);
        }

        p++;
    }

    return end - (particle*)mem;
}
