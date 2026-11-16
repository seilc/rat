#ifndef ZFXPUNCHSNOW_H
#define ZFXPUNCHSNOW_H

#include "xMath3.h"

struct ptank_pool;

namespace zFX
{

class punch_snow
{
public:
    struct config
    {
        F32 min_life;
        F32 max_life;
        F32 min_size;
        F32 max_size;
        F32 emit;
        F32 emit_rate;
        F32 slow;
        F32 fade_start;
        F32 gravity;
        U8 r;
        U8 g;
        U8 b;
        U8 cinematic_only;
        F32 glow;
        F32 min_rot;
        F32 max_rot;
        F32 size_delta;
        F32 vel_right;
        F32 vel_up;
        F32 vel_at;
        F32 nrm_right;
        F32 nrm_up;
        F32 nrm_at;
        F32 cone_rnd;
        F32 cone_vel;
    };

    struct particle
    {
        xVec3 pos;
        F32 size;
        xVec3 vel;
        F32 life;
        F32 rot;
        F32 rot_delta;
        S32 texture_index;
        config* cfg;
    };

    static void setup(); // UNUSED
    static void scene_setup(); // UNUSED
    static void add_tweaks(const char* prefix, config* cfg); // UNUSED
    static void emit(const xVec3& position, const xVec3&, F32& emit_remaining, const xVec3& velocity, F32 dt,
                     config* cfg); // UNUSED
    static S32 update(U8* mem, S32 count, ptank_pool& pool_, F32 dt, void*); // UNUSED

    static config default_config;
    static S32 system_id;
    static bool need_setup;
};

} // namespace zFX

#endif
