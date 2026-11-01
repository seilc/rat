#ifndef XRENDERSTATE_H
#define XRENDERSTATE_H

#include "types.h"

#include <rwcore.h>
#include <rpworld.h>

void xRenderSceneEnter();
void xRenderSceneExit();
void xRenderStateSetTexture(U32 assetID);
void xRenderStateSetTexture(RwTexture* texture);
void xRenderStateSetSrcBlendMode(S32 xmode);
void xRenderStateSetDstBlendMode(S32 xmode);
void xRenderStateEnableStdBlend(S32 enable);
void xRenderStateSetZBias(RwReal bias);
void xRenderStateResetZBias();
void xRenderStateSetAlphaDiscard(RwInt32 value);
void xRenderStateResetAlphaDiscard();
void xRenderFixUntexturedBegin(RpAtomic*);
void xRenderFixUntexturedEnd(RpAtomic*);
void xRenderFixIMBegin();
void xRenderFixIMEnd();
void xRenderFixClampBegin(RpAtomic*);
void xRenderFixClampEnd(RpAtomic*);

#if DEBUG || RELEASE
RwBool xRenderStateCheckBlendModes(RwBlendFunction desiredSrcBlend, RwBlendFunction desiredDestBlend);
RwBool xRenderStateCheckState(RwRenderState state, void* desiredValue);
void xRenderStateSetState(RwRenderState state, void* value);
void RwRenderStateSetWrapper(RwRenderState state, void* value);
#define RwRenderStateSet RwRenderStateSetWrapper
RwBool RwRenderStateGetWrapper(RwRenderState state, void* value);
#define RwRenderStateGet RwRenderStateGetWrapper
#else
#define xRenderStateCheckBlendModes FALSE
#define xRenderStateCheckState FALSE
#endif

void xRenderStateBlendModesSet(RwBlendFunction srcBlend, RwBlendFunction destBlend);
void xRenderStateZModesSet(RwBool ZWrite, RwBool ZTest);
void xRenderStateBlendAndZModesSet(RwBlendFunction srcBlend, RwBlendFunction destBlend, RwBool ZWrite, RwBool ZTest);
void xRenderStateBlendModesGet(RwBlendFunction* srcBlend, RwBlendFunction* destBlend);

#endif
