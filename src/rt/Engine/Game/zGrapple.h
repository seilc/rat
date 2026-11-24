#ifndef ZGRAPPLE_H
#define ZGRAPPLE_H

#include "xBase.h"
#include "zGrappleAsset.h"

struct xMat3x3;
struct zScene;

struct zGrapple : xBase
{
    zGrappleAsset* grappleAsset;
    U32 flags;
};

struct zGrapplePoint
{
    zGrapple* grap;
    xVec3* pos;
    xMat3x3* local;
    xVec3* offset;
    void* optr;
};

void zGrapple_Init(xBase& data, xDynAsset& asset, size_t asset_size);
void zGrapple_Init(zGrapple* grapple, zGrappleAsset* asset);
S32 zGrapple_Recurse(U32 id, zGrapplePoint* gpList, zGrapple* grapple);
void zGrapple_SceneInit(zScene* zsc);
void zGrapple_Reset();
void zGrapple_EventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);
S32 zGrapple_NumGrapplePoints(); // UNUSED
zGrapplePoint* zGrapple_GetGrapplePoint(S32 i); // UNUSED
void zGrapple_GetPosition(zGrapplePoint* point, xVec3* pos); // UNUSED

#endif
