#ifndef ZFXLIGHTDUST_H
#define ZFXLIGHTDUST_H

#include "xMath3.h"

struct ptank_pool;

namespace zFX
{

class light_dust
{
public:
    struct config
    {
        F32 min_life;
        F32 max_life;
        F32 min_size;
        F32 max_size;
        F32 velocity_min;
        F32 velocity_max;
        F32 emit_rate;
        F32 slow;
        F32 fade_start;
        F32 gravity;
        U8 r;
        U8 g;
        U8 b;
        U8 cinematic_only;
        F32 system_emit_time;
        F32 glow;
        F32 min_rot;
        F32 max_rot;
        F32 size_delta;
        xVec3 start_velocity;
        xVec3 velocity_dir_scale;
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

    static void setup();
    static void scene_setup();
    static void add_tweaks(const char* prefix, config* dust);
    static void emit(const xVec3& position, const xVec3& normal, F32& emit_remaining, const xVec3& velocity, F32 dt,
                     config* cfg, F32 velocity_scale, F32 emit_scale);
    static void emit_line(const xVec3& startPos, const xVec3& endPos, const xVec3& normal, F32& emit_remaining,
                          const xVec3& velocity, F32 dt, config* cfg); // UNUSED
    static S32 update(U8* mem, S32 count, ptank_pool& pool_, F32 dt, void*);

    static config default_config;
    static S32 system_id;
    static bool need_setup;
};

} // namespace zFX

#endif
