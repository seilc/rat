#ifndef ZROCKET_H
#define ZROCKET_H

#include "xMath3.h"

struct ptank_pool;

namespace z {

class smoke_trail
{
public:
    struct smoke_config
    {
        F32 floor;
        F32 SMOKE_ALPHA_DELTA;
        F32 SMOKE_SIZE_DELTA;
        F32 SMOKE_START_SIZE;
        xVec3 SMOKE_VELOCITY;
        F32 SMOKE_EMIT_RATE;
        F32 SMOKE_ROTATION_DELTA_MIN;
        F32 SMOKE_ROTATION_DELTA_MAX;
        F32 SMOKE_COLOR;
        F32 VELOCITY_RANDOM;
        F32 START_ALPHA;
        bool distance_emit;
    };

    struct particle
    {
        xVec3 pos;
        F32 size;
        xVec3 vel;
        F32 rot;
        F32 a;
        F32 c;
        F32 rot_delta;
        smoke_config* cfg;
    };

#if DEBUG || RELEASE
    static void add_tweaks(const char* prefix, smoke_config* smoke);
#endif
    static void setup();
    static void scene_setup();
    static S32 update(U8* mem, S32 count, ptank_pool& pool_, F32 dt, void*);
    static void emit(F32& emit_remaining, const xVec3 position, const xVec3& velocity, F32 dt, smoke_config* cfg);

    static S32 system_id;
    static smoke_config default_config;
};

} // namespace z

#endif
