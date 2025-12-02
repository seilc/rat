#ifndef XPAREMITTER_H
#define XPAREMITTER_H

#include "xBase.h"
#include "xParEmitterAsset.h"
#include "xModel.h"

struct xParGroup;
struct xParSys;

struct xParEmitter : xBase
{
    xParEmitterAsset* tasset;
    xParGroup* group;
    xParEmitterPropsAsset* prop;
    U32 sortID;
    U8 rate_mode;
    xVec3 last_emit_position;
    xVec3 last_emit_velocity;
    F32 rate;
    F32 rate_time;
    F32 rate_fraction;
    F32 rate_fraction_cull;
    U8 emit_flags;
    U8 tweak_flags;
    U8 emit_pad[2];
    U8 rot[3];
    xModelTag tag;
    F32 oocull_distance_sqr;
    F32 distance_to_cull_sqr;
    void* attachTo;
    xParSys* parSys;
    xBase* emit_volume;
    xVec3 last_attach_loc;
};

#endif
