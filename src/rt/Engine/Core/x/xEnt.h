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
struct tweak_info;

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

    const xVec3& getDpos() const
    {
        return dpos;
    }

    xVec3& setDpos()
    {
        return dpos;
    }
};

typedef void(*xEntCollisPostCallback)(xEnt*, xScene*, F32, xEntCollis*);
typedef U32(*xEntCollisDepenQueryCallback)(xEnt*, xEnt*, xScene*, F32, xCollis*);

// xEntFrame::mode
#define k_XENT_SET_ROT (1 << 4)

// xEnt::flags
#define k_XENT_IS_VISIBLE ((U8)(1 << 0))

// xEnt::pflags
#define k_XENT_IS_MOVING ((U8)(1 << 0))
#define k_XENT_HAS_VELOCITY ((U8)(1 << 1))
#define k_XENT_HAS_GRAVITY ((U8)(1 << 2))
#define k_XENT_HAS_DRAG ((U8)(1 << 3))

// xEnt::collType, chkby, penby; xEntCollis::chk, pen
#define k_XENT_COLLTYPE_TRIG ((U8)(1 << (0)))
#define k_XENT_COLLTYPE_STAT ((U8)(1 << (1)))
#define k_XENT_COLLTYPE_DYN ((U8)(1 << (2)))
#define k_XENT_COLLTYPE_NPC ((U8)(1 << (3)))
#define k_XENT_COLLTYPE_PC ((U8)(1 << (4)))
#define k_XENT_COLLTYPE_INHIBIT ((U8)(1 << (5)))
#define k_XENT_COLLTYPE_ENV ((U8)(1 << (6)))

// xEnt::collLev
#define k_XENT_COLLLEV_FASTEST 0
#define k_XENT_COLLLEV_AABB 1
#define k_XENT_COLLLEV_SPH 2
#define k_XENT_COLLLEV_OBB 3
#define k_XENT_COLLLEV_BOUND 4
#define k_XENT_COLLLEV_MODEL 5

// xEnt::moreFlags
#define k_MORE_FLAGS_ANIM_COLL ((U8)1<<5)

#define k_XENT_MAX_COLL 18

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
    xCollis colls[k_XENT_MAX_COLL];
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
    enum radius_enum
    {
        RADIUS_CACHE,
        RADIUS_RASTER,
        MAX_RADIUS
    };

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
typedef void(*xEntBoundUpdateCallback)(xEnt*, xVec3*);
typedef void(*xEntMoveCallback)(xEnt*, xScene*, F32, xEntFrame*);
typedef void(*xEntRenderCallback)(xEnt*);
typedef void(*xEntTranslateCallback)(xEnt*, xVec3*, xMat4x3*);

struct xEnt : xBase
{
    struct anim_coll_data
    {
        U32 flags;
        U32 bones;
        xMat4x3 old_mat;
        xMat4x3 new_mat;
        size_t verts_size;
        xVec3* verts;
        xVec3* normals;
    };

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
    xEntUpdateCallback endUpdate;
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

inline xVec3* xEntGetCenter(const xEnt* ent)
{
    return xBoundCenter(&ent->bound);
}

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

struct xEntOpacity
{
    F32 startAlpha;
    F32 endAlpha;
    F32 deltaOpacity;
    U32 oldPipeFlags;
};

typedef xEnt*(*xEntCollCheckCallback)(xEnt*, xScene*, void*);

void xEntSceneInit();
void xEntSceneExit();
void xEntInit(xEnt* ent, U8 type, U8 collType, U8 subtype); // UNUSED
void xEntInit(xEnt* ent, xEntAsset* asset);
void xEntGrowGlobalBoundingBox(const xVec3* pos);
void xEntInitForType(xEnt* ent);
void xEntExit(xEnt* ent); // UNUSED
void xEntDrop(xEnt* ent);
void xEntSetup(xEnt* ent);
void xEntDrivenBySetup(xEnt* ent);
void xEntDrivenByReset(xEnt* ent);
void xEntSave(xEnt* ent, xSerial* s);
void xEntLoad(xEnt* ent, xSerial* s);
void xEntReset(xEnt* ent);
void xEntReset(xEnt* ent, const xVec3& pos, const xVec3& ang);
void xEntToString(char*, const xEnt* ent); // UNUSED
void xEntLoadModelFromDisk(xEnt* ent, const char* filename, xMemPool*); // UNUSED
void xEntLoadAnimFromDisk(xEnt* ent, const char* animname, const char* statename, xMemPool* mempool); // UNUSED
xModelInstance* xEntLoadModel(xEnt* ent, RpAtomic* imodel, xModelInstance* reuseInstance);
void xEntLoadAnim(xEnt* ent, const void* data, U32, xMemPool* pool); // UNUSED
void xEntLoadAnimGivenTable(xEnt* ent, xMemPool* pool, xAnimTable* table); // UNUSED
void xEntSetupPipeline(xModelInstance* model);
void xEntSetupPipeline(xSurface* surf, RpAtomic* model);
void xEntRestorePipeline(xModelInstance* model);
void xEntRestorePipeline(xSurface*, RpAtomic* model);
void xEntRender(xEnt* ent);
void DK_renderTranslucent(xEnt*); // UNUSED
void xEntUpdate(xEnt* ent, xScene* sc, F32 dt);
void xEntBeginUpdate(xEnt* ent, xScene*, F32 dt);
void xEntEndUpdate(xEnt* ent, xScene* sc, F32 dt);
void xEntDefaultBoundUpdate(xEnt* ent, xVec3* pos);
void xEntSphereBoundUpdate(xEnt* ent, xVec3* pos); // UNUSED
void xEntDefaultTranslate(xEnt* ent, xVec3* dpos, xMat4x3* dmat);
void xEntMotionToMatrix(xEnt* ent, xEntFrame* frame);
void xEntInitialMatNoscale(xMat4x3* mat, xEnt* ent); // UNUSED
void xEntMove(xEnt* ent, xScene* sc, F32 dt);
void xEntApplyPhysics(xEnt* ent, xScene* sc, F32 dt);
void xEntDebugUpdate();
void xEntCollide(xEnt* ent, xScene* sc, F32 dt);
void xEntBeginCollide(xEnt* ent, xScene*, F32);
void xEntEndCollide(xEnt* ent, xScene* sc, F32 dt);
void xEntCollCheckEnv(xEnt* p, xScene* sc);
void xEntCollCheckByGrid(xEnt* p, xScene* sc, xEntCollCheckCallback hitIt);
void xEntCollCheckNPCsByGrid(xEnt* p, xScene* sc, xEntCollCheckCallback hitIt);
void xEntCollCheckStats(xEnt* p, xScene* sc, xEntCollCheckCallback hitIt); // UNUSED
void xEntCollCheckDyns(xEnt* p, xScene* sc, xEntCollCheckCallback hitIt); // UNUSED
void xEntCollCheckNPCs(xEnt* p, xScene* sc, xEntCollCheckCallback hitIt); // UNUSED
xEnt* xEntCollCheckOneEntNoDepen(xEnt* ent, xScene* sc, void* data);
void xEntCollideFloor(xEnt* p, xScene* sc, F32 dt);
void xEntCollideCeiling(xEnt* p, xScene* sc, F32 dt);
void xEntCollideWalls(xEnt* p, xScene* sc, F32 dt);
void xEntSetMaterialMul(xEnt* ent, F32 r, F32 g, F32 b); // UNUSED
void xEntSetNostepNormAngle(F32 angle); // UNUSED
F32 xEntGetNostepNormAngle(); // UNUSED
xBox* xEntGetAllEntsBox();
#if DEBUG || RELEASE
void xEntDebugInit();
void xEntDebugExit();
void xEntDebugAdd(xEnt* xent);
#endif
void xModCollAddSample(xEnt* xent, S64 elapsed);
void xEntAnimateCollision(xEnt& ent, bool on);
bool xEntValidType(U8 type);
bool xEntTurnToFace(xEnt* ent, const xVec3* target, F32 speedLimit, F32 dt);
void xEntReposition(xEnt& ent, const xMat4x3& mat); // UNUSED
void xEntOpacityInit(xEntOpacity* opacity, xEnt* ent);
void xEntOpacityUpdate(xEntOpacity* opacity, xEnt* ent, F32 dt);
void xEntInitShadow(xEnt& ent, xEntShadow& shadow); // UNUSED
void xEntLoadCheckPoint();
void xEntStoreCheckPoint();

inline void xEntShow(xEnt* ent)
{
    ent->flags |= k_XENT_IS_VISIBLE;
    if (ent->visUpdate) {
        ent->visUpdate(ent);
    }
}

inline void xEntHide(xEnt* ent)
{
    ent->flags &= (U8)~k_XENT_IS_VISIBLE;
    if (ent->visUpdate) {
        ent->visUpdate(ent);
    }
}

extern S32 xent_entent;
#if DEBUG || RELEASE
extern S32 xent_nqcrejects;
extern S32 gDrawInvisibleWalls;
extern S32 gEnt_ShowBone;
extern S32 gEnt_ShowVertex;
extern S32 gEnt_ActiveModel;
extern xVec3 gEnt_ShowBoneLoc;
extern xVec3 gEnt_ShowVertexLoc;

void on_select_debug_me(const tweak_info& info);
void on_change_active_model(const tweak_info& info);
void on_change_show_vertex(const tweak_info& info);
#endif

inline bool xEntMoves(const xEnt& ent)
{
    return ent.pflags & k_XENT_IS_MOVING || ent.collType & (k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC) ||
           ent.moreFlags & 0x108;
}

inline U32 xEntIsVisible(const xEnt* ent)
{
#if DEBUG || RELEASE
    if (gDrawInvisibleWalls) {
        return TRUE;
    }
#endif

    return (ent->flags & (k_XENT_IS_VISIBLE | 0x4 | 0x80)) == k_XENT_IS_VISIBLE;
}

#endif
