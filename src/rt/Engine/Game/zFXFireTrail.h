#ifndef ZFXFIRETRAIL_H
#define ZFXFIRETRAIL_H

#include "xColor.h"
#include "xMath3.h"

struct ptank_pool;

class fire_trail
{
public:
    struct config
    {
        F32 min_life;
        F32 max_life;
        F32 min_size;
        F32 max_size;
        F32 velocity;
        F32 emit_rate;
        F32 gravity;
        F32 slow;
        F32 fade_start;
        F32 system_emit_time;
        F32 glow;
        F32 min_rot;
        F32 max_rot;
        F32 size_delta;
        F32 size_delta2;
        F32 start_speed;
        xColor color;
    };

    struct particle
    {
        xVec3 pos;
        F32 size;
        xVec3 vel;
        F32 life;
        F32 rot;
        F32 rot_delta;
        F32 size_delta;
        config* cfg;
    };

    static void setup();
    static void scene_setup();
    static void add_tweaks(const char* prefix, config* fire);
    static void emit(const xVec3& position, const xVec3& normal, F32& emit_remaining, const xVec3& velocity, F32 dt,
                     xVec3*, config* cfg);
    static S32 update(U8* mem, S32 count, ptank_pool& pool_, F32 dt, void*);

    static config default_config;
    static bool need_setup;
    static S32 system_id;
};

#endif
