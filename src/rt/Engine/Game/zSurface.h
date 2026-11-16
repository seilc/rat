#ifndef ZSURFACE_H
#define ZSURFACE_H

#include "xVec3.h"
#include "zSurfAsset.h"

struct xBase;
struct xCollis;
struct RwTexture;
struct xSurface;

struct zSurfacePropTexAnim
{
    U16 mode;
    F32 speed;
    F32 frame;
    U32 group;
    U32 group_idx;
    xBase* group_ptr;
    RwTexture** txtr_animList;
};

struct zSurfacePropUVFX
{
    S32 mode;
    F32 rot;
    F32 rot_spd;
    F32 minmax_timer[2];
    xVec3 trans;
    xVec3 trans_spd;
    xVec3 scale;
    xVec3 scale_spd;
    xVec3 min;
    xVec3 max;
    xVec3 minmax_spd;
};

// zSurfaceProps::texanim_flags
#define SURF_TEXANIM_ON 0x1
#define SURF_TEXANIM_ON2 0x2

// zSurfaceProps::uvfx_flags
#define UVANIM_FLAG_ON 0x1
#define UVANIM_FLAG_ON2 0x2

struct zSurfaceProps
{
    zSurfAssetIN* asset;
    U32 texanim_flags;
    zSurfacePropTexAnim texanim[2];
    U32 uvfx_flags;
    zSurfacePropUVFX uvfx[2];
    RwTexture* txtr_matFXDualMap;
};

xSurface* zSurfaceGetSurface(U32 mat_id);
xSurface* zSurfaceGetSurface(const xCollis* coll);
U32 zSurfaceGetStandOn(const xSurface* surf);

#endif
