#include "xEnt.h"
#include "xMemMgr.h"
#include "xDebug.h"
#include "xMath3.h"
#include "xEvent.h"
#include "xutil.h"
#include "xGrid.h"
#include "zBase.h"
#include "zEvent.h"
#include "zDestructible.h"
#include "zPlayer.h"
#include "xGlobals.h"
#include "xSurface.h"
#include "zSurface.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zCollGeom.h"
#include "xCollide.h"
#include "xstransvc.h"
#include "xString.h"
#include "xpkrsvc.h"
#include "xMath.h"
#include "xserializer.h"
#include "xLinkAsset.h"
#include "xFX.h"
#include "xGroup.h"
#include "iModel.h"
#include "xDraw.h"
#include "xColor.h"
#include "xShadow.h"
#include "xFFX.h"
#include "zGrid.h"
#include "iTime.h"
#include "zPad.h"
#include "xDebugTweak.h"

#include "decomp.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define TURRET_HOLDABLE_DEATH_SIMP_ID 0x3159B667 // xStrHash("TURRET_HOLDABLE_DEATH_SIMP")
#define S08_TURRET_HOLDABLE_DEATH_SIMP_ID 0x36A1171F // xStrHash("S08_TURRET_HOLDABLE_DEATH_SIMP")
#define LID_DD_MINF_ID 0xE71A065E // xStrHash("lid_dd.MINF")
#define BRIDGE_DD_DEST_ID 0xAC1BEE23 // xStrHash("bridge_dd_dest")
#define PLATFORM_RISE_B1_MINF_ID 0x1B80E759 // xStrHash("platform_rise_b1.MINF")
#define FLOOR_ICE_BSP01_ID 0x1E373895 // xStrHash("floor_ice_bsp01")
#define FLOOR_ICE_BSP02_ID 0x1E373896 // xStrHash("floor_ice_bsp02")
#define FLOOR_ICE_BSP03_ID 0x1E373897 // xStrHash("floor_ice_bsp03")
#define GRATE_CIRCULAR_ROOM_BB_ID 0x216EE348 // xStrHash("grate_circular_room_bb")
#define GRATE_ROTATING_TOWER_BB_ID 0x64F7DEF5 // xStrHash("grate_rotating_tower_bb")
#define FLOOR_ICE_BREAK_BB_MINF_ID 0xB3BBCA7F // xStrHash("floor_ice_break_bb.MINF")
#define BB01_BRIDGE_DESTRUCTABLE_ID 0xF07B67B4 // xStrHash("bb01_bridge_destructable")
#define BB01_FROZONE_BRIDGE_TEMP_ID 0x634D1A8C // xStrHash("bb01_frozone_bridge_temp")
#define PLATFORM_GRILL_LARGE_MM_ID 0x9C3DF2C1 // xStrHash("platform_grill_large_mm")
#define PLATFORM_GRILL_SMALL_MM_ID 0xFD512D69 // xStrHash("platform_grill_small_mm")
#define SNOW_DRIFT_MM_ID 0x86770A6E // xStrHash("snow_drift_mm")
#define BRIDGE_METAL_RF_ID 0xF72C1A2A // xStrHash("bridge_metal_rf")
#define PLATFORM_METAL_RF_ID 0x39BA103C // xStrHash("platform_metal_rf")
#define PLATFORM_METAL_BEND_RF_ID 0xBFFC0A5E // xStrHash("platform_metal_bend_rf")
#define PLATFORM_METAL_BEND_B_RF_ID 0xB474D3BB // xStrHash("platform_metal_bend_b_rf")
#define PLATFORM_METAL_BATTERY_RF_ID 0x97B0D79E // xStrHash("platform_metal_battery_rf")
#define ARM_COLLAPSE_RF_ID 0x37B476EF // xStrHash("arm_collapse_rf")
#define ELEVATOR_PLATFORM_UW_ID 0xF3E92A2D // xStrHash("elevator_platform_UW")
#define PLATFORM_GOO_MESH_UW_ID 0xABAD2140 // xStrHash("platform_goo_mesh_UW")
#define ELEVATOR_ROTATE_COVERED_UW_ID 0xE683650E // xStrHash("elevator_rotate_covered_UW")
#define ELEVATOR_ROTATE_UW_ID 0x2F70CD39 // xStrHash("elevator_rotate_UW")
#define GROUND_MESH_A3_ID 0xD34FF538 // xStrHash("ground_mesh_a3")
#define FLOOR_LIVE_A2_ID 0x8E5A52C5 // xStrHash("Floor_Live_A2")

#define ANIM_COLL_SETUP_L(line)                                                                   \
    xASSERT_L(line, ent.model != 0);                                                              \
    xASSERT_L(line, ent.model->Mat != 0);                                                         \
    xASSERT_L(line, ent.moreFlags & k_MORE_FLAGS_ANIM_COLL);                                      \
    xASSERT_L(line, ent.anim_coll != 0);                                                          \
    xModelInstance& model = ent.collModel ? *ent.collModel : *ent.model;                          \
    xMat4x3& mat = *(xMat4x3*)model.Mat;                                                          \
    xEnt::anim_coll_data& ac = *ent.anim_coll;

#define ANIM_COLL_SETUP() ANIM_COLL_SETUP_L(__LINE__)

char* g_xEnt_FMT = "";
static F32 nsn_angle = PI / 6.0f;
static xBox all_ents_box;
static S32 all_ents_box_init;

#if DEBUG || RELEASE
#define MAX_DBG_XENTS 2500

static xEnt** dbg_xents;
static U16 dbg_num;
static S16 dbg_idx = -1;
static S32 dbg_mode;
static S32 dbg_holding_inc;
static S32 dbg_holding_dec;
static F32 dbg_tmr;
static F32 dbg_ltm;

struct xModCollRec
{
    xEnt* owner;
    U32 nusing;
    S64 ctime;
    S64 ltime;
    F32 frac;
};

static xModCollRec* mcrecs;
static S32 num_mcrecs;
static S64 mctime;
#define MAX_MCRECS 150

static xModCollRec smcrecs[MAX_MCRECS];

enum eModCollMode
{
    eModCollMode_None,
    eModCollMode_CTime,
    eModCollMode_LTime
};

static eModCollMode mcmode;

static void xEntDebugCB();
static void xEntDebugWrite(const xEnt* xent);
static void xEntDebugDraw(const xEnt* xent);
static void xEntDebugIPad(xEnt* xent);
static void xModCollInit();
static void xModCollExit();
static void xModCollAdd(xEnt* xent);
#endif

namespace
{
    inline void HackHoldableTurret(xEnt* ent)
    {
        if (ent->id == TURRET_HOLDABLE_DEATH_SIMP_ID || ent->id == S08_TURRET_HOLDABLE_DEATH_SIMP_ID) {
            ent->moreFlags &= (U16)~k_MORE_FLAGS_ANIM_COLL;
            ent->collLev = k_XENT_COLLLEV_OBB;
            ent->bound.box.box.upper.assign(1.6f, 8.0f, 1.3f);
            ent->bound.box.box.lower.assign(-1.6f, 0.0f, -1.3f);
        }
    }

    namespace anim_coll
    {
        void reset(xEnt& ent)
        {
            if (!ent.anim_coll) {
                ent.anim_coll = (xEnt::anim_coll_data*)xMEMALLOC_L(sizeof(xEnt::anim_coll_data), 0, eMemMgrTag_Anim, 0, 246);
                ent.anim_coll->flags = 0;
                ent.anim_coll->verts = NULL;
            }

            ANIM_COLL_SETUP_L(250);

            if (!(ac.flags & 0x8)) {
                switch (model.BoneCount) {
                case 0:
                    xFAIL_FMT_L(257, "Model with zero bones can't be animated. Asset: 0x%x", ent.asset->id);
                    break;
                case 1:
                    ac.flags |= 0x1;
                    ac.old_mat = mat;
                    ac.new_mat = g_I3;
                    break;
                default:
                    ac.flags |= 0x2;
                    ac.old_mat = mat;
                    ac.new_mat = g_I3;

                    xModelAnimCollStart(model);

                    xBox& box = ent.bound.box.box;
                    xVec3 size;
                    size.Sub(box.upper, box.lower);
                    F32 max_size = size.x;
                    if (max_size < size.y) max_size = size.y;
                    if (max_size < size.z) max_size = size.z;
                    max_size += 1.0f;
                    box.upper += max_size;
                    box.lower -= max_size;
                    model.Data->boundingSphere.radius *= 3.0f;
                }
            }
        }

        void refresh(xEnt& ent)
        {
            ANIM_COLL_SETUP_L(313);
            xASSERT_L(314, model.BoneCount > 0);
            xMat4x3& bone_mat = ((xMat4x3*)model.Mat)[1];
            xMat4x3Mul(&mat, &bone_mat, &ac.old_mat);
            ac.new_mat = bone_mat;
            bone_mat = g_I3;
        }

        inline void pre_move(xEnt& ent)
        {
            ANIM_COLL_SETUP_L(323);
            xASSERT_L(324, model.BoneCount > 0);
            ent.frame->mat = mat = ac.old_mat;
        }

        inline void post_move(xEnt& ent)
        {
            ANIM_COLL_SETUP_L(330);
            xASSERT_L(331, model.BoneCount > 0);
            ac.old_mat = ent.frame->mat;
            xMat4x3Mul(&mat, &ac.new_mat, &ac.old_mat);
            ent.frame->mat = mat;
        }
    }
}

void xEntSceneInit()
{
#if DEBUG || RELEASE
    xEntDebugInit();
#endif

    all_ents_box_init = TRUE;
}

void xEntSceneExit()
{
#if DEBUG || RELEASE
    xEntDebugExit();
#endif
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("ent", "xEntInit: deprecated!\n")
#endif

static void xEntAddHittableFlag(xEnt* ent)
{
    if (ent->baseType == eBaseTypeNPCEntity || ent->baseType == eBaseTypeNPC || ent->baseType == eBaseTypeDestructObj ||
        ent->baseType == eBaseTypeButton || ent->baseType == eBaseTypeBoulder ||
        (ent->baseType == eBaseTypePlatform && ent->subType == 12)) {
        ent->moreFlags |= 0x10;
        return;
    }

    for (U32 i = 0; i < ent->linkCount; i++) {
        if (ent->link[i].srcEvent == eEventHit || ent->link[i].srcEvent == eEventHit_Melee ||
            ent->link[i].srcEvent == eEventHit_BubbleBounce || ent->link[i].srcEvent == eEventHit_BubbleBash ||
            ent->link[i].srcEvent == eEventHit_BubbleBowl || ent->link[i].srcEvent == eEventHit_Cruise ||
            ent->link[i].srcEvent == eEventHit_PatrickSlam || ent->link[i].srcEvent == eEventHit_Throw ||
            ent->link[i].srcEvent == eEventHit_PaddleLeft || ent->link[i].srcEvent == eEventHit_PaddleRight ||
            ent->link[i].srcEvent == UNUSED_eEventHit_Cartwheel) {
            ent->moreFlags |= 0x10;
            return;
        }
    }
}

static void hack_receive_shadow(xEnt* ent)
{
    static U32 receive_models[29] = {
        LID_DD_MINF_ID,
        BRIDGE_DD_DEST_ID,
        PLATFORM_RISE_B1_MINF_ID,
        0x93582686,
        0xAC696C27,
        FLOOR_ICE_BSP01_ID,
        FLOOR_ICE_BSP02_ID,
        FLOOR_ICE_BSP03_ID,
        GRATE_CIRCULAR_ROOM_BB_ID,
        GRATE_ROTATING_TOWER_BB_ID,
        FLOOR_ICE_BREAK_BB_MINF_ID,
        0x557CB1CA,
        BB01_BRIDGE_DESTRUCTABLE_ID,
        BB01_FROZONE_BRIDGE_TEMP_ID,
        PLATFORM_GRILL_LARGE_MM_ID,
        PLATFORM_GRILL_SMALL_MM_ID,
        SNOW_DRIFT_MM_ID,
        BRIDGE_METAL_RF_ID,
        PLATFORM_METAL_RF_ID,
        PLATFORM_METAL_BEND_RF_ID,
        PLATFORM_METAL_BEND_B_RF_ID,
        PLATFORM_METAL_BATTERY_RF_ID,
        ARM_COLLAPSE_RF_ID,
        ELEVATOR_PLATFORM_UW_ID,
        PLATFORM_GOO_MESH_UW_ID,
        ELEVATOR_ROTATE_COVERED_UW_ID,
        ELEVATOR_ROTATE_UW_ID,
        GROUND_MESH_A3_ID,
        FLOOR_LIVE_A2_ID,
    };

    const U32* end_receive_models = receive_models + ARRAY_LENGTH(receive_models);
    U32 modelID = ent->asset->modelInfoID;

    for (U32* id = receive_models; id != end_receive_models; id++) {
        if (modelID == *id) {
            ent->baseFlags |= k_XBASE_RECEIVES_SHADOWS;
            ent->asset->baseFlags |= k_XBASE_RECEIVES_SHADOWS;
            break;
        }
    }
}

static void xEntAddShadowRecFlag(xEnt* ent)
{
    switch (ent->baseType) {
    case eBaseTypePlatform:
    case eBaseTypeStatic:
    case eBaseTypeButton:
    case eBaseTypeDestructObj:
    case eBaseTypeNPC:
    case eBaseTypeBoulder:
    case eBaseTypeNPCEntity:
        if (ent->model && (ent->model->Pipe.Flags & (eSRCBLEND_MASK | eDESTBLEND_MASK))) {
            ent->baseFlags &= (U16)~k_XBASE_RECEIVES_SHADOWS;
        }
        break;
    default:
        ent->baseFlags &= (U16)~k_XBASE_RECEIVES_SHADOWS;
    }

    hack_receive_shadow(ent);
}

void xEntInit(xEnt* ent, xEntAsset* asset)
{
    xASSERT_L(659, ent);
    xASSERT_L(660, asset);

    xBaseInit(ent, asset);

    ent->asset = asset;
    ent->update = xEntUpdate;
    ent->bupdate = xEntDefaultBoundUpdate;
    ent->render = xEntRender;
    ent->move = NULL;
    ent->transl = xEntDefaultTranslate;
    ent->flags = asset->flags;
    ent->miscflags = 0;
    ent->moreFlags = asset->moreFlags;
    ent->subType = asset->subtype;
    ent->pflags = asset->pflags;
    ent->ffx = NULL;
    ent->num_ffx = 0;
    ent->driver = NULL;
    ent->driven = NULL;
    ent->model = NULL;
    ent->collModel = NULL;
    ent->camcollModel = NULL;
    ent->frame = NULL;
    ent->collis = NULL;
    ent->simpShadow = NULL;
    ent->entShadow = NULL;
    ent->baseFlags |= k_XBASE_IS_ENTITY;

    xGridBoundInit(&ent->gridb, ent);

    ent->anim_coll = NULL;

    xEntGrowGlobalBoundingBox(&asset->pos);
}

void xEntGrowGlobalBoundingBox(const xVec3* pos)
{
    if (all_ents_box_init) {
        iBoxInitBoundVec(&all_ents_box, pos);
        all_ents_box_init = FALSE;
    } else {
        iBoxBoundVec(&all_ents_box, &all_ents_box, pos);
    }
}

void xEntInitForType(xEnt* ent)
{
    ent->update = xEntUpdate;
    ent->render = xEntRender;

    switch (ent->collType) {
    case k_XENT_COLLTYPE_TRIG:
        ent->pflags &= ~(k_XENT_IS_MOVING | k_XENT_HAS_VELOCITY);
        ent->chkby = k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC;
        ent->penby = 0;
        break;
    case k_XENT_COLLTYPE_STAT:
        ent->pflags &= ~(k_XENT_IS_MOVING | k_XENT_HAS_VELOCITY);
        ent->chkby = k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC;
        ent->penby = k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC;
        break;
    case k_XENT_COLLTYPE_DYN:
        ent->pflags |= k_XENT_IS_MOVING;
        ent->move = NULL;
        ent->frame = (xEntFrame*)xMEMALLOC_L(sizeof(xEntFrame), 0, eMemMgrTag_Anim, 0, 763);
        memset(ent->frame, 0, sizeof(xEntFrame));
        ent->pflags &= (U8)~k_XENT_HAS_VELOCITY;
        ent->chkby = k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC;
        ent->penby = k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC;
        break;
    case k_XENT_COLLTYPE_NPC:
        ent->pflags |= k_XENT_IS_MOVING;
        ent->move = NULL;
        ent->pflags |= k_XENT_HAS_VELOCITY;
        ent->chkby = k_XENT_COLLTYPE_PC;
        ent->penby = k_XENT_COLLTYPE_PC;
        break;
    case k_XENT_COLLTYPE_PC:
        ent->pflags |= k_XENT_IS_MOVING;
        ent->move = NULL;
        ent->frame = (xEntFrame*)xMEMALLOC_L(sizeof(xEntFrame), 0, eMemMgrTag_Player, 0, 815);
        memset(ent->frame, 0, sizeof(xEntFrame));
        ent->pflags |= k_XENT_HAS_VELOCITY;
        ent->chkby = 0;
        ent->penby = 0;
        ent->collis = (xEntCollis*)xMEMALLOC_L(sizeof(xEntCollis), 0, eMemMgrTag_Player, 0, 823);
        ent->collis->chk = k_XENT_COLLTYPE_TRIG | k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_ENV;
        ent->collis->pen = k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_ENV;
        ent->collis->post = NULL;
        ent->collis->depenq = NULL;
        break;
    }

    if ((ent->moreFlags & k_MORE_FLAGS_ANIM_COLL || ent->flags & 0x2) && !ent->frame) {
        ent->frame = (xEntFrame*)xMEMALLOC_L(sizeof(xEntFrame), 0, eMemMgrTag_Anim, 0, 845);
        memset(ent->frame, 0, sizeof(xEntFrame));
    }

#if DEBUG || RELEASE
    if (ent->bound.type) {
        xEntDebugAdd(ent);
    }
#endif

    ent->baseFlags |= k_XBASE_IS_ENTITY;
}

namespace
{
    F32 get_lower_extent(const xBound& bound)
    {
        switch (bound.type) {
        case k_XBOUNDTYPE_SPHERE:
            return bound.sph.r;
        case k_XBOUNDTYPE_BOX:
            return bound.box.center.y - bound.box.box.lower.y;
        case k_XBOUNDTYPE_OBB:
        {
            xMat4x3& mat = *bound.mat;
            if (0.0f == mat.up.x && 0.0f == mat.up.z) {
                F32 y = mat.up.y * bound.box.box.lower.y + mat.pos.y;
                return bound.box.center.y - y;
            }

            xBox abox;
            xBoundGetBox(abox, bound);
            return bound.box.center.y - abox.lower.y;
        }
        default:
            xASSERT_L(914, false &&"Unsupported bound type.");
        }

        return 0.0f;
    }

    bool collide_downward(xVec3& loc, xEnt*& hit, xScene& s, xEnt& ent, F32 max_dist)
    {
        F32 lower_bound_extent = get_lower_extent(ent.bound);
        xVec3& center = *xBoundCenter(&ent.bound);

        xRay3 ray;
        ray.origin = center;
        ray.dir.assign(0.0f, -1.0f, 0.0f);
        ray.min_t = 0.0f;
        ray.max_t = max_dist + lower_bound_extent;
        ray.flags = 0xc00;

        U8 old_bound_type = ent.bound.type;
        F32 old_bound_radius = ent.bound.sph.r;
        ent.bound.type = k_XBOUNDTYPE_SPHERE;
        ent.bound.sph.r = 0.0f;

        xCollis coll;
        center.y = 1000.0f;
        coll.flags = 0x100;
        xRayHitsScene(&s, &ray, &coll);
        ent.bound.type = old_bound_type;
        ent.bound.sph.r = old_bound_radius;
        center.y = ray.origin.y;

        if (coll.flags & 0x1) {
            loc = ((xMat4x3*)ent.model->Mat)->pos;
            loc.y -= coll.dist - lower_bound_extent;
            hit = (xEnt*)coll.optr;
            return true;
        }

        xSphere* spheres[5];
        S32 count = xglobals->players[0]->GetCollisionSpheres((const xSphere**)spheres);
        S32 i;
        for (i = 0; i < count; i++) {
            xBound b;
            xBoundFromSphere(b, *spheres[i]);
            xBoundHitsBound(&ent.bound, &b, &coll);
            if (coll.flags & 0x1) {
                break;
            }
        }

        if (i != count) {
            xASSERT_FMT_L(979, zDestructibleGetFromEntity(&ent), "The stacked item \"%s\" fell and hit the player, but it is not destructible, so it will fall through the player. Please make it destructible", xSceneID2Name(xglobals->sceneCur, ent.id));
            zEntEvent(&ent, eEventHit, 1000000.0f, 0.0f, 0.0f, 0.0f);
            loc = ((xMat4x3*)ent.model->Mat)->pos;
            loc.y -= coll.dist - lower_bound_extent;
            hit = xglobals->players[0];
            return true;
        }

        return false;
    }

    void drop_stacked_entity(xEnt& ent);

    void stacked_owner_destroyed(zDestructible*, void* context)
    {
        xASSERT_L(996, context != 0);
        drop_stacked_entity(*(xEnt*)context);
    }

    void mount_stacked_entity(xEnt& ent, xEnt& driver)
    {
        if (ent.driver)
            ent.driver->driving_count--;
        ent.driver = &driver;
        driver.driving_count++;

        driver.driven = &ent;
        if (driver.pflags & k_XENT_HAS_GRAVITY) {
            driver.driven->pflags = k_XENT_HAS_GRAVITY;
        }

        zDestructible* durst = zDestructibleGetFromEntity(&driver);
        if (durst) {
            durst->destroy_notify = stacked_owner_destroyed;
            durst->notify_context = &ent;
        }
    }

    void drop_driven_stacked_entity(xEnt& ent)
    {
        if (ent.driven && ent.driven->driver) {
            xASSERT_L(1027, ent.driven->driver->driving_count);
            ent.driven->driver->driving_count--;
            ent.driven->driver = NULL;
        }

        if (ent.driven) {
            ent.driven->pflags = k_XENT_HAS_GRAVITY;
            if (ent.driven->driver)
                drop_driven_stacked_entity(*ent.driven->driver);
        }
    }

    void dismount_stacked_entity(xEnt& ent)
    {
        if (!ent.driver) {
            return;
        }

        zDestructible* durst = zDestructibleGetFromEntity(ent.driver);
        if (durst) {
            durst->destroy_notify = NULL;
            durst->notify_context = NULL;
        }

        if (ent.driver->driving_count) {
            --ent.driver->driving_count;
        }

        ent.driver = NULL;

        drop_driven_stacked_entity(ent);
    }

    void setup_stacked_entity(xEnt& ent)
    {
        ent.pflags = k_XENT_HAS_GRAVITY;
    }

    void drop_stacked_entity(xEnt& ent)
    {
        ent.pflags = k_XENT_HAS_GRAVITY;
        dismount_stacked_entity(ent);
    }

    void stop_stacked_entity(xEnt& ent)
    {
        ent.pflags = 0;
    }

    void update_stacked_entity(xScene& s, xEnt& ent, F32 dt)
    {
        if (ent.driver && (ent.driver->pflags & k_XENT_HAS_GRAVITY)) {
            return;
        }

        xEntApplyPhysics(&ent, &s, dt);
        xEntMove(&ent, &s, dt);
        F32 dist = ((xMat4x3*)ent.model->Mat)->pos.y - ent.frame->mat.pos.y;
        if (dist <= 0.0f) {
            return;
        }

        xVec3 hitloc;
        xEnt* hitent;
        if (!collide_downward(hitloc, hitent, s, ent, dist)) {
            return;
        }

        xVec3 initialObjEuler;
        xMat4x3* objMat = (xMat4x3*)ent.model->Mat;
        xMat3x3GetEuler(objMat, &initialObjEuler);

        xVec3 destObjEuler;
        destObjEuler.x = initialObjEuler.x;
        if (initialObjEuler.y > PI)
            destObjEuler.y = 2.0f * PI;
        else if (initialObjEuler.y < -PI)
            destObjEuler.y = -2.0f * PI;
        else
            destObjEuler.y = 0.0f;

        if (initialObjEuler.z > PI)
            destObjEuler.z = 2.0f * PI;
        else if (initialObjEuler.z < -PI)
            destObjEuler.z = -2.0f * PI;
        else
            destObjEuler.z = 0.0f;

        xMat3x3Euler(objMat, &destObjEuler);
        ent.frame->mat = *objMat;

        ent.frame->mat.pos.y = hitloc.y;
        stop_stacked_entity(ent);
        if (hitent)
            mount_stacked_entity(ent, *hitent);
    }
}

void xEntDrop(xEnt* ent)
{
    drop_stacked_entity(*ent);
}

void xEntSetup(xEnt* ent)
{
    xASSERT_L(1156, ent);
    xASSERT_L(1157, ent->asset);

    xBaseSetup(ent);

    ent->baseFlags |= k_XBASE_IS_ENTITY;

    if (ent->asset->surfaceID) {
        xSurface* surf = (xSurface*)xSceneResolvID(globals.sceneCur, ent->asset->surfaceID);
        if (surf) {
            surf->type = 1;
            surf->ent = ent;

            zSurfaceProps* pp = (zSurfaceProps*)surf->moprops;
            xASSERT_FMT_L(1175, !((ent->baseFlags & k_XBASE_IS_NPC) && (pp->texanim_flags & (SURF_TEXANIM_ON | SURF_TEXANIM_ON2))), "NPC surface has texture animation");
            xASSERT_FMT_L(1176, !((ent->baseFlags & k_XBASE_IS_NPC) && (pp->uvfx_flags & (UVANIM_FLAG_ON | UVANIM_FLAG_ON2 ))), "NPC surface has UV animation");

            xModelInstance* minst = ent->model;
            if (minst->Next) {
                while (minst) {
                    if (minst->Pipe.Flags & eMATFX_MASK) {
                        minst->Surf = surf;
                    }
                    minst = minst->Next;
                }
            } else {
                minst->Surf = surf;
            }
        }
    }

    xEntDrivenBySetup(ent);

    if (ent->model) {
        ent->model->RedMultiplier = ent->asset->redMult;
        ent->model->GreenMultiplier = ent->asset->greenMult;
        ent->model->BlueMultiplier = ent->asset->blueMult;
        ent->model->Alpha = ent->asset->seeThru;
    }

    xEntAddHittableFlag(ent);
    xEntAddShadowRecFlag(ent);

    zCollGeom_EntSetup(ent);

#if DEBUG || RELEASE
    if (ent->collLev == k_XENT_COLLLEV_MODEL) {
        xModelInstance* collmod = ent->collModel ? ent->collModel : ent->model;
        if (!collmod->Data->geometry->triangles) {
            PKRAssetTOCInfo ainfo;
            S32 i, numModel;
            const char* assetname;
            memset(&ainfo, 0, sizeof(ainfo));
            numModel = xSTAssetCountByType('MODL');
            for (i = 0; i < numModel; i++) {
                xSTGetAssetInfoByType('MODL', i, &ainfo);
                if (ainfo.mempos == collmod->Data) {
                    break;
                }
            }
            xASSERT_L(1245, i != numModel);
            assetname = ainfo.assetname;
        }

        if ((xabs(ent->asset->scale.y - ent->asset->scale.x) > 0.001f || xabs(ent->asset->scale.z - ent->asset->scale.x) > 0.001f) && ent->chkby) {
            xFAIL_FMT_L(1270, "Non-uniform scale on model with collision--FATAL ERROR\nEntity:  %s\n", xSTAssetNameOrID(ent->id));
        }
    }
#endif

    if (ent->model) {
        if (ent->bound.type == k_XBOUNDTYPE_BOX) {
            xBoxForModel(ent->bound.box.box, ent->collModel ? ent->collModel : ent->model, true);
        } else if (ent->bound.type == k_XBOUNDTYPE_OBB) {
            xBoxForModelLocal(ent->bound.box.box, ent->collModel ? ent->collModel : ent->model, true);
        }
    }

    if (ent->moreFlags & k_MORE_FLAGS_ANIM_COLL) {
        anim_coll::reset(*ent);
    }

    HackHoldableTurret(ent);

    if (ent->flags & 0x2) {
        setup_stacked_entity(*ent);
    }
}

void xEntDrivenBySetup(xEnt* ent)
{
    for (S32 i = 0; i < ent->linkCount; i++) {
        const xLinkAsset* la = &ent->link[i];
        if (la->dstEvent == eEventDrivenby) {
            xEnt* dent = (xEnt*)xSceneResolvID(globals.sceneCur, la->dstAssetID);
            xVALIDATE_L(1306, dent);
            if (dent) {
                xASSERT_L(1310, dent->frame);
                xVALIDATE_L(1311, !ent->driver);
                ent->driver = dent;
                if ((S32)la->param[0]) {
                    ent->driveInfo = (xEntDriveInfo*)xMEMALLOC_L(sizeof(xEntDriveInfo), 0, eMemMgrTag_Anim, 0, 1315);

                    xMat4x3 invMat;
                    RwMatrixUpdate((RwMatrix*)&dent->frame->mat);
                    RwMatrixInvert((RwMatrix*)&invMat, (RwMatrix*)&dent->frame->mat);
                    if (ent->model) {
                        xMat4x3Mul(&ent->driveInfo->driveMat, (xMat4x3*)ent->model->Mat, &invMat);
                    }

                    xMat3x3RMulVec(&ent->driveInfo->centerOffset, &invMat, xEntGetCenter(ent));
                    xVec3AddTo(&ent->driveInfo->centerOffset, &invMat.pos);
                    ent->driveInfo->flags = 1;
                } else {
                    ent->driveInfo = NULL;
                }

                dent->driving_count++;
            }
        }
    }
}

void xEntDrivenByReset(xEnt* ent)
{
    for (S32 i = 0; i < ent->linkCount; i++) {
        const xLinkAsset* la = &ent->link[i];
        if (la->dstEvent == eEventDrivenby) {
            xEnt* dent = (xEnt*)xSceneResolvID(globals.sceneCur, la->dstAssetID);
            xVALIDATE_L(1359, dent);
            if (dent) {
                if (!ent->driver) {
                    dent->driving_count++;
                }

                ent->driver = dent;
                if (ent->driveInfo) {
                    ent->driveInfo->flags = 1;
                }
            }
        }
    }
}

void xEntSave(xEnt* ent, xSerial* s)
{
    xBaseSave(ent, s);

    xASSERT_L(1394, ent);
    xASSERT_L(1395, s);

    if (ent->flags & k_XENT_IS_VISIBLE)
        s->Write_b1(1);
    else
        s->Write_b1(0);
}

void xEntLoad(xEnt* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    xASSERT_L(1413, ent);
    xASSERT_L(1414, s);

    S32 b = 0;
    s->Read_b1(&b);
    if (b)
        xEntShow(ent);
    else
        xEntHide(ent);
}

void xEntReset(xEnt* ent)
{
    xEntReset(ent, ent->asset->pos, ent->asset->ang);
}

void xEntReset(xEnt* ent, const xVec3& pos, const xVec3& ang)
{
    xASSERT_L(1437, ent);
    xASSERT_L(1438, ent->asset);

    xBaseReset(ent, ent->asset);

    ent->baseFlags |= k_XBASE_IS_ENTITY;
    ent->flags &= (U8)0x4;
    ent->flags |= (U8)(ent->asset->flags & ~0x4);
    ent->miscflags = 0;
    ent->moreFlags = (ent->moreFlags & 0xff00) | ent->asset->moreFlags;

    xEntAddHittableFlag(ent);
    xEntAddShadowRecFlag(ent);

    xFIX_NAN_ELEM_L(1460, ((xVec3&)ang).x);
    xFIX_NAN_ELEM_L(1461, ((xVec3&)ang).y);
    xFIX_NAN_ELEM_L(1462, ((xVec3&)ang).z);

    xMat4x3 frame;
    if (!ent->driver || !ent->driver->frame || !ent->driveInfo || gInSceneResetOrInit) {
        xMat3x3Euler(&frame, ang.x, ang.y, ang.z);

        xVALIDATE_L(1482, xVec3Dot(&ent->asset->scale,&ent->asset->scale) > 0.0f);
        xVec3SMulBy(&frame.right, ent->asset->scale.x);
        xVec3SMulBy(&frame.up, ent->asset->scale.y);
        xVec3SMulBy(&frame.at, ent->asset->scale.z);

        xVec3Copy(&frame.pos, &pos);
    } else {
        xMat4x3Mul(&frame, &ent->driveInfo->driveMat, &ent->driver->frame->mat);
        if (ent->driver->driving_reset) {
            ent->driveInfo->flags |= 0x4;
        }
    }

    frame.flags = 0;

    if (ent->model) {
        xModelSetFrame(ent->model, &frame);

        if (ent->collModel) {
            xModelSetFrame(ent->collModel, &frame);
        }

        if (ent->bound.type == k_XBOUNDTYPE_BOX) {
            xBoxForModel(ent->bound.box.box, ent->collModel ? ent->collModel : ent->model, true);
        } else if (ent->bound.type == k_XBOUNDTYPE_OBB) {
            xBoxForModelLocal(ent->bound.box.box, ent->collModel ? ent->collModel : ent->model, true);
        }

        if (ent->moreFlags & k_MORE_FLAGS_ANIM_COLL) {
            anim_coll::reset(*ent);
        }

        HackHoldableTurret(ent);

        xModelInstance* minst = ent->model;
        while (minst) {
            minst->RedMultiplier = ent->asset->redMult;
            minst->GreenMultiplier = ent->asset->greenMult;
            minst->BlueMultiplier = ent->asset->blueMult;
            minst->Alpha = minst->Data->geometry->matList.materials[0]->color.alpha / 255.0f;
            minst->Scale.x = 0.0f;
            minst->Scale.y = 0.0f;
            minst->Scale.z = 0.0f;
            minst = minst->Next;
        }

        minst = ent->model;
        xBox* combinedAnimBound = &minst->combinedAnimBound;
        combinedAnimBound->upper.x = -1e38f;
        while (minst) {
            U16 old_flags = minst->Flags;
            minst->Flags |= 0x1;
            xModelEvalSingle(minst, combinedAnimBound);
            minst->Flags = old_flags;
            minst = minst->Next;
        }
    }

    xEntDrivenByReset(ent);

    if (ent->frame) {
        xMat4x3Copy(&ent->frame->mat, &frame);

        if (!ent->driving_count || gInSceneResetOrInit) {
            ent->frame->oldmat = ent->frame->mat;
        } else {
            ent->driving_reset = 1;
        }

        xVec3Copy(&ent->frame->setDpos(), &g_O3);
        xVec3Copy(&ent->frame->dvel, &g_O3);
        xVec3Copy(&ent->frame->vel, &g_O3);
        xVec3Copy(&ent->frame->oldvel, &g_O3);

        xVec3Copy(&ent->frame->rot.axis, &ent->asset->ang);
        ent->frame->rot.angle = 0.0f;
        xRotCopy(&ent->frame->oldrot, &ent->frame->rot);
    }

    if (ent->bupdate && ent->model) {
        ent->bupdate(ent, &((xMat4x3*)ent->model->Mat)->pos);
    }

    if (ent->flags & 0x2) {
        setup_stacked_entity(*ent);
    }

    if (ent->baseType == eBaseTypeStatic && ent->visUpdate) {
        ent->visUpdate(ent);
    }
}

// UNUSED: needed to match (speculative code)
void xEntLoadModelFromDisk(xEnt* ent, const char* filename, xMemPool*)
{
    char fullfilename[80];
    sprintf(fullfilename, "%s.DFF", filename);

    RpAtomic* imodel;
    U32 modelID = xStrHash(fullfilename);
    imodel = (RpAtomic*)xSTFindAsset(modelID, NULL);

    ent->model = xModelInstanceAlloc(imodel, ent, 0, 0, NULL);
    xASSERT_FMT((ent->model || xModelPoolStressTesting()), "xModelInstanceAlloc fn couldn't allocate pool space for Model %s !", fullfilename);

    while (imodel = iModelFile_RWMultiAtomic(imodel)) {
        xModelInstanceAttach(xModelInstanceAlloc(imodel, ent, 0x8, 0, NULL), ent->model);
    }
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("ent->model", "%s.SKA", "ent-table", "ent->model->Anim")
#endif

xModelInstance* xEntLoadModel(xEnt* ent, RpAtomic* imodel, xModelInstance* reuseInstance)
{
    xASSERT_L(1697, imodel);

    xModelInstance* nextModel = NULL;
    if (reuseInstance) {
        nextModel = reuseInstance->Next;
    }

    xModelInstance* model = xModelInstanceAllocReuse(imodel, ent, 0, 0, NULL, reuseInstance);

    while (imodel = iModelFile_RWMultiAtomic(imodel)) {
        if (nextModel) {
            reuseInstance = nextModel;
            nextModel = reuseInstance->Next;
            xModelInstanceAttach(xModelInstanceAllocReuse(imodel, ent, 0x8, 0, NULL, reuseInstance), model);
        } else {
            xModelInstanceAttach(xModelInstanceAlloc(imodel, ent, 0x8, 0, NULL), model);
        }
    }

    if (ent) {
        ent->model = model;
    }

    xASSERT_L(1726, model || xModelPoolStressTesting());
    return model;
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("idle", "anim-file", "anim-table", "table")
#endif

static void xEntAddToPos(xEnt* ent, const xVec3* dpos)
{
    xVec3AddTo(&ent->frame->mat.pos, dpos);
}

static S32 setMaterialTextureRestore;

inline RwTexture* LocateSurfTexture_TexAnim(zSurfaceProps* surfProp, S32 idx_pass)
{
    RwTexture* txtr = NULL;
    U32 idx_grpItem = surfProp->texanim[idx_pass].group_idx;

#if DEBUG || RELEASE
    static S32 useGroups = FALSE;
#else
    static S32 useGroups;
#endif

#if DEBUG || RELEASE
    if (useGroups) {
        xGroup* grp = (xGroup*)surfProp->texanim[idx_pass].group_ptr;
        xASSERT_L(1806, grp);
        if (!grp) {
            return NULL;
        }

        txtr = (RwTexture*)xGroupGetItemPtr(grp, idx_grpItem);
    } else
#endif
    {
        if (surfProp->texanim[idx_pass].txtr_animList) {
            txtr = surfProp->texanim[idx_pass].txtr_animList[idx_grpItem];
        }
    }

    return txtr;
}

void xEntSetupPipeline(xModelInstance* model)
{
    xEntSetupPipeline(model->Surf, model->Data);
}

void xEntSetupPipeline(xSurface* surf, RpAtomic* model)
{
    S32 setPipeline = FALSE;
    setMaterialTextureRestore = FALSE;
    setPipeline = FALSE;

    if (!surf) return;

    if (!surf->moprops) {
        return;
    }

    zSurfaceProps* pp = (zSurfaceProps*)surf->moprops;

    if (pp->texanim_flags & SURF_TEXANIM_ON) {
        S32 tdx = 0;
        RwTexture* texptr = LocateSurfTexture_TexAnim(pp, tdx);
        if (texptr) {
            iModelSetMaterialTexture(model, texptr);
            setMaterialTextureRestore = TRUE;
        }
    }

    {
        RwTexture* texptr = NULL;
        if (pp->texanim_flags & SURF_TEXANIM_ON2) {
            S32 tdx = 1;
            texptr = LocateSurfTexture_TexAnim(pp, 1);
        }
        else if (pp->uvfx_flags & UVANIM_FLAG_ON2) {
            texptr = pp->txtr_matFXDualMap;
        }

        xFXanimUV2PSetTexture(texptr);
        if (texptr) {
            setPipeline = TRUE;
        }
    }

    if (pp->uvfx_flags & (UVANIM_FLAG_ON | UVANIM_FLAG_ON2)) {
        setPipeline = TRUE;
        xFXanimUVSetTranslation(&pp->uvfx[0].trans);
        xFXanimUV2PSetTranslation(&pp->uvfx[1].trans);

        xFXanimUVSetScale(&pp->uvfx[0].scale);
        xFXanimUV2PSetScale(&pp->uvfx[1].scale);

        xFXanimUVSetAngle((PI / 180.0f) * pp->uvfx[0].rot);
        xFXanimUV2PSetAngle((PI / 180.0f) * pp->uvfx[1].rot);
    }

    if (setPipeline) {
        bool dualPass = (pp->uvfx_flags & UVANIM_FLAG_ON2) ? true : false;
        if (!xFXanimUVAtomicSetup(model, dualPass)) {
#if DEBUG || RELEASE
            if (ReportUVProb) {
                char err[1024];
                if (surf->ent) {
                    sprintf(err, "The surface (%s) is using a model (%s) that isn't specified in the default.pipespec or has incorrect flags for UV animation.\nThis will be only reported once.", xSTAssetName(surf->id), xSTAssetName(surf->ent->model->modelID));
                } else {
                    sprintf(err, "The surface (%s) doesn't have a model ", xSTAssetName(surf->id));
                }

                ReportUVProb = 0;
            }
#endif
        }
    }
}

void xEntRestorePipeline(xModelInstance* model)
{
    xEntRestorePipeline(model->Surf, model->Data);
}

void xEntRestorePipeline(xSurface*, RpAtomic* model)
{
    if (setMaterialTextureRestore) {
        iModelResetMaterial(model);
        setMaterialTextureRestore = FALSE;
    }
}

#if DEBUG || RELEASE
static void xEntRender_DebugInfo(xEnt* ent);

S32 gEnt_ShowBone = -1;
S32 gEnt_ShowVertex = -1;
xEnt* gEnt_Debug;
S32 gEnt_ActiveModel;
xVec3 gEnt_ShowBoneLoc;
xVec3 gEnt_ShowVertexLoc;
#endif

void xEntRender(xEnt* ent)
{
    xASSERT_L(2164, ent);

    if (!ent->model)
        return;
    if (!xEntIsVisible(ent))
        return;

    if (ent->model->Flags & 0x400) {
        return;
    }

    ent->isCulled = 0;

    if (ent->baseType == eBaseTypePlayer || (ent->baseType == eBaseTypeNPCEntity && !(ent->flags & 0x20))) {
        S32 shadowOutside;
        xVec3 shadVec;
        shadVec.x = ((xMat4x3*)ent->model->Mat)->pos.x;
        shadVec.y = ((xMat4x3*)ent->model->Mat)->pos.y - 10.0f;
        shadVec.z = ((xMat4x3*)ent->model->Mat)->pos.z;
        if (xModelCullGroupPlusShadow(ent->model, 10.0f, &shadowOutside)) {
            if (shadowOutside) {
                ent->isCulled = 1;
                return;
            }
            goto shadow;
        }
    } else if (xModelCullGroup(ent->model)) {
        ent->isCulled = 1;
        return;
    }

#if DEBUG || RELEASE
    if (xDebugModeGet() == 4 && smcrecs && smcrecs[0].owner && ent->model->Data == smcrecs[0].owner->model->Data) {
        ent->model->Flags |= 0x200;
    }

    if (gDrawInvisibleWalls && ent->collModel && ent->model != ent->collModel) {
        xModelRender(ent->collModel);
    }
#endif

    xModelRender(ent->model);

#if DEBUG || RELEASE
    xEntRender_DebugInfo(ent);
#endif

shadow:
    if ((ent->baseType == eBaseTypeNPCEntity || ent->baseType == eBaseTypePlayer) && ent->entShadow && !(ent->flags & 0x20)) {
        xShadow_ListAdd(ent);
    }
}

#if DEBUG || RELEASE
static void xEntRender_DebugInfo(xEnt* ent)
{
    if (xDebugModeGet() == 4) {
        ent->model->Flags &= (U16)~0x200;
        xDrawSetColor(g_PIMP_GOLD);
        xBoundDraw(&ent->bound);
    }
    else if (ent->moreFlags & 0x40) {
        xDrawSetColor(g_PIMP_GOLD);
        xBoundDraw(&ent->bound);
    }

    if (xDebugModeGet() == 6 && (ent->miscflags & 0x4)) {
        xDrawSetColor(g_PIMP_GOLD);
        xBoundDraw(&ent->bound);
        xDrawSetColor(g_ORANGE);
    }

    if (ent == gEnt_Debug && (gEnt_ShowBone >= 0 || gEnt_ShowVertex >= 0)) {
        xModelInstance* active_model = ent->model;
        S32 i;
        for (i = 0; active_model && i < gEnt_ActiveModel; i++)
            active_model = active_model->Next;

        if (active_model) {
            if (gEnt_ShowBone >= 0 && gEnt_ShowBone <= ent->model->BoneCount) {
                xModelGetBoneLocation(gEnt_ShowBoneLoc, *active_model, gEnt_ShowBone);
                xDrawSetColor(g_NEON_GREEN);
                xDrawSphere2(&gEnt_ShowBoneLoc, 0.5f, 12);

                xMat4x3 mat_bone;
                xModelGetBoneMat(mat_bone, *active_model, gEnt_ShowBone);
                xDrawAxis(&mat_bone, 1.0f);
                xDrawSetColor(g_MAGENTA);
                xDrawLine(&gEnt_ShowBoneLoc, &((xMat4x3*)xglobals->players[0]->model->Mat)->pos);
            } else
                gEnt_ShowBoneLoc = 0.0f;

            if (gEnt_ShowVertex >= 0 && gEnt_ShowVertex < (S16)iModelVertCount(active_model->Data)) {
                iModelVertEval(active_model->Data, gEnt_ShowVertex, 1, active_model->Mat, NULL, &gEnt_ShowVertexLoc);
                xDrawSetColor(g_RED);
                xDrawSphere2(&gEnt_ShowVertexLoc, 0.5f, 12);
                xDrawSetColor(g_YELLOW);
                xDrawLine(&gEnt_ShowVertexLoc, &((xMat4x3*)xglobals->players[0]->model->Mat)->pos);
            } else
                gEnt_ShowVertexLoc = 0.0f;
        }
    }
}
#endif

void xEntUpdate(xEnt* ent, xScene* sc, F32 dt)
{
    xASSERT_L(2388, ent);
    xASSERT_L(2389, sc);

    xSTATS_BEGIN(t0);

    xEntBeginUpdate(ent, sc, dt);

    if (ent->pflags & k_XENT_HAS_VELOCITY) {
        xEntApplyPhysics(ent, sc, dt);
    }

    if (ent->pflags & k_XENT_IS_MOVING) {
        xEntMove(ent, sc, dt);
    }

    xFFXApply(ent, sc, dt);

    xSTATS_END(t0, ent_update);

    if (ent->collis) {
        xSTATS_BEGIN(tec);
        xEntCollide(ent, sc, dt);
        xSTATS_END(tec, collision);
    }

    xSTATS_BEGIN(t1);

    if ((ent->flags & 0x2) && (ent->pflags & k_XENT_HAS_GRAVITY)) {
        update_stacked_entity(*sc, *ent, dt);
    }

    xEntEndUpdate(ent, sc, dt);

    xSTATS_END(t1, ent_update);
}

void xEntBeginUpdate(xEnt* ent, xScene*, F32 dt)
{
    if (!ent->model)
        return;
    xModelUpdate(ent->model, dt);

    if (!ent->frame)
        return;
    xVec3Copy(&ent->frame->oldvel, &ent->frame->vel);
    ent->frame->oldmat = ent->frame->mat;
    ent->driving_reset = 0;
    xRotCopy(&ent->frame->oldrot, &ent->frame->rot);
    xMat4x3Copy(&ent->frame->mat, xModelGetFrame(ent->model));
    ent->frame->mode = 0;
}

void xEntEndUpdate(xEnt* ent, xScene* sc, F32 dt)
{
    if (!ent->model)
        return;
    if (ent->frame) {
        if (!(ent->frame->mode & 0x20000)) {
            xMat3x3Copy((xMat3x3*)ent->model->Mat, &ent->frame->mat);
        }

        if (!(ent->frame->mode & 0x10000)) {
            xVec3* mpos = &((xMat4x3*)ent->model->Mat)->pos;
            xVec3* upos = &ent->frame->mat.pos;
            xVec3Copy(mpos, upos);
        }

        if (ent->collModel && ent->collModel != ent->model)
            *ent->collModel->Mat = *ent->model->Mat;
        if (ent->camcollModel && ent->camcollModel != ent->model) {
            *ent->camcollModel->Mat = *ent->model->Mat;
        }
    }

    xModelEval(ent->model);

    if (ent->bupdate) {
        ent->bupdate(ent, &((xMat4x3*)ent->model->Mat)->pos);
    }

    if (ent->moreFlags & k_MORE_FLAGS_ANIM_COLL) {
        anim_coll::refresh(*ent);
    }

    if (ent->endUpdate)
        ent->endUpdate(ent, sc, dt);
}

void xEntDefaultBoundUpdate(xEnt* ent, xVec3* pos)
{
    xBound* bound = &ent->bound;

    if (bound->type == k_XBOUNDTYPE_SPHERE) {
        xVec3Copy(&bound->sph.center, pos);
        bound->sph.center.y += 0.7f;
        bound->sph.r = 0.7f;
    } else if (bound->type == k_XBOUNDTYPE_BOX) {
    } else if (bound->type == k_XBOUNDTYPE_OBB) {
        (void)0;
    }

    xBoundUpdate(bound);
    zGridUpdateEnt(ent);
}

#if DEBUG
DECOMP_FORCEACTIVE("%s(%d) : (ent->bound.type == k_XBOUNDTYPE_SPHERE) in '%s'\n")
#endif

void xEntDefaultTranslate(xEnt* ent, xVec3* dpos, xMat4x3* dmat)
{
    xASSERT_L(2636, ent);
    if (dmat) {
        if (ent->model) {
            xMat4x3Mul((xMat4x3*)ent->model->Mat, (xMat4x3*)ent->model->Mat, dmat);
        }

        if (ent->frame) {
            xMat4x3Mul(&ent->frame->mat, &ent->frame->mat, dmat);
        }

        xMat4x3ToWorld(xEntGetCenter(ent), dmat, xEntGetCenter(ent));
    } else {
        xASSERT_L(2647, dpos);

        if (ent->model) {
            xVec3AddTo(xEntGetPos(ent), dpos);
        }

        if (ent->frame) {
            xVec3AddTo(&ent->frame->mat.pos, dpos);
        }

        xVec3AddTo(xEntGetCenter(ent), dpos);
    }
}

static void xEntRotationToMatrix(xEntFrame* frame)
{
    xASSERT_L(2661, frame);

    if (frame->mode & 0x20) {
        if (frame->mode & 0x400) {
            xVec3AddTo(&frame->rot.axis, &frame->drot.axis);
            xMat3x3Euler(&frame->mat, frame->rot.axis.x, frame->rot.axis.y, frame->rot.axis.z);
        } else {
            frame->rot.angle = xAngleClamp(frame->rot.angle + frame->drot.angle);
            xMat3x3Rot(&frame->mat, &frame->rot.axis, frame->rot.angle);
        }

        frame->drot.angle = 0.0f;
    }

    if (frame->mode & k_XENT_SET_ROT) {
        DBprintf(DBML_VDBG, "xEnt: k_XENT_SET_ROT intentionally ignored\n");
    }
}

void xEntMotionToMatrix(xEnt* ent, xEntFrame* frame)
{
    xASSERT_L(2691, ent);
    xASSERT_L(2692, frame);

    if (frame->mode & 0x1000) {
        xEntRotationToMatrix(frame);
    }

    if (frame->mode & 0x2) {
        if (frame->mode & 0x800) {
            xMat3x3RMulVec(&frame->setDpos(), &frame->mat, &frame->getDpos());
        }

        xEntAddToPos(ent, &frame->getDpos());
        ent->frame->setDpos().assign(0.0f);
    }

    if (frame->mode & 0x8) {
        if (frame->mode & 0x800) {
            xMat3x3RMulVec(&frame->dvel, &frame->mat, &frame->dvel);
        }

        xVec3AddTo(&frame->vel, &frame->dvel);
        ent->frame->dvel.assign(0.0f);
    }

    if (!(frame->mode & 0x1000)) {
        xEntRotationToMatrix(frame);
    }
}

void xEntMove(xEnt* ent, xScene* sc, F32 dt)
{
    xASSERT_L(2741, ent->frame);

    if (ent->moreFlags & k_MORE_FLAGS_ANIM_COLL) {
        anim_coll::pre_move(*ent);
    }

    if (ent->move) {
        ent->move(ent, sc, dt, ent->frame);
    }

    xEntMotionToMatrix(ent, ent->frame);

    if (ent->driver && ent->driver->frame && (!ent->driveInfo || (ent->driveInfo->flags & 0x1))) {
        xEntFrame* dframe = ent->driver->frame;
        if (!ent->driveInfo) {
            xVec3 dpos;
            xVec3Sub(&dpos, &dframe->mat.pos, &dframe->oldmat.pos);
            ent->transl(ent, &dpos, NULL);
        } else {
            if (ent->baseType == eBaseTypeStatic) {
                xMat4x3 newMat;
                xMat4x3Mul(&newMat, &ent->driveInfo->driveMat, &dframe->mat);
                if (ent->model) {
                    if (ent->driveInfo->flags & 0x2) {
                        xVec3Copy(&((xMat4x3*)ent->model->Mat)->pos, &newMat.pos);
                    } else {
                        xMat4x3Copy((xMat4x3*)ent->model->Mat, &newMat);
                    }
                }

                if (ent->frame) {
                    if (ent->driveInfo->flags & 0x2) {
                        xVec3Copy(&ent->frame->mat.pos, &newMat.pos);
                    } else {
                        xMat4x3Copy(&ent->frame->mat, &newMat);
                    }
                }

                xMat3x3RMulVec(xEntGetCenter(ent), &dframe->mat, &ent->driveInfo->centerOffset);
                xVec3AddTo(xEntGetCenter(ent), &dframe->mat.pos);
            } else {
                if (!(ent->driveInfo->flags & 0x4)) {
                    xMat4x3 invOldmat, deltaMat;
                    RwMatrixUpdate((RwMatrix*)&dframe->oldmat);
                    RwMatrixInvert((RwMatrix*)&invOldmat, (RwMatrix*)&dframe->oldmat);
                    xMat4x3Mul(&deltaMat, &invOldmat, &dframe->mat);
                    if (ent->driveInfo->flags & 0x2) {
                        xVec3 startPos, dpos;
                        if (ent->frame)
                            xVec3Copy(&startPos, &ent->frame->mat.pos);
                        else if (ent->model)
                            xVec3Copy(&startPos, &((xMat4x3*)ent->model->Mat)->pos);
                        else
                            xVec3Init(&startPos, 0.0f, 0.0f, 0.0f);
                        xMat3x3RMulVec(&dpos, &deltaMat, &startPos);
                        xVec3AddTo(&dpos, &deltaMat.pos);
                        xVec3SubFrom(&dpos, &startPos);
                        ent->transl(ent, &dpos, NULL);
                    } else {
                        ent->transl(ent, NULL, &deltaMat);
                    }
                }

                ent->driveInfo->flags &= ~0x4;
            }
        }
    }

    if (ent->moreFlags & k_MORE_FLAGS_ANIM_COLL)
        anim_coll::post_move(*ent);
}

void xEntApplyPhysics(xEnt* ent, xScene* sc, F32 dt)
{
    xVec3 dposvel = { 0.0f, 0.0f, 0.0f };

    xASSERT_L(2839, ent->frame);

    if (ent->pflags & k_XENT_HAS_GRAVITY) {
        if (sc->flags & 0x1) {
            ent->frame->vel.y += sc->gravity * dt;
        }
    }

    if (ent->pflags & 0x10) {
        if (sc->flags & 0x2) {
            F32 tfric = 1.0f - sc->friction * dt;
            xVec3SMulBy(&ent->frame->vel, tfric);
        }
    }

    if (ent->pflags & k_XENT_HAS_DRAG) {
        if (sc->flags & 0x4) {
            F32 tdrag = 1.0f - sc->drag * dt;
            xVec3SMulBy(&ent->frame->vel, tdrag);
        }
    }

    xVec3Add(&dposvel, &ent->frame->vel, &ent->frame->oldvel);
    xVec3SMulBy(&dposvel, 0.5f * dt);

    if (dposvel.y < 0.0f) {
        F32 dposXZ = xsqrt(xsqr(dposvel.x) + xsqr(dposvel.z));
        F32 scaleXZ = (dposXZ > 0.00001f) ? (0.63f * (30.0f * dt)) / dposXZ : 0.0f;
        F32 scaleY = (0.63f * (30.0f * dt)) / xabs(dposvel.y);
        if (scaleXZ < 1.0f) {
            dposvel.x *= scaleXZ;
            dposvel.z *= scaleXZ;
            DBprintf(DBML_VDBG, "xEntApplyPhysics: clipping dposvel XZ (%f).\n", scaleXZ);
        }

        if (scaleY < 1.0f) {
            dposvel.y *= scaleY;
            DBprintf(DBML_VDBG, "xEntApplyPhysics: clipping dposvel Y (%f).\n", scaleY);
        }
    }

    ent->frame->setDpos() += dposvel;
    ent->frame->mode |= 0x2;
}

#if DEBUG || RELEASE
S32 xent_nupdates = 1;
S32 xent_entent_nchks = 1;
F32 xent_ee_per_upd = 1.0f;
S32 xent_nentcoll = 1;
F32 xent_ec_per_upd = 1.0f;
F32 xent_ee_per_ec = 1.0f;
S32 xent_nqcrejects = 1;
F32 xent_qcr_per_upd = 1.0f;
F32 xent_qcr_per_ec = 1.0f;
F32 xent_qcr_per_ee = 1.0f;
#endif

void xEntDebugUpdate()
{
#if DEBUG || RELEASE
    xent_nupdates++;
    xent_ee_per_upd = (F32)xent_entent_nchks / (F32)xent_nupdates;
    xent_ec_per_upd = (F32)xent_nentcoll / (F32)xent_nupdates;
    xent_ee_per_ec = (F32)xent_entent_nchks / (F32)xent_nentcoll;
    xent_qcr_per_upd = (F32)xent_nqcrejects / (F32)xent_nupdates;
    xent_qcr_per_ec = (F32)xent_nqcrejects / (F32)xent_nentcoll;
    xent_qcr_per_ee = (F32)xent_nqcrejects / (F32)xent_entent_nchks;
#endif
}

void xEntCollide(xEnt* ent, xScene* sc, F32 dt)
{
#if DEBUG || RELEASE
    xent_nentcoll++;
#endif

    xSTATS_BEGIN(t2);

    if (!ent->model) {
        DBprintf(DBML_DBG, "xEntCollide: called but no model, hmm.\n");
        return;
    }

    if (ent->collis->chk & (k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_ENV)) {
        xEntBeginCollide(ent, sc, dt);
    }

    if (ent->collis->chk & k_XENT_COLLTYPE_NPC) {
        xEntCollCheckNPCsByGrid(ent, sc, xEntCollCheckOneEntNoDepen);
    }

    if (ent->collis->chk & (k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN)) {
        xEntCollCheckByGrid(ent, sc, xEntCollCheckOneEntNoDepen);
    }

    if (ent->collis->chk & k_XENT_COLLTYPE_ENV) {
        xEntCollCheckEnv(ent, sc);
    }

    if (ent->collis->chk & (k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_ENV)) {
        F32 sbr = (ent->bound.type == k_XBOUNDTYPE_SPHERE) ? ent->bound.sph.r : 0.7f;
        if (ent->pflags & 0x80) {
            xCollis* coll = &ent->collis->colls[0];
            if (coll->flags & 0x1) {
                F32 h_dot_n = xVec3Dot(&coll->hdng, &coll->norm);
                if (h_dot_n > 0.0f) {
                    DBprintf(DBML_VDBG, "xEntCollide: unexpected inverted normal in NORM_DEPEN xform ... fixing.\n");
                    xVec3Inv(&coll->norm, &coll->norm);
                    h_dot_n = -h_dot_n;
                }

                F32 depen_len = h_dot_n * coll->dist + sbr;
                if (depen_len < 0.0f || depen_len > sbr) {
                    DBprintf(DBML_DBG, "xEntCollide: unexpected depen.length in NORM_DEPEN xform (%f) ... clamping.\n", depen_len);
                    depen_len = CLAMP(depen_len, 0.0f, sbr);
                }

                xVec3SMul(&coll->depen, &coll->norm, depen_len);
            }
        }

        if (ent->frame->vel.y <= 0.0f) {
            xEntCollideFloor(ent, sc, dt);
        } else {
            xEntCollideCeiling(ent, sc, dt);
        }

        xEntCollideWalls(ent, sc, dt);

        xEntEndCollide(ent, sc, dt);
    }

    xSTATS_END(t2, collision);
}

void xEntBeginCollide(xEnt* ent, xScene*, F32)
{
    if (ent->bupdate) {
        ent->bupdate(ent, &ent->frame->mat.pos);
    }

    for (U8 idx = 0; idx < k_XENT_MAX_COLL; idx++) {
        xCollis* coll = &ent->collis->colls[idx];
        coll->flags = 0x1f00;
        coll->optr = NULL;
        coll->mptr = NULL;
        coll->dist = 1e38f;
    }

    ent->collis->idx = 6;

    ent->collis->stat_sidx = 6;
    ent->collis->stat_eidx = 6;
    ent->collis->dyn_sidx = 6;
    ent->collis->dyn_eidx = 6;
    ent->collis->npc_sidx = 6;
    ent->collis->npc_eidx = 6;
    ent->collis->env_sidx = 6;
    ent->collis->env_eidx = 6;
}

void xEntEndCollide(xEnt* ent, xScene* sc, F32 dt)
{
    xASSERT_FMT_L(3104, ((U32*)(ent->collis->post) != (U32*)0xcccccccc), "uninitialized ptr on the stack !");

    if (ent->collis->post) {
        ent->collis->post(ent, sc, dt, ent->collis);
    }

#if DEBUG || RELEASE
    if (xDebugModeGet() == 6 && (ent->miscflags & 0x4)) {
        F32 sbr = (ent->bound.type == k_XBOUNDTYPE_SPHERE) ? ent->bound.sph.r : 0.7f;
        xVec3 start, end;
        xCollis* coll;
        xVec3Copy(&start, &ent->frame->mat.pos);
        start.y += 3.0f * sbr;

        if (ent->collis->colls[0].flags & 0x1) {
            xVec3Add(&end, &start, &ent->collis->colls[0].norm);
            xDrawSetColor(g_NEON_BLUE);
            xDrawLine(&start, &end);
        }

        for (U8 idx = 0; idx < 6; idx++) {
            coll = &ent->collis->colls[idx];

            if (coll->flags & 0x1) {
                xVec3SMul(&end, &coll->depen, 3.0f);
                xVec3AddTo(&end, &start);
                xDrawSetColor(g_NEON_RED);
                xDrawLine(&start, &end);

                xVec3SMul(&end, &coll->tohit, 3.0f);
                xVec3AddTo(&end, &start);
                xDrawSetColor(g_NEON_GREEN);
                xDrawLine(&start, &end);
            }
        }
    }
#endif
}

void xEntCollCheckEnv(xEnt* p, xScene* sc)
{
    xASSERT_L(3156, p);
    xASSERT_L(3157, sc);
    xASSERT_L(3158, p->collis);

    p->collis->env_sidx = p->collis->idx;

    xCollis* coll = &p->collis->colls[p->collis->idx];
    coll->flags = 0x1f00;

    U8 ncolls = xSphereHitsEnv3(&p->bound.sph, sc->env, coll, k_XENT_MAX_COLL - p->collis->idx, PI / 4.0f);

    p->collis->idx = p->collis->idx + ncolls;
    p->collis->env_eidx = p->collis->idx;
}

#define NUM_CELL_OFFS 3

static void xEntCollCheckOneGrid(xEnt* p, xScene* sc, xEntCollCheckCallback hitIt, xGrid* grid)
{
    xGridIterator it;
    xEnt* ent;
    xVec3* pcenter = xEntGetCenter(p);
    S32 px, pz;
    xGridBound* cell = xGridIterFirstCell(grid, pcenter->x, pcenter->y, pcenter->z, px, pz, it);
    while (cell) {
        xBound* cellbound = (xBound*)(cell + 1);
        if (xQuickCullIsects(&p->bound.qcd, &cellbound->qcd)) {
            ent = (xEnt*)cell->data;
            hitIt(ent, sc, p);
        }

        cell = xGridIterNextCell(it);
    }

    static S32 offs[4][NUM_CELL_OFFS][2] = {
        { { -1, 0 }, { -1, -1 }, { 0, -1 } },
        { { 0, -1 }, { 1, -1 }, { 1, 0 } },
        { { 1, 0 }, { 1, 1 }, { 0, 1 } },
        { { 0, 1 }, { -1, 1 }, { -1, 0 } },
    };
    static S32 k;

    xBox clbox;
    F32 clcenterx, clcenterz;
    clbox.lower.x = grid->csizex * px, clbox.lower.z = grid->csizez * pz;
    clbox.lower.x += grid->minx, clbox.lower.z += grid->minz;
    clcenterx = 0.5f * grid->csizex, clcenterz = 0.5f * grid->csizez;
    clcenterx += clbox.lower.x, clcenterz += clbox.lower.z;

    if (pcenter->x < clcenterx) {
        if (pcenter->z < clcenterz) {
            k = 0;
        } else {
            k = 1;
        }
    } else {
        if (pcenter->z < clcenterz) {
            k = 3;
        } else {
            k = 2;
        }
    }

    for (S32 i = 0; i < NUM_CELL_OFFS; i++) {
        S32 _x = px + offs[k][i][1];
        S32 _z = pz + offs[k][i][0];
        cell = xGridIterFirstCell(grid, _x, _z, it);
        while (cell) {
            xBound* cellbound = (xBound*)(cell + 1);
            if (xQuickCullIsects(&p->bound.qcd, &cellbound->qcd)) {
                ent = (xEnt*)cell->data;
                hitIt(ent, sc, p);
            }

            cell = xGridIterNextCell(it);
        }
    }

    cell = xGridIterFirstCell(&grid->other, it);
    while (cell) {
        xBound* cellbound = (xBound*)(cell + 1);
        if (xQuickCullIsects(&p->bound.qcd, &cellbound->qcd)) {
            ent = (xEnt*)cell->data;
            hitIt(ent, sc, p);
        }

        cell = xGridIterNextCell(it);
    }
}

void xEntCollCheckByGrid(xEnt* p, xScene* sc, xEntCollCheckCallback hitIt)
{
    xASSERT_L(3250, p->collis);

    p->collis->stat_sidx = p->collis->idx;
    p->collis->dyn_sidx = p->collis->idx;
    xEntCollCheckOneGrid(p, sc, hitIt, &colls_grid);
    xEntCollCheckOneGrid(p, sc, hitIt, &colls_oso_grid);
    p->collis->stat_eidx = p->collis->idx;
    p->collis->dyn_eidx = p->collis->idx;
}

void xEntCollCheckNPCsByGrid(xEnt* p, xScene* sc, xEntCollCheckCallback hitIt)
{
    xASSERT_L(3262, p->collis);

    p->collis->npc_sidx = p->collis->idx;
    xEntCollCheckOneGrid(p, sc, hitIt, &npcs_grid);
    p->collis->npc_eidx = p->collis->idx;
}

S32 xent_entent;

xEnt* xEntCollCheckOneEntNoDepen(xEnt* ent, xScene*, void* data)
{
    xEnt* p = (xEnt*)data;
    xCollis* coll;

#if DEBUG || RELEASE
    xent_entent_nchks++;
#endif

    xent_entent = TRUE;

    U32 modl_coll = FALSE;

    if (p->collis->idx >= 15) {
        DBprintf(DBML_VDBG, "xEntCollCheckOneEntNoDepen: max num collisions reached\n");
        DBprintf(DBML_VDBG, "xEntCollCheckOneEntNoDepen: ... well almost, but we gotta save room for env\n");
        xent_entent = FALSE;
        return NULL;
    }

    if (!(ent->chkby & p->collType)) {
        xent_entent = FALSE;
        return ent;
    }

    if (ent == p) {
        xent_entent = FALSE;
        return ent;
    }

    if (ent->id == p->id) {
        if (ent == p && ent->collType == k_XENT_COLLTYPE_NPC) {
            (void)0;
        }

        if (ent->baseType != eBaseTypeBoulder && ent->baseType != eBaseTypeProjBomb) {
            xent_entent = FALSE;
            return ent;
        }
    }

    coll = &p->collis->colls[p->collis->idx];

    if (ent->collLev == k_XENT_COLLLEV_MODEL) {
        if (p->collType & (k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC)) {
            modl_coll = TRUE;
        }
    }

    if (modl_coll) {
        coll->flags = 0;
    } else {
        coll->flags = 0x1f00;
    }

    xBoundHitsBound(&p->bound, &ent->bound, coll);

    if (coll->flags & 0x1) {
        xBound tmp;
        xBound* bptr = &p->bound;
        if (modl_coll) {
            coll->flags = 0x1f00;

#if DEBUG || RELEASE
            S64 t0 = iTimeGet();
#endif

            U8 ncolls = 0;
            if (p->bound.type == k_XBOUNDTYPE_SPHERE) {
                xModelInstance* collModel = ent->collModel ? ent->collModel : ent->model;
                ncolls = xSphereHitsModel3(&p->bound.sph, collModel, coll, 15 - p->collis->idx, PI / 4.0f);
            } else if (p->bound.type == k_XBOUNDTYPE_BOX || p->bound.type == k_XBOUNDTYPE_OBB) {
                tmp.type = k_XBOUNDTYPE_SPHERE; xVec3* upper = &p->bound.box.box.upper, *lower = &p->bound.box.box.lower;
                xVec3Add(&tmp.sph.center, upper, lower);
                xVec3SMulBy(&tmp.sph.center, 0.5f);
                tmp.sph.r = 0.167f * (upper->x + upper->y + upper->z - lower->x - lower->y - lower->z);

                xModelInstance* collModel = ent->collModel ? ent->collModel : ent->model;
                ncolls = xSphereHitsModel3(&p->bound.sph, collModel, coll, 15 - p->collis->idx, PI / 4.0f);
            } else {
                xFAIL_MSG_L(3415, "Expecting bound to be sphere or box");
            }

#if DEBUG || RELEASE
            S64 t1 = iTimeGet();
            xModCollAddSample(ent, t1 - t0);
#endif

            for (U8 idx = 0; idx < ncolls; idx++) {
                coll[idx].optr = ent;
                coll[idx].mptr = ent->model;
                p->collis->idx++;
            }

            xent_entent = FALSE;
            return ent;
        }

        coll->oid = ent->id;
        coll->optr = ent;
        coll->mptr = ent->model;

        p->collis->idx++;

        if ((ent->pflags & 0x20) && ent->bound.type == k_XBOUNDTYPE_SPHERE && bptr->type == k_XBOUNDTYPE_SPHERE && coll->hdng.y < -0.866025f) {
            F32 rsum, dx, dy, dz, hsqr;
            rsum = bptr->sph.r + ent->bound.sph.r, dx = bptr->sph.center.x - ent->bound.sph.center.x, dz = bptr->sph.center.z - ent->bound.sph.center.z;
            dy = bptr->sph.center.y - ent->bound.sph.center.y;
            hsqr = xsqr(rsum) - (xsqr(dx) + xsqr(dz));
            if (hsqr >= 0.0f) {
                coll->depen.x = 0.0f;
                coll->depen.y = xsqrt(hsqr) - dy;
                coll->depen.z = 0.0f;
                coll->dist = 0.7f - coll->depen.y;
                coll->hdng.x = 0.0f;
                coll->hdng.y = -1.0f;
                coll->hdng.z = 0.0f;
            } else {
                xFAIL_MSG_L(3478, "whatup?!");
            }
        }
    }

    xent_entent = FALSE;
    return ent;
}

void xEntCollideFloor(xEnt* p, xScene* sc, F32 dt)
{
    xASSERT_L(3491, p);
    xASSERT_L(3492, sc);
    xASSERT_L(3493, p->collis);

    xCollis* coll = &p->collis->colls[0];
    U8 idx;
    xCollis* ml = coll;
    xVec3 motion;
    F32 mlen;
    S32 stepping = FALSE;
    F32 sbr = (p->bound.type == k_XBOUNDTYPE_SPHERE) ? p->bound.sph.r : 0.7f;

    xVec3Copy(&motion, &p->frame->mat.pos);
    xVec3SubFrom(&motion, &p->frame->oldmat.pos);

    motion.y = 0.0f;
    mlen = xVec3Length(&motion);

    for (idx = 6; idx < p->collis->idx; idx++) {
        xCollis* mf = &p->collis->colls[idx];

        if (!(mf->flags & 0x1)) {
            continue;
        }

        xEnt* fent = (xEnt*)mf->optr;

        if (fent) {
            if (!(fent->collType == k_XENT_COLLTYPE_DYN || fent->collType == k_XENT_COLLTYPE_STAT || ((fent->pflags & 0x20) && 0.0f == mf->hdng.x && 0.0f == mf->hdng.z))) {
                continue;
            }

            if (!(p->collis->depenq ? p->collis->depenq(p, fent, sc, dt, mf) : (fent->collType & p->collis->pen) && (fent->penby & p->collType))) {
                continue;
            }
        } else {
            if (!(p->collis->pen & k_XENT_COLLTYPE_ENV)) {
                continue;
            }
        }

        if (mf->dist < ml->dist)
            if (mf->hdng.y < -icos(PI / 3.0f) && (mf->norm.y > icos(nsn_angle) || p->frame->oldmat.pos.y > p->frame->mat.pos.y + dt * (sc->gravity * dt))) {
                ml = mf;
                stepping = FALSE;
            } else if (mlen > 0.001f && mf->hdng.y < 0.65f / sbr - 1.0f && mf->norm.y > icos(nsn_angle)) {
                stepping = TRUE;
                ml = mf;
            }
    }

    if (ml != coll) {
        F32 flr_dist = ml->dist * xabs(ml->hdng.y);

        *coll = *ml;

        if (flr_dist < sbr) {
            ml->flags |= 0x6;

            if (stepping) {
                p->frame->mat.pos.y += 1.5f * dt;
                p->frame->mat.pos.x += ml->depen.x;
                p->frame->mat.pos.z += ml->depen.z;
            } else {
                p->frame->mat.pos.y += ml->depen.y;
            }

            p->frame->vel.y = 0.0f;
        }
    }
}

void xEntCollideCeiling(xEnt* p, xScene* sc, F32 dt)
{
    xASSERT_L(3619, p);
    xASSERT_L(3620, sc);
    xASSERT_L(3621, p->collis);

    xCollis* coll = &p->collis->colls[1];
    U8 idx;
    xCollis* ml = coll;
    F32 sbr = (p->bound.type == k_XBOUNDTYPE_SPHERE) ? p->bound.sph.r : 0.7f;

    for (idx = 6; idx < p->collis->idx; idx++) {
        xCollis* mf = &p->collis->colls[idx];

        if (mf->optr) {
            xEnt* fent = (xEnt*)mf->optr;
            if (!(p->collis->depenq ? p->collis->depenq(p, fent, sc, dt, mf) : (fent->collType & p->collis->pen) && (fent->penby & p->collType))) {
                continue;
            }
        } else {
            if (!(p->collis->pen & k_XENT_COLLTYPE_ENV)) {
                continue;
            }
        }

        if (mf->hdng.y > icos(PI / 4.0f) && mf->dist < ml->dist) {
            ml = mf;
        }
    }

    if (ml != coll) {
        F32 ceil_dist = ml->dist * xabs(ml->hdng.y);

        DBprintf(DBML_VDBG, "xEntCollideCeiling: hit it.\n");

        *coll = *ml;
        ml->flags |= 0xa;

        if (ceil_dist < sbr) {
            p->frame->mat.pos.y -= sbr - ceil_dist;
            p->frame->vel.y = 0.0f;
        }
    }
}

void xEntCollideWalls(xEnt* p, xScene* sc, F32 dt)
{
    xCollis* coll;
    xEnt* cent;
    U8 sidx, eidx, idx;

    xASSERT_L(3714, p);
    xASSERT_L(3715, sc);

    F32 sbr = (p->bound.type == k_XBOUNDTYPE_SPHERE) ? p->bound.sph.r : 0.7f;

    if (p->collis->pen & k_XENT_COLLTYPE_NPC) {
        sidx = p->collis->npc_sidx;
        eidx = p->collis->npc_eidx;
        for (idx = sidx; idx < eidx; idx++) {
            coll = &p->collis->colls[idx];
            cent = (xEnt*)coll->optr;
            xASSERT_L(3726, cent);
            if (!(coll->flags & 0x2) && coll->dist < sbr && (p->collis->depenq ? p->collis->depenq(p, cent, sc, dt, coll) : cent->penby & p->collType)) {
                if (0.0f != coll->depen.x || 0.0f != coll->depen.z) {
                    coll->depen.y = 0.0f;
                }

                xEntAddToPos(p, &coll->depen);
            }
        }
    }

    if (p->collis->pen & k_XENT_COLLTYPE_DYN) {
        sidx = p->collis->dyn_sidx;
        eidx = p->collis->dyn_eidx;
        for (idx = sidx; idx < eidx; idx++) {
            coll = &p->collis->colls[idx];
            cent = (xEnt*)coll->optr;
            xASSERT_L(3769, cent);
            if (!(coll->flags & 0x2) && coll->dist < sbr && (p->collis->depenq ? p->collis->depenq(p, cent, sc, dt, coll) : cent->penby & p->collType)) {
                coll->depen.y = 0.0f;
                xEntAddToPos(p, &coll->depen);
            }
        }
    }

    if (p->collis->pen & k_XENT_COLLTYPE_STAT) {
        sidx = p->collis->stat_sidx;
        eidx = p->collis->stat_eidx;
        for (idx = sidx; idx < eidx; idx++) {
            coll = &p->collis->colls[idx];
            cent = (xEnt*)coll->optr;
            xASSERT_L(3805, cent);
            if (!(coll->flags & 0x2) && coll->dist < sbr && (p->collis->depenq ? p->collis->depenq(p, cent, sc, dt, coll) : cent->penby & p->collType)) {
                coll->depen.y = 0.0f;
                xEntAddToPos(p, &coll->depen);
            }
        }
    }

    if (p->collis->pen & k_XENT_COLLTYPE_ENV) {
        sidx = p->collis->env_sidx;
        eidx = p->collis->env_eidx;
        for (idx = sidx; idx < eidx; idx++) {
            coll = &p->collis->colls[idx];
            if (!(coll->flags & 0x2) && coll->dist < sbr) {
                coll->depen.y = 0.0f;
                xEntAddToPos(p, &coll->depen);
            }
        }
    }
}

xBox* xEntGetAllEntsBox()
{
    return &all_ents_box;
}

#if DEBUG || RELEASE
void xEntDebugInit()
{
    U32 num_xents = MAX_DBG_XENTS;

    xDebugModeRegisterCallback(6, xEntDebugCB);

    dbg_num = 0;
    dbg_xents = (xEnt**)xMEMALLOC_L(num_xents * sizeof(xEnt*), 0, eMemMgrTag_Debug, 0, 3903);
    dbg_idx = 0;
    dbg_mode = 1;

    dbg_holding_inc = FALSE;
    dbg_holding_dec = FALSE;

    xModCollInit();
}

void xEntDebugExit()
{
    dbg_num = 0;

    dbg_xents = NULL;
    dbg_idx = -1;
    dbg_mode = 1;

    xModCollExit();
}

void xEntDebugAdd(xEnt* xent)
{
    if (dbg_num >= MAX_DBG_XENTS) {
        DBprintf(DBML_DBG, "xEntDebugAdd: couldnt add ent %d.\n", dbg_num);
        return;
    }

    dbg_xents[dbg_num++] = xent;

    xModCollAdd(xent);
}

static void xEntDebugCB()
{
    if (dbg_idx == -1) {
        return;
    }

    if (dbg_num == 0) {
        return;
    }

    xASSERT_L(3947, dbg_idx >= 0);
    xASSERT_L(3948, dbg_idx < dbg_num);

    xEnt* xent = dbg_xents[dbg_idx];
    xASSERT_L(3951, xent);

    xEntDebugWrite(xent);

    xEntDebugDraw(xent);

    xEntDebugIPad(xent);
}

static char* xEntCollTypeStrSingle(U8 type)
{
    switch (type) {
    case k_XENT_COLLTYPE_TRIG:
        return "trig";
    case k_XENT_COLLTYPE_STAT:
        return "stat";
    case k_XENT_COLLTYPE_DYN:
        return "dyn";
    case k_XENT_COLLTYPE_NPC:
        return "npc";
    case k_XENT_COLLTYPE_PC:
        return "pc";
    case k_XENT_COLLTYPE_ENV:
        return "env";
    case k_XENT_COLLTYPE_INHIBIT:
        return "inhibit";
    }

    return "???";
}

static char* xEntCollTypeStrAll(char* str, U8 type)
{
    xASSERT_L(3986, str);
    str[0] = '\0';
    if (type & k_XENT_COLLTYPE_TRIG) {
        strcat(str, "trig ");
    }

    if (type & k_XENT_COLLTYPE_STAT) {
        strcat(str, "stat ");
    }

    if (type & k_XENT_COLLTYPE_DYN) {
        strcat(str, "dyn ");
    }

    if (type & k_XENT_COLLTYPE_NPC) {
        strcat(str, "npc ");
    }

    if (type & k_XENT_COLLTYPE_PC) {
        strcat(str, "pc ");
    }

    if (type & k_XENT_COLLTYPE_ENV) {
        strcat(str, "env ");
    }

    if (type & k_XENT_COLLTYPE_INHIBIT) {
        strcat(str, "inhibit ");
    }

    return str;
}

static char* xEntCollLevStr(U8 lev)
{
    switch (lev) {
    case k_XENT_COLLLEV_FASTEST:
        return "fastest";
    case k_XENT_COLLLEV_AABB:
        return "aabb (box)";
    case k_XENT_COLLLEV_SPH:
        return "sph";
    case k_XENT_COLLLEV_OBB:
        return "obb";
    case k_XENT_COLLLEV_BOUND:
        return "bound";
    case k_XENT_COLLLEV_MODEL:
        return "model";
    }

    return "???";
}

static char* xEntBoundTypeStr(U8 type)
{
    switch (type) {
    case k_XBOUNDTYPE_NONE:
        return "none";
    case k_XBOUNDTYPE_SPHERE:
        return "sph";
    case k_XBOUNDTYPE_BOX:
        return "box";
    case k_XBOUNDTYPE_CYLINDER:
        return "cyl";
    case k_XBOUNDTYPE_OBB:
        return "obb";
    }

    return "???";
}

#define DM_XENT_COLLB 3

static char* xEntBoundValStr(char* str, const xBound* b)
{
    xASSERT_L(4050, str);
    xASSERT_L(4051, b);
    str[0] = '\0';
    switch (b->type) {
    case k_XBOUNDTYPE_NONE:
        break;
    case k_XBOUNDTYPE_SPHERE:
        sprintf(str, "<%.3f %.3f %.3f> %.3f", b->sph.center.x, b->sph.center.y, b->sph.center.z, b->sph.r);
        break;
    case k_XBOUNDTYPE_BOX:
        sprintf(str, "<%.3f %.3f %.3f> <%.3f %.3f %.3f>", b->box.box.lower.x, b->box.box.lower.y, b->box.box.lower.z, b->box.box.upper.x, b->box.box.upper.y, b->box.box.upper.z);
        break;
    case k_XBOUNDTYPE_OBB:
        sprintf(str, "<%.3f %.3f %.3f> <%.3f %.3f %.3f>", b->box.box.lower.x, b->box.box.lower.y, b->box.box.lower.z, b->box.box.upper.x, b->box.box.upper.y, b->box.box.upper.z);
        break;
    }

    return str;
}

#define NUM_SCOLL_NAMES 6

static char scoll_name[NUM_SCOLL_NAMES][6] = { "floor", "ceil", "front", "left", "rear", "right" };
static S32 sBoneIndex;

static void xEntDebugWrite(const xEnt* xent)
{
    char str[40];

    xprintf("name:         %s", xSceneID2Name(globals.sceneCur, xent->id));
    xprintf("   id: 0x%08x\n", xent->id);
    if (dbg_mode == 1) {
        xprintf("is enabled:   %s\n", xbtoa(xent->baseFlags & k_XBASE_IS_ENABLED));
        xprintf("is visible:   %s\n", xbtoa(xent->flags & k_XENT_IS_VISIBLE));
        xprintf("is moving:    %s\n", xbtoa(xent->pflags & k_XENT_IS_MOVING));
        xprintf("has velocity: %s\n", xbtoa(xent->pflags & k_XENT_HAS_VELOCITY));
        xprintf("has gravity:  %s\n", xbtoa(xent->pflags & k_XENT_HAS_GRAVITY));
        xprintf("has drag:     %s\n", xbtoa(xent->pflags & k_XENT_HAS_DRAG));
        const xMat3x3* mat = xent->model ? (xMat3x3*)xent->model->Mat : (xent->frame ? &xent->frame->mat : NULL);

        const xVec3* pos = xent->model ? &((xMat4x3*)xent->model->Mat)->pos : (xent->frame ? &xent->frame->mat.pos : NULL);

        if (pos) {
            xprintf("pos:   <%8.3f %8.3f %8.3f>\n", pos->x, pos->y, pos->z);
            if (xent->pflags & k_XENT_IS_MOVING) {
                xVec3 dpos;
                xVec3Sub(&dpos, pos, &xent->frame->oldmat.pos);
                xprintf("dpos:  <%8.3f %8.3f %8.3f>\n", dpos.x, dpos.y, dpos.z);
            }
        }

        if (xent->pflags & k_XENT_HAS_VELOCITY) {
            xASSERT_L(4110, xent->frame);
            xVec3* vel = &xent->frame->vel;
            xVec3* ovel = &xent->frame->oldvel;
            xVec3 dvel;
            xVec3Sub(&dvel, vel, ovel);
            xprintf("vel:   <%8.3f %8.3f %8.3f>\n", vel->x, vel->y, vel->z);
            xprintf("dvel:  <%8.3f %8.3f %8.3f>\n", dvel.x, dvel.y, dvel.z);
        }

        if (mat) {
            xVec3 scale;
            xMat3x3GetScale(mat, &scale);
            xprintf("scale: <%8.3f %8.3f %8.3f>\n", scale.x, scale.y, scale.z);
            xMat3x3 mat_norm;
            xMat3x3Normalize(&mat_norm, mat);
            xVec3 ang;
            xMat3x3GetEuler(&mat_norm, &ang);
            xprintf("ang:   <%6.1f   %6.1f   %6.1f  >   Y P R\n", (180.0f / PI) * xDangleClamp(ang.x), (180.0f / PI) * xDangleClamp(ang.y), (180.0f / PI) * xDangleClamp(ang.z));
        }
    }

    if (dbg_mode == 2 || dbg_mode == DM_XENT_COLLB) {
        xprintf("colltype:     %s\n", xEntCollTypeStrSingle(xent->collType));
        xprintf("coll-lev:     %s\n", xEntCollLevStr(xent->collLev));
        xprintf("chkby:        %s\n", xEntCollTypeStrAll(str, xent->chkby));
        xprintf("penby:        %s\n", xEntCollTypeStrAll(str, xent->penby));
        xprintf("bound: %s", xEntBoundTypeStr(xent->bound.type));
        xprintf("   %s\n", xEntBoundValStr(str, &xent->bound));
        if (xent->collis) {
            if (dbg_mode == 2) {
                xprintf("chk:        %s\n", xEntCollTypeStrAll(str, xent->collis->chk));
                xprintf("pen:        %s\n", xEntCollTypeStrAll(str, xent->collis->pen));
                xprintf("hits:       stat:%d", xent->collis->stat_eidx - xent->collis->stat_sidx);
                xprintf("   dyn:%d", xent->collis->dyn_eidx - xent->collis->dyn_sidx);
                xprintf("   npc:%d", xent->collis->npc_eidx - xent->collis->npc_sidx);
                xprintf("   env:%d\n", xent->collis->env_eidx - xent->collis->env_sidx);
            } else {
                xASSERT_L(4151, dbg_mode == DM_XENT_COLLB);
                xCollis* scoll = &xent->collis->colls[0];
                for (S32 i = 0; i < NUM_SCOLL_NAMES; i++, scoll++) {
                    if (scoll->flags & 0x1) {
                        xprintf("%s-hit", scoll_name[i]);
                    } else {
                        xprintf("no %s", scoll_name[i]);
                    }

                    if (scoll->dist < 1e38f) {
                        xprintf("   %.3f", scoll->dist);
                        xprintf(" <%.3f %.3f %.3f>\n", scoll->hdng.x, scoll->hdng.y, scoll->hdng.z);
                        xprintf("%s-oid:  %u", scoll_name[i], scoll->oid);

                        xVec3 lhdng;
                        xMat3x3LMulVec(&lhdng, &xent->frame->mat, &scoll->hdng);
                        xprintf("   lhdng: <%.3f %.3f %.3f>\n", lhdng.x, lhdng.y, lhdng.z);
                        if (scoll->optr) {
                            xprintf("%s-optr: %d", scoll_name[i], scoll->optr);
                            xprintf("   (%s)\n", xSceneID2Name(globals.sceneCur, ((xBase*)scoll->optr)->id));
                        } else {
                            xprintf("\n");
                        }
                    } else {
                        xprintf("\n\n\n");
                    }
                }
            }
        } else {
            xprintf("no checking\n");
        }
    }

    xprintf("\nBone : %d / %d\n", sBoneIndex, xent->model->BoneCount);
}

static void xEntDebugDraw(const xEnt* xent)
{
    xVec3 v;
    xMat4x3 t;

    if (!xent->model) {
        return;
    }

    if (sBoneIndex) {
        xMat4x3Mul(&t, &((xMat4x3*)xent->model->Mat)[sBoneIndex], (xMat4x3*)xent->model->Mat);
    } else {
        xMat4x3Copy(&t, (xMat4x3*)xent->model->Mat);
    }

    xVec3Add(&v, &t.pos, &t.right);
    xDrawSetColor(255, 0, 0, 255);
    xDrawLine(&t.pos, &v);
    xVec3Add(&v, &t.pos, &t.up);
    xDrawSetColor(0, 255, 0, 255);
    xDrawLine(&t.pos, &v);
    xVec3Add(&v, &t.pos, &t.at);
    xDrawSetColor(0, 0, 255, 255);
    xDrawLine(&t.pos, &v);

    xDrawSetColor(g_PIMP_GOLD);
    xBoundDraw(&xent->bound);
}

static void xEntDebugIPad(xEnt* xent)
{
    F32 now = iTimeDiffSec(iTimeGet());
    F32 dt = now - dbg_ltm;
    dbg_tmr += dt;
    dbg_ltm = now;

    if (xent->model) {
        if (zPad::GetDebugPad()->Pressed(0x10000)) {
            sBoneIndex++;
            if (sBoneIndex > xent->model->BoneCount) sBoneIndex = 0;
        } else if (zPad::GetDebugPad()->Pressed(0x80000)) {
            sBoneIndex--;
            if (sBoneIndex < 0) sBoneIndex = xent->model->BoneCount;
        }
    }

    if (zPad::GetDebugPad()->On(0x20)) {
        if (zPad::GetDebugPad()->Pressed(0x20)) {
            dbg_holding_inc = TRUE;
            dbg_tmr = 0.0f;
        }
        else if (dbg_holding_inc) {
            if (dbg_tmr >= 0.25f) {
                dbg_holding_inc = FALSE;
                goto next_ent;
            }
        } else if (dbg_tmr >= 0.065f) {
            goto next_ent;
        }
    } else if (dbg_holding_inc) {
        dbg_holding_inc = FALSE;
    next_ent:
        sBoneIndex = 0;
        dbg_tmr = 0.0f;
        dbg_idx++;
        if (dbg_idx >= dbg_num) {
            dbg_idx = 0;
        }
    } else if (zPad::GetDebugPad()->On(0x80)) {
        if (zPad::GetDebugPad()->Pressed(0x80)) {
            dbg_holding_dec = TRUE;
            dbg_tmr = 0.0f;
        }
        else if (dbg_holding_dec) {
            if (dbg_tmr >= 0.25f) {
                dbg_holding_dec = FALSE;
                goto prev_ent;
            }
        } else if (dbg_tmr >= 0.065f) {
            goto prev_ent;
        }
    } else if (dbg_holding_dec) {
        dbg_holding_dec = FALSE;
    prev_ent:
        sBoneIndex = 0;
        dbg_tmr = 0.0f;
        dbg_idx--;
        if (dbg_idx < 0) {
            dbg_idx = dbg_num - 1;
        }
    }

    if (zPad::GetDebugPad()->Pressed(0x40000)) {
        if (dbg_mode == 1) {
            dbg_mode = 2;
            dbg_xents[dbg_idx]->miscflags |= 0x4;
        } else if (dbg_mode == 2) {
            dbg_mode = DM_XENT_COLLB;
        } else {
            dbg_mode = 1;
            dbg_xents[dbg_idx]->miscflags &= (U8)~0x4;
        }
    }
}

static void xModCollCB();

static void xModCollInit()
{
    xDebugModeRegisterCallback(4, xModCollCB);

    mcrecs = (xModCollRec*)xMEMALLOC_L(MAX_MCRECS * sizeof(xModCollRec), 0, eMemMgrTag_Model, 0, 4313);
    num_mcrecs = 0;
    mctime = 0;
    mcmode = eModCollMode_None;

    for (S32 i = 0; i < MAX_MCRECS; i++) {
        smcrecs[i].owner = NULL;
    }
}

static void xModCollExit()
{
    mcrecs = NULL;
    num_mcrecs = 0;
    mctime = 0;

    for (S32 i = 0; i < MAX_MCRECS; i++) {
        smcrecs[i].owner = NULL;
    }
}

static void xModCollAdd(xEnt* xent)
{
    if (num_mcrecs >= MAX_MCRECS) {
        DBprintf(DBML_DBG, "xModCollAdd: couldnt add mcrec %d.\n", dbg_num);
        return;
    }

    xVALIDATE_L(4340, xent);
    xVALIDATE_L(4341, xent->model);
    xVALIDATE_L(4342, xent->model->Data);
    if (xent && xent->model && xent->model->Data) {
        xModCollRec* end = &mcrecs[num_mcrecs];
        xModCollRec* mc = mcrecs;
        for (; mc < end; mc++) {
            if (mc->owner->model->Data == xent->model->Data) {
                mc->nusing++;
                return;
            }
        }

        num_mcrecs++;

        mc->owner = xent;
        mc->ctime = 0;
        mc->ltime = 0;
        mc->frac = 0.0f;
        mc->nusing = 1;
    }
}

void xModCollAddSample(xEnt* xent, S64 elapsed)
{
    xVALIDATE_L(4366, xent);
    xVALIDATE_L(4367, xent->model);
    xVALIDATE_L(4368, xent->model->Data);
    if (!xent || !xent->model || !xent->model->Data) {
        return;
    }

    xModCollRec* end = &mcrecs[num_mcrecs];
    for (xModCollRec* mc = mcrecs; mc < end; mc++) {
        if (mc->owner->model) {
            if (mc->owner->model->Data == xent->model->Data) {
                mc->ctime += elapsed;
                mc->ltime += elapsed;
            }
        }
    }

    mctime += elapsed;
}
#else
void xModCollAddSample(xEnt*, S64)
{
}
#endif

#if DEBUG || RELEASE
static S32 CmpModCollCtime(const void* a, const void* b)
{
    xModCollRec* mca = (xModCollRec*)a;
    xModCollRec* mcb = (xModCollRec*)b;
    if (mca->ctime < mcb->ctime) {
        return 1;
    } else if (mca->ctime > mcb->ctime) {
        return -1;
    }

    return 0;
}

static S32 CmpModCollLtime(const void* a, const void* b)
{
    xModCollRec* mca = (xModCollRec*)a;
    xModCollRec* mcb = (xModCollRec*)b;
    if (mca->ltime < mcb->ltime) {
        return 1;
    } else if (mca->ltime > mcb->ltime) {
        return -1;
    } else if (mca->ctime < mcb->ctime) {
        return 1;
    } else if (mca->ctime > mcb->ctime) {
        return -1;
    }

    return 0;
}

static void xModCollCB()
{
    xModCollRec* mc;
    S32 i;

    F32 poo = MAX(0.00001f, (F32)mctime);
    F32 scale = 1.0f / poo;

    for (i = 0, mc = mcrecs; i < num_mcrecs; i++, mc++) {
        mc->frac = (F32)mc->ctime * scale;
    }

    memcpy(smcrecs, mcrecs, num_mcrecs * sizeof(xModCollRec));
    xprintf("no.xents = %d   no.uniq.models = %d\n", dbg_num, num_mcrecs);

    if (mcmode == eModCollMode_CTime) {
        qsort(smcrecs, num_mcrecs, sizeof(xModCollRec), CmpModCollCtime);
        xprintf("\nsort by ctime (cumulative time)\n");
        xprintf("model-addr  no  ctime  ltime   frac   xent-name\n");

        for (i = 0, mc = smcrecs; i < MIN(10, num_mcrecs); i++, mc++) {
            xprintf("0x%08x %3d %6dK %7d %6.4f %s\n", mc->owner->model->Data, mc->nusing, mc->ctime >> 10, mc->ltime, mc->frac,
                    xSceneID2Name(globals.sceneCur, mc->owner->id));
        }

        if (zPad::GetDebugPad()->Pressed(0x40000))
            mcmode = eModCollMode_LTime;
    }
    else if (mcmode == eModCollMode_LTime) {
        qsort(smcrecs, num_mcrecs, sizeof(xModCollRec), CmpModCollLtime);
        xprintf("\nsort by ltime,ctime (live time)\n");
        xprintf("model-addr  no  ctime  ltime   frac   xent-name\n");

        for (i = 0, mc = smcrecs; i < MIN(10, num_mcrecs); i++, mc++) {
            xprintf("0x%08x %3d %6dK %7d %6f %s\n", mc->owner->model->Data, mc->nusing, (S32)(mc->ctime >> 10), (S32)mc->ltime, mc->frac,
                    xSceneID2Name(globals.sceneCur, mc->owner->id));
        }

        if (zPad::GetDebugPad()->Pressed(0x40000))
            mcmode = eModCollMode_None;
    } else {
        if (zPad::GetDebugPad()->Pressed(0x40000)) {
            mcmode = eModCollMode_CTime;
        }
    }

    for (i = 0, mc = mcrecs; i < num_mcrecs; i++, mc++) {
        mc->ltime = 0;
    }
}
#endif

void xEntAnimateCollision(xEnt& ent, bool on)
{
    if (on && !(ent.moreFlags & k_MORE_FLAGS_ANIM_COLL)) {
        ent.moreFlags |= k_MORE_FLAGS_ANIM_COLL;
        if (!ent.frame) {
            ent.frame = (xEntFrame*)xMEMALLOC_L(sizeof(xEntFrame), 0, eMemMgrTag_Anim, 0, 4505);
            memset(ent.frame, 0, sizeof(xEntFrame));
        }

        anim_coll::reset(ent);
    }
    else if (!on && (ent.moreFlags & k_MORE_FLAGS_ANIM_COLL))
        ent.moreFlags &= (U8)~k_MORE_FLAGS_ANIM_COLL;
}

bool xEntValidType(U8 type)
{
    return type == eBaseTypeTrigger || type == eBaseTypeVillain || type == eBaseTypePlayer || type == eBaseTypePickup ||
           type == eBaseTypePlatform || type == eBaseTypeDoor || type == eBaseTypeSavePoint || type == eBaseTypeItem ||
           type == eBaseTypeStatic || type == eBaseTypeDynamic || type == eBaseTypeBubble || type == eBaseTypePendulum ||
           type == eBaseTypeHangable || type == eBaseTypeButton || type == eBaseTypeProj || type == eBaseTypeProjBomb ||
           type == eBaseTypeDestructObj || type == eBaseTypeUI || type == eBaseTypeUIFont || type == eBaseTypeProjectileType ||
           type == eBaseTypeEGenerator || type == eBaseTypeNPCEntity || type == eBaseTypeBoulder || type == eBaseTypeTeleportBox ||
           type == eBaseTypeZipLine;
}

bool xEntTurnToFace(xEnt* ent, const xVec3* target, F32 speedLimit, F32 dt)
{
    xVec3 currentFacing = ent->frame->mat.at;
    xVec3Normalize(&currentFacing, &currentFacing);

    F32 dot = xVec3Dot(&currentFacing, target);
    if (dot < 0.9999f) {
        xVec3 axis;
        xVec3Cross(&axis, &currentFacing, target);
        xVec3Normalize(&axis, &axis);

        F32 angle = xacos(dot);
        F32 maxAngle = speedLimit * dt;
        if (angle > maxAngle) {
            angle = maxAngle;
        }

        if (xVec3Dot(&ent->frame->rot.axis, &axis) < 0.0f) {
            angle = -angle;
        }

        ent->frame->drot.axis.assign(0.0f, 0.0f, 0.0f);
        ent->frame->drot.angle = angle;
        ent->frame->mode |= 0x20;

        return false;
    }

    return true;
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("ent.asset != 0")
#endif

void xEntOpacityInit(xEntOpacity* opacity, xEnt* ent)
{
    if (ent->model) {
        opacity->startAlpha = ent->model->Alpha;
        opacity->endAlpha = ent->model->Alpha;
    } else {
        opacity->startAlpha = 0.0f;
        opacity->endAlpha = 0.0f;
    }

    opacity->deltaOpacity = 0.0f;
    opacity->oldPipeFlags = 0;
}

void xEntOpacityUpdate(xEntOpacity* opacity, xEnt* ent, F32 dt)
{
    if (!ent->model) {
        return;
    }

    if (ent->model->Alpha == opacity->endAlpha) {
        return;
    }

    if (0.0f == opacity->deltaOpacity) {
        return;
    }

    ent->model->Alpha += opacity->deltaOpacity * dt;

    if (opacity->startAlpha < opacity->endAlpha)
        ent->model->Alpha = CLAMP(ent->model->Alpha, opacity->startAlpha, opacity->endAlpha);
    else
        ent->model->Alpha = CLAMP(ent->model->Alpha, opacity->endAlpha, opacity->startAlpha);

    ent->model->Alpha = CLAMP(ent->model->Alpha, 0.0f, 1.0f);

    if (1.0f == ent->model->Alpha || 0.0f == ent->model->Alpha) {
        ent->model->Pipe.Flags &= ~opacity->oldPipeFlags;
    }
}

void xEntLoadCheckPoint()
{
}

void xEntStoreCheckPoint()
{
}

#if DEBUG || RELEASE
void on_select_debug_me(const tweak_info& info)
{
    gEnt_Debug = (xEnt*)info.context;
}

void on_change_active_model(const tweak_info& info)
{
    xEnt* this_ent = (xEnt*)info.context; xASSERT_L(4771, this_ent != 0);
    xModelInstance* active_model = this_ent->model;
    for (S32 i = 0; i < gEnt_ActiveModel; i++) {
        if (!active_model) break;
        if (!active_model->Next) { gEnt_ActiveModel = (S8)i; break; }
        active_model = active_model->Next;
    }
}

void on_change_show_vertex(const tweak_info& info)
{
    xEnt* this_ent = (xEnt*)info.context; xASSERT_L(4783, this_ent != 0);
    xModelInstance* active_model = this_ent->model;
    for (S32 i = 0; active_model && i < gEnt_ActiveModel; i++)
        active_model = active_model->Next;

    if (!active_model) gEnt_ShowVertex = -1;
    else {
        S16 verts = iModelVertCount(active_model->Data);
        if (gEnt_ShowVertex >= verts) gEnt_ShowVertex = (S16)(verts - 1);
    }
}
#endif
