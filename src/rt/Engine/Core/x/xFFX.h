#ifndef XFFX_H
#define XFFX_H

#include "xEnt.h"

typedef void(*xFFXEffectCallback)(xEnt*, xScene*, F32, void*);

struct xFFX
{
    U32 flags;
    xFFXEffectCallback doEffect;
    void* fdata;
    xFFX* next;
};

struct xFFXRotMatchState
{
    S32 lgrounded;
    xVec3 lfup;
    xVec3 lfat;
    xVec3 plfat;
    F32 tmr;
    F32 mrate;
    F32 tmatch;
    F32 rrate;
    F32 trelax;
    F32 max_decl;
    xFFXRotMatchState* next;
};

struct xFFXShakeState
{
    xVec3 disp;
    F32 dur;
    F32 freq;
    F32 tmr;
    F32 alpha;
    F32 lval;
    xFFXShakeState* next;
};

void xFFXPoolInit(U32 num_ffx);
xFFX* xFFXAlloc();
void xFFXFree(xFFX* f);
U32 xFFXIsOn(xFFX* f); // UNUSED
void xFFXTurnOn(xFFX* f); // UNUSED
void xFFXTurnOff(xFFX* f); // UNUSED
void xFFXToggleOn(xFFX* f); // UNUSED
U8 xFFXGetNumFFX(const xEnt* ent); // UNUSED
xFFX* xFFXGetEffect(xEnt* ent, U8 n); // UNUSED
S16 xFFXAddEffect(xEnt* ent, xFFX* f);
S16 xFFXAddEffect(xEnt* ent, xFFXEffectCallback dof, void* fd); // UNUSED
U32 xFFXRemoveEffect(xEnt* ent, xFFX* f); // UNUSED
U32 xFFXRemoveEffectByIdx(xEnt* ent, U8 n); // UNUSED
U32 xFFXRemoveEffectByFData(xEnt* ent, void* fdata);
void xFFXApply(xEnt* ent, xScene* sc, F32 dt);
void xFFXShakeUpdateEnt(xEnt* ent, xScene* sc, F32 dt, void* fdata);
void xFFXShakePoolInit(U32 num);
xFFXShakeState* xFFXShakeAlloc();
void xFFXShakeFree(xFFXShakeState* s);
void xFFXRotMatchUpdateEnt(xEnt* ent, xScene* sc, F32 dt, void* fdata); // UNUSED
void xFFXRotMatchPoolInit(U32 num);
xFFXRotMatchState* xFFXRotMatchAlloc(); // UNUSED
void xFFXRotMatchFree(xFFXRotMatchState* s); // UNUSED

#endif
