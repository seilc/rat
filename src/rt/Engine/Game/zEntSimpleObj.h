#ifndef ZENTSIMPLEOBJ_H
#define ZENTSIMPLEOBJ_H

#include "xSimpleObjAsset.h"
#include "zEnt.h"

struct xModelBucket;
struct zDestructible;
struct zEntSimpleObj;
struct zReactiveAnimationData;
struct zReactiveGenre;

struct zSimpleMgr
{
    xSphere worldBound;
    F32 noRenderDist;
    F32 lodDist0;
    F32 fadeDist;
    U32 flags;
    xModelBucket** lodBucket[2];
    RpAtomic* lodAtomic[2];
    RwMatrix* mat;
    xModelInstance* model;
    zEntSimpleObj* ent;
};

struct zEntAnimData
{
    union
    {
        struct
        {
            zReactiveAnimationData* reactiveAnimationData;
            zReactiveGenre* reactGenre;
            zSimpleMgr* smgrExtra;
        };
        struct
        {
            void* anim;
            F32 animTime;
        };
    };
};

struct zEntSimpleObj : zEnt, zEntAnimData
{
    xSimpleObjAsset* sasset;
    U32 sflags;
    zDestructible* destructible;
    zSimpleMgr* smgr;
    xEntOpacity opacity;
};

#endif
