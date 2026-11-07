#ifndef XSHADOW_H
#define XSHADOW_H

#include "xMath2.h"
#include "xMath3.h"

#include <rwcore.h>
#include <rpworld.h>

struct xEnt;

struct xShadowPoly
{
    xVec4 vert[3];
    F32 depth;
    F32 pad[3];
};

struct xShadowCache
{
    xVec3 pos;
    xVec2 radius;
    S32 entCount;
    S32 polyCount;
    F32 polyRayDepth[5];
    U16 castOnEnt;
    U16 castOnPoly;
    F32 minDepth;
    F32 maxDepth;
    U32 pad[3];
    xEnt* ent[16];
    xShadowPoly* poly;
};

extern S32 SHADOW_CACHE_POLY_MAX;

void xShadowManager_Add(xEnt* ent);
void xShadowSetWorld(RpWorld* world);
void xShadowSetLight(xVec3* target_pos, xVec3* in_vec, F32);
void xShadowCameraUpdate(void* model, void (*renderCB)(void*), xVec3* center, xVec3* radius, S32 shadowMode);
void xShadowVertical_FillCache(xShadowCache* cache, xVec3* pos, F32 r, F32 depth, F32 minNormY, bool castOnJSP, bool castOnEnts);
void xShadowVertical_DrawCache(xShadowCache* cache, F32 shadowFactor, F32 fadeDist, S32 shadowMode, RwMatrix* shadowMat, RwRaster* shadowRast);
void xShadowModelFileRender(RpAtomic* model, RwMatrix* animMat);

#endif
