#include "xCommon.h"
#include "zRatBallPlayer.h"

#include "zRatBallPlayerActions.h"
#include "zGlobals.h"
#include "zEvent.h"
#include "zCommonPlayer.h"
#include "zCutsceneMgr.h"
#include "xRumbleEmitter.h"
#include "xEntBoulder.h"
#include "xHierarchyBound.h"
#include "xModelAsset.h"
#include "zInteraction.h"
#include "xDebugTweak.h"
#include "xDraw.h"
#include "xColor.h"
#include "xString.h"

#include <stdio.h>

#define MAX_MULTIATOMICS_COUNT 12

F32 RB_PLSBOUND_RADIUS = 1.0f;
F32 RB_PLSBOUND_MIDRADIUS = 0.35f;
F32 RB_boundYShift = 0.2f;

namespace {

S32 lastUsedPadMRI = -1;
#if DEBUG || RELEASE
bool drawPlayerBounds;
#endif
xRumble::compositeEffect rumbleEffects[3];

xRumble::compositeEffect& GetRumble(U32 type); // UNUSED

} // namespace

void zRatBallPlayer::Exit()
{
    if (pad)
        lastUsedPadMRI = pad->GetPadPort();
    else
        lastUsedPadMRI = -1;

    zPlayer::Exit();
}

void zRatBallPlayer::Init(xEntAsset* asset)
{
    zPlayer::Init(asset);
    actionManager.Setup();

    if (pad)
        lastUsedPadMRI = pad->GetPadPort();

    Set_refract(true);
    Set_reflect(false);

    InitAtomicRenderOrder();

    prev_at = frame->mat.at;
    prev_bi_normal.x = prev_bi_normal.z = 0.0f;
    prev_bi_normal.y = 1.0f;
    last_sign = 0.0f;
}

void zRatBallPlayer::InitAtomicRenderOrder()
{
    atomics_count = 0;
    atomics_order = NULL;
    U32 assetID = asset->modelInfoID;
    atomics_order = (U32*)xMEMALLOC_L(MAX_MULTIATOMICS_COUNT * sizeof(U32), 0, eMemMgrTag_Model, 0, 225);

    for (U32 i = 0; i < MAX_MULTIATOMICS_COUNT; i++)
        atomics_order[i] = i;

    U16 size;
    xModelAssetParam* param = zEntGetModelParams(assetID, &size);
    while (param) {
        char buffer[256];
        sprintf(buffer, "Atomic%d", atomics_count);
        if (param->HashID == xStrHash(buffer)) {
            xASSERT_L(239, atomics_count < MAX_MULTIATOMICS_COUNT);
            buffer[0] = '\0';
            sprintf(buffer, "%s", param->String);
            U32 idx = buffer[0] - '0';
            xASSERT_L(243, idx >= 0 && idx < MAX_MULTIATOMICS_COUNT);
            atomics_order[atomics_count++] = idx;
        }

        param = zParamGetNext(param, &size);
    }
}

void zRatBallPlayer::SceneSetup()
{
    zPlayer::SceneSetup();
}

void zRatBallPlayer::Reset()
{
    actionManager.Reset();
    zPlayer::Reset();

    modelYaw = asset->ang.x;
    isOn = true;

#if DEBUG || RELEASE
    debugDrawDebugLines = true;
#endif
}

void zRatBallPlayer::DefaultBeginUpdate(F32 dt)
{
    S32 num_bones = model->Anim->BoneCount;
    xModelInstance* m = model;
    while (m) {
        for (S32 i = 0; i < num_bones; i++) {
            m->Mat[i + 1].pos.x -= bone_adjust.x;
            m->Mat[i + 1].pos.y -= bone_adjust.y;
            m->Mat[i + 1].pos.z -= bone_adjust.z;
        }

        m = m->Next;
    }

    Update_FlyCheatMode(dt);
    zPlayer::DefaultBeginUpdate(dt);

    xEntDriveUpdate(GetDrive(), sc, dt, NULL);

    BoundUpdate((xVec3*)&model->Mat->pos);

    xEntMove(this, sc, dt);

    xEntBoulder_ApplyForces(this, collis);

    xVec3Sub(&update_motion, &frame->mat.pos, &frame->oldmat.pos);
    UpdatePlayerDrive(dt);

    xASSERT_FMT_L(315, collis->idx <= k_XENT_MAX_COLL, "The collision index is off, which could potentially cause a bad array access. Please fix this immediately!");
}

void zRatBallPlayer::DefaultUpdate(F32 dt)
{
    zPlayer::DefaultUpdate(dt);
    CollideTrigger(sc);
    Update_Control();

#if DEBUG || RELEASE
    if (drawPlayerBounds)
        DrawBounds();
#endif

    Update_Particles(dt);

    Update_FX(dt);
}

void zRatBallPlayer::RenderEffects()
{
    zPlayer::RenderEffects();
}

static bool zRatBallSphereCollideCheck(const xSphere* sphere); // UNUSED

class BoundHitsObject
{
    const xSphere* sphere;
    bool* done;

public:
    BoundHitsObject(const xSphere* _sphere, bool* _done); // UNUSED
    bool operator()(xEnt& ent) const; // UNUSED
};

void zRatBallPlayer::BoundUpdateBottomSphere(F32 radius)
{
    xVec3Copy(&bound.sph.center, &frame->mat.pos);

    bound.sph.r = radius;

    bound.sph.center.AddScale(*(xVec3*)&model->Mat->up, bound.sph.r);
}

xVec3 zRatBallPlayer::GetBoundOffset()
{
    xVec3 offset;
    xVec3Sub(offset, frame->mat.pos, get_bone_world_position(0));
    return offset;
}

void zRatBallPlayer::BoundUpdateMidSphere(U32 chestBone, F32 radius)
{
    xSphere* midSphere = &extraSpheres[numExtraSpheres];

    numExtraSpheres++;

    midSphere->center = get_bone_world_position(chestBone);
    midSphere->center += GetBoundOffset();
    midSphere->r = radius;

    if (midSphere->center.y - midSphere->r < bound.sph.center.y - bound.sph.r)
        midSphere->center.y = midSphere->r + (bound.sph.center.y - bound.sph.r);
}

void zRatBallPlayer::BoundUpdate(xVec3*)
{
    BoundUpdateBottomSphere(RB_PLSBOUND_RADIUS);

    numExtraSpheres = 0;

    U32 animFlags = xEntGetAnimFlags(this), playerMove = animFlags & 0xF;

    if (numExtraSpheres + 1 <= MAX_EXTRA_SPHERES)
        BoundUpdateMidSphere(6, RB_PLSBOUND_MIDRADIUS);

    xBoundUpdate(&bound);
    QuickCullMergeExtraSpheres();
}

#if DEBUG || RELEASE
void zRatBallPlayer::SetupTweaks()
{
    char prefix[128];

    sprintf(prefix, "Player|Rafting|Bound|");

    xAUTOTWEAK(prefix, "RB_PLSBOUND_RADIUS", &RB_PLSBOUND_RADIUS, 0.0f, 1000000000.0f, NULL, NULL, 0, true);

    xTWEAK_BOOL("Player|Rafting|Draw Player Bounds", &drawPlayerBounds, NULL, NULL, 0);
    xTWEAK_BOOL("Player|Rafting|Debug Draw", &debugDrawDebugLines, NULL, NULL, 0);

    zPlayer::SetupTweaks();
}
#endif

void zRatBallPlayer::BeforeAnimMatrices(xAnimPlay*, xQuat*, xVec3*, S32)
{
}

void zRatBallPlayer::Update_Particles(F32)
{
}

void zRatBallPlayer::Update_FX(F32)
{
}

bool zRatBallPlayer::AllowInteraction(zInteraction*)
{
    if (currAction)
        return false;

    return true;
}

F32 zRatBallPlayer::InitialDepenetrationVelocity()
{
    if (((xEntGetAnimFlags(this) & 0x4000000) != 0) == true)
        return 20.0f;

    return zPlayer::InitialDepenetrationVelocity();
}

S32* zRatBallPlayer::GetHeadBones()
{
    return NULL;
}

void zRatBallPlayer::InitActions()
{
    actionManager.Init(1);
    actionManager.Add(this, new (eMemStaticType_0, eMemMgrTag_Player) zRatBallPlayerDrive);
}

xHierarchyBoundInitData* zRatBallPlayer::GetHierarchyBoundInitData() const
{
    static xHierarchyBoundInitData bounds[] = {
        { 6, 0.4f, 1 },
        { 4, 0.6f, 1 },
        { 2, 0.6f, 1 },
        { 42, 0.6f, 1 },
        { 46, 0.6f, 1 },
        { -2, 0.0f, 0 },
    };

    return bounds;
}

F32 zRatBallPlayer::GetHierarchyMasterRadius() const
{
    return 3.0f;
}

bool zRatBallPlayer::IsJumping() const
{
    return false;
}

#if DEBUG || RELEASE
const char** zRatBallPlayer::GetBoneNames() const
{
    static const char* gRatBallBoneNames[] = {
        "GROUP",
        "ROOT",
        "PELVIS",
        "WAIST",
        "CHEST",
        "NECK",
        "HEAD",
        "JAW",
        "LIP4",
        "LIP1",
        "LIP2",
        "LIP3",
        "EYEL",
        "EYER",
        "EYELID_UP",
        "EYELID_BOTTOM",
        "EYEBROWL",
        "EYEBROWR",
        "LARM1",
        "LARM2",
        "LARM3",
        "LARM_FINGER1A",
        "LARM_FINGER1B",
        "LARM_FINGER2A",
        "LARM_FINGER2A_LARM_FINGER2B",
        "LARM_FINGER3A",
        "LARM_FINGER3B",
        "LARM_FINGER4A",
        "LARM_FINGER4B",
        "LARM_FINGER_THUMB",
        "RARM1",
        "RARM2",
        "RARM3",
        "RARM_FINGER1A",
        "RARM_FINGER1B",
        "RARM_FINGER2A",
        "RARM_FINGER2A_LARM_FINGER2B",
        "RARM_FINGER3A",
        "RARM_FINGER3B",
        "RARM_FINGER4A",
        "RARM_FINGER4B",
        "RARM_FINGER_THUMB",
        "LEG1",
        "LEG2",
        "LEG3",
        "LEG4",
        "LEG5",
        "LEG6",
        "LEG7",
        "LEG8",
        NULL
    };

    return gRatBallBoneNames;
}
#endif

#if DEBUG || RELEASE
void zRatBallPlayer::DrawBounds() const
{
    if (Get_collisionOn())
        xDrawSetColor(0, 0, 0xFF, 0xFF);
    else
        xDrawSetColor(0, 0xFF, 0, 0xFF);

    if (Get_useBaseSphere())
        xDrawSphere(&bound.sph, 0xC0006);

    xDrawSetColor(g_MAGENTA);
    for (S32 i = 0; i < numExtraSpheres; i++)
        xDrawSphere(&extraSpheres[i], 0xC0006);
}
#endif

void zRatBallPlayer::Update_FlyCheatMode(F32)
{
    if (!pad)
        return;

    if (!globals.bAllowMasterCheats)
        return;

    if ((globals.enableFriendlyFly && pad->Pressed(0x200)) ||
        (!globals.enableFriendlyFly && pad->On(0x100) && pad->On(0x1000) && pad->Pressed(0x40))) {
        if (cheatMode) {
            collis->chk = k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_ENV;
            cheatMode = 0;
        } else {
            xVec3Copy(&frame->vel, &g_O3);
            collis->chk = k_XENT_COLLTYPE_NONE;
            cheatMode = 1;

            if (currAction) {
                zEntEvent(currAction, eEventStop);
                currAction = NULL;
            }
        }
    }
}

void zRatBallPlayer::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    switch (toEvent) {
    case eEventOff:
        isOn = false;
        return;
    case eEventOn:
        isOn = true;
        return;
    }

    zPlayer::HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}
