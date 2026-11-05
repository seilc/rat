#ifndef XPAREMITTER_H
#define XPAREMITTER_H

#include "xBase.h"
#include "xParEmitterAsset.h"
#include "xModel.h"
#include "xParSys.h"

struct xSerial;
struct xScene;

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
#if DEBUG || RELEASE
    U8 tweak_flags;
    U8 emit_pad[2];
#else
    U8 emit_pad[3];
#endif
    U8 rot[3];
    xModelTag tag;
    F32 oocull_distance_sqr;
    F32 distance_to_cull_sqr;
    void* attachTo;
    xParSys* parSys;
    xBase* emit_volume;
    xVec3 last_attach_loc;
};

struct xParEmitterCustomSettings : xParEmitterPropsAsset
{
    U32 custom_flags;
    U32 attachToID;
    xVec3 pos;
    xVec3 last_emit_position;
    xVec3 vel;
    xVec3 last_emit_velocity;
    F32 vel_angle_variation;
    U8 rot[3];
    U8 padding;
    F32 radius;
    F32 emit_interval_current;
    xBase* emit_volume;
};

S32 xParInterpConvertInterpMode(xParInterp* p);
void xParEmitterInit(void* b, void* tasset);
void xParEmitterInit(xBase* b, xParEmitterAsset* pea);
void xParEmitterSetup(xParEmitter* t);
void xParEmitterReset(xParEmitter* t);
void xParEmitterSave(xParEmitter* ent, xSerial* s);
void xParEmitterLoad(xParEmitter* ent, xSerial* s);
void xParEmitterEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);
xPar* xParEmitterEmitCustom(xParEmitter* p, F32 dt, xParEmitterCustomSettings* info);
F32 xParInterpCompute(S32 interp_mode, xParInterp* r, F32 time, S32 time_has_elapsed, F32 lastVal);
xPar* xParEmitterEmitSetTexIdxs(xPar* p, const xParSys* ps);
xPar* xParEmitterEmit(xParEmitter* pe, F32 emit_dt, F32 par_dt);
void xParEmitterUpdate(xBase* to, xScene*, F32 dt);
void xParEmitterDestroy();

inline xPar* xParEmitterEmit(xParEmitter* pe, F32 dt)
{ return xParEmitterEmit(pe, dt, dt); }

#endif
