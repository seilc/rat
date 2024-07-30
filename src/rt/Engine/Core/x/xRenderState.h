#ifndef XRENDERSTATE_H
#define XRENDERSTATE_H

#include "types.h"

#include <rwcore.h>
#include <rpworld.h>

void xRenderSceneEnter();
void xRenderSceneExit();
void xRenderStateSetZBias(RwReal bias);
void xRenderStateResetZBias();
void xRenderStateSetAlphaDiscard(RwInt32 value);
void xRenderStateResetAlphaDiscard();
void xRenderFixUntexturedBegin(RpAtomic*);
void xRenderFixUntexturedEnd(RpAtomic*);
void xRenderFixIMBegin();
void xRenderFixIMEnd();

#ifdef DEBUGRELEASE
RwBool xRenderStateCheckBlendModes(RwBlendFunction desiredSrcBlend, RwBlendFunction desiredDestBlend);
RwBool xRenderStateCheckState(RwRenderState state, void* desiredValue);
void RwRenderStateSetWrapper(RwRenderState state, void* value);
RwBool RwRenderStateGetWrapper(RwRenderState state, void* value);
#else
#define xRenderStateCheckBlendModes FALSE
#define xRenderStateCheckState FALSE
#endif

void xRenderStateBlendModesSet(RwBlendFunction srcBlend, RwBlendFunction destBlend);
void xRenderStateZModesSet(RwBool ZWrite, RwBool ZTest);
void xRenderStateBlendAndZModesSet(RwBlendFunction srcBlend, RwBlendFunction destBlend, RwBool ZWrite, RwBool ZTest);
void xRenderStateBlendModesGet(RwBlendFunction* srcBlend, RwBlendFunction* destBlend);

#endif
