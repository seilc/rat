#ifndef ZHANGABLE_H
#define ZHANGABLE_H

#include "xBase.h"
#include "xMath3.h"
#include "zHangableAsset.h"

struct xEnt;
struct zScene;

struct zHangable : xBase
{
    zHangableAsset* hangableAsset;
    U32 flags;
};

struct zHangableObject
{
    zHangable* hangable;
    xEnt* object;
    F32 radius;
    xVec3 vel;
    xVec3 accel;
    xVec3 handle;
    xMat3x3 centerMat;
    xVec3 rotAxis;
    xVec3 lastDisp;
    xVec3 objVel;
};

void zHangable_Init(xBase& data, xDynAsset& asset, size_t asset_size);
void zHangable_Init(zHangable* hangable, zHangableAsset* asset);
void zHangable_AttachPlayer(zHangableObject* hangObj); // UNUSED
void zHangable_PlayerDPos(xVec3* dpos); // UNUSED
S32 zHangable_Recurse(U32 id, zHangableObject* gpList, zHangable* hangable);
void zHangable_SceneInit(zScene* zsc);
void zHangable_Reset();
void zHangable_Update(F32 dt);
void zHangable_Recenter(zHangableObject* hangObj);
void zHangable_EventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);
S32 zHangable_NumHangableObjects(); // UNUSED
zHangableObject* zHangable_GetHangableObject(S32 i); // UNUSED

#endif
