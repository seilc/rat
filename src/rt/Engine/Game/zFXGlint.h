#ifndef ZFXGLINT_H
#define ZFXGLINT_H

#include "xMath3.h"

struct ptank_pool;

namespace zFX
{

class glint
{
public:
    struct config
    {
        F32 min_life;
        F32 max_life;
        F32 min_size;
        F32 max_size;
        F32 emit_rate;
        F32 fade_in;
        F32 fade_out;
        U8 r;
        U8 g;
        U8 b;
        U8 cinematic_only;
        F32 size_delta;
    };

    struct particle
    {
        xVec3 pos;
        F32 size;
        F32 life;
        F32 lifetime;
        config* cfg;
        U32 pad;
    };

    static void setup();
    static void scene_setup();
    static void add_tweaks(const char* prefix, config* glint); // UNUSED
    static void emit(const xVec3& position, F32& emit_remaining, F32 dt, config* cfg);
    static S32 update(U8* mem, S32 count, ptank_pool& pool_, F32 dt, void*);

    static config default_config;
    static S32 system_id;
    static bool need_setup;
};

} // namespace zFX

#endif
