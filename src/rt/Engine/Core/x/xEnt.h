#ifndef XENT_H
#define XENT_H

#include "xBase.h"
#include "xEntAsset.h"
#include "xModel.h"
#include "xCollide.h"
#include "xGridBound.h"
#include "xBound.h"
#include "xShadowSimple.h"
#include "xScene.h"

struct xFFX;

struct xEnt;
struct xEntFrame;
struct xEntCollis;
struct xEntDriveInfo;
struct xEntShadow;

struct xEntFrame
{
    xMat4x3 mat;
    xMat4x3 oldmat;
    xVec3 oldvel;
    xRot oldrot;
    xRot drot;
    xRot rot;
    xVec3 dvel;
    xVec3 vel;
    U32 mode;
    xVec3 dpos;

    xVec3& getDpos() const;
    xVec3& setDpos();
};

typedef void(*xEntCollisPostCallback)(xEnt*, xScene*, F32, xEntCollis*);
typedef U32(*xEntCollisDepenQueryCallback)(xEnt*, xEnt*, xScene*, F32, xCollis*);

struct xEntCollis
{
    U8 chk;
    U8 pen;
    U8 env_sidx;
    U8 env_eidx;
    U8 npc_sidx;
    U8 npc_eidx;
    U8 dyn_sidx;
    U8 dyn_eidx;
    U8 stat_sidx;
    U8 stat_eidx;
    U8 idx;
    xCollis colls[18];
    xEntCollisPostCallback post;
    xEntCollisDepenQueryCallback depenq;
};

struct xEntDriveInfo
{
    xMat4x3 driveMat;
    xVec3 centerOffset;
    U32 flags;
};

struct xEntShadow
{
    xVec3 pos;
    xVec3 vec;
    RpAtomic* shadowModel;
    F32 dst_cast;
    F32 radius[2];
    S32 flg_castOnOneDFF : 1;
    S32 flg_castOnAllDFF : 1;
    S32 flg_disableEnvCast : 1;
    S32 flg_shadowUnused : 29;
};

typedef void(*xEntVisUpdateCallback)(xEnt*);
typedef void(*xEntUpdateCallback)(xEnt*, xScene*, F32);
typedef void(*xEntEndUpdateCallback)(xEnt*, xScene*, F32);
typedef void(*xEntBoundUpdateCallback)(xEnt*, xVec3*);
typedef void(*xEntMoveCallback)(xEnt*, xScene*, F32, xEntFrame*);
typedef void(*xEntRenderCallback)(xEnt*);
typedef void(*xEntTranslateCallback)(xEnt*, xVec3*, xMat4x3*);

struct xEnt : xBase
{
    struct anim_coll_data;

    xEntAsset* asset;
    U16 idx;
    U8 flags;
    U8 miscflags;
    U8 subType;
    U8 pflags;
    U16 moreFlags;
    U8 isCulled : 2;
    U8 collisionEventReceived : 2;
    U8 driving_count : 7;
    U8 driving_reset : 1;
    U8 num_ffx;
    U8 collType;
    U8 collLev;
    U8 chkby;
    U8 penby;
    xEntVisUpdateCallback visUpdate;
    xModelInstance* model;
    xModelInstance* collModel;
    xModelInstance* camcollModel;
    xEntUpdateCallback update;
    xEntEndUpdateCallback endUpdate;
    xEntBoundUpdateCallback bupdate;
    xEntMoveCallback move;
    xEntRenderCallback render;
    xEntFrame* frame;
    xEntCollis* collis;
    xGridBound gridb;
    xBound bound;
    xEntTranslateCallback transl;
    xFFX* ffx;
    xEnt* driver;
    xEnt* driven;
    xEntDriveInfo* driveInfo;
    xShadowSimpleCache* simpShadow;
    xEntShadow* entShadow;
    anim_coll_data* anim_coll;
    void* user_data;
};

inline xVec3* xEntGetPos(const xEnt* ent)
{
    xASSERT_L(1078, ent->model);
    return &xModelGetFrame(ent->model)->pos;
}

inline xVec3* xEntGetFacingDirection(const xEnt* ent)
{
    xASSERT_L(1083, ent->model);
    return &xModelGetFrame(ent->model)->at;
}

inline xVec3* xEntGetUpDirection(const xEnt* ent)
{
    xASSERT_L(1088, ent->model);
    return &xModelGetFrame(ent->model)->up;
}

inline void xEntDisable(xEnt* ent)
{
    xBaseDisable(ent);
}

inline U32 xEntIsEnabled(const xEnt* ent)
{
    return xBaseIsEnabled(ent);
}

inline void xEntVisibilityCullOn(xEnt* ent)
{
    ent->flags |= (U8)0x4;
    if (ent->visUpdate) {
        ent->visUpdate(ent);
    }
}

inline void xEntVisibilityCullOff(xEnt* ent)
{
    ent->flags &= (U8)~0x4;
    if (ent->visUpdate) {
        ent->visUpdate(ent);
    }
}

inline xMat4x3* xEntGetFrame(const xEnt* ent)
{
    xASSERT_L(1098, ent->model);
    return xModelGetFrame(ent->model);
}

inline U32 xEntGetAnimFlags(const xEnt* ent)
{
    xASSERT_L(1103, ent->model != NULL && ent->model->Anim != NULL);
    return ent->model->Anim->Single->State->UserFlags;
}

bool xEntValidType(U8 type);
void xEntInit(xEnt* ent, xEntAsset* asset);
void xEntSetup(xEnt* ent);
void xEntReset(xEnt* ent);
void xEntBeginUpdate(xEnt* ent, xScene*, F32 dt);
void xEntEndUpdate(xEnt* ent, xScene* sc, F32 dt);
void xEntCollide(xEnt* ent, xScene* sc, F32 dt);
void xEntDebugAdd(xEnt* xent);
void xEntSetupPipeline(xSurface* surf, RpAtomic* model);
void xEntRestorePipeline(xSurface*, RpAtomic* model);
void xEntShow(xEnt* ent);
void xEntHide(xEnt* ent);

extern S32 xent_entent;
#if DEBUG || RELEASE
extern S32 xent_nqcrejects;
extern S32 gDrawInvisibleWalls;
#endif

inline U32 xEntIsVisible(const xEnt* ent)
{
#if DEBUG || RELEASE
    if (gDrawInvisibleWalls) {
        return TRUE;
    }
#endif

    return (ent->flags & 0x85) == 0x1;
}

struct xEntOpacity
{
    F32 startAlpha;
    F32 endAlpha;
    F32 deltaOpacity;
    U32 oldPipeFlags;
};

#endif
