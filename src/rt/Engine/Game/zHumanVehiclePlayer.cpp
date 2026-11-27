#include "xCommon.h"
#include "zHumanVehiclePlayer.h"

#include "zHumanVehiclePlayerActions.h"
#include "zGlobals.h"
#include "zEvent.h"
#include "zCommonPlayer.h"
#include "zCutsceneMgr.h"
#include "zPickup.h"
#include "zShrapnel.h"
#include "xRumbleEmitter.h"
#include "xRumbleManager.h"
#include "xTRC.h"
#include "xSndMgr.h"
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

F32 PLSBOUND_RADIUS = 0.25f;
F32 PLSBOUND_MIDRADIUS = 0.35f;
F32 boundYShift = 0.1f;

namespace {

S32 lastUsedPadMRI = -1;
#if DEBUG || RELEASE
bool drawPlayerBounds;
#endif
xTWEAKABLE F32 LEAN_MIN_ANGLE = 1.0f * (PI / 180.0f);
xTWEAKABLE F32 LEAN_MAX_ANGLE = 3.0f * (PI / 180.0f);
xTWEAKABLE F32 LEAN_SMOOTH_CONSTANT = 20.0f;
xTWEAKABLE F32 LEAN_SMOOTH_MINRATE = 1.0f;
xTWEAKABLE F32 LEAN_MOVE_MAXDEFLECT = 0.5f;
xRumble::compositeEffect rumbleEffects[3];

xRumble::compositeEffect& GetRumble(U32 type); // UNUSED

} // namespace

void zHumanVehiclePlayer::Exit()
{
    if (pad)
        lastUsedPadMRI = pad->GetPadPort();
    else
        lastUsedPadMRI = -1;

    zPlayer::Exit();
}

void zHumanVehiclePlayer::SetupRumbles()
{
    rumbleEffects[0].asset.time = 0.2f;
    rumbleEffects[0].asset.intensity = 0.3f;
    rumbleEffects[0].asset.id = IDTAG('P','L','Y','R');
    rumbleEffects[0].asset.type = 3;
    rumbleEffects[0].asset.param1 = 0.1f;

    rumbleEffects[1].asset.time = 0.2f;
    rumbleEffects[1].asset.intensity = 0.5f;
    rumbleEffects[1].asset.id = IDTAG('P','L','Y','R');
    rumbleEffects[1].asset.type = 3;
    rumbleEffects[1].asset.param1 = 0.1f;

    rumbleEffects[2].asset.time = 0.2f;
    rumbleEffects[2].asset.intensity = 0.7f;
    rumbleEffects[2].asset.id = IDTAG('P','L','Y','R');
    rumbleEffects[2].asset.type = 3;
    rumbleEffects[2].asset.param1 = 0.1f;
}

void zHumanVehiclePlayer::Rumble(U32 rumbleType)
{
    if (Get_humanPlayer())
        xRumble::Manager::Get().Add(pad->GetXPad(), &rumbleEffects[rumbleType]);
}

void zHumanVehiclePlayer::Init(xEntAsset* asset)
{
    zPlayer::Init(asset);

    if (pad) {
        lastUsedPadMRI = pad->GetPadPort();
        if (xTRCPadCountWereActiveTotal() == 1)
            xPadRumbleEnable(lastUsedPadMRI, globals.option_vibration_p1);
    }

    SetupRumbles();

    Set_refract(true);
    Set_reflect(false);

    InitAtomicRenderOrder();

    prev_at = frame->mat.at;
    prev_bi_normal.x = prev_bi_normal.z = 0.0f;
    prev_bi_normal.y = 1.0f;
    last_sign = 0.0f;

    soundAsset[0] = xSndMgrGetSoundGroup(xStrHash("Lg_skid"));
    soundAsset[1] = xSndMgrGetSoundGroup(xStrHash("Lg_collide"));
}

void zHumanVehiclePlayer::InitAtomicRenderOrder()
{
    atomics_count = 0;
    atomics_order = NULL;
    U32 assetID = asset->modelInfoID;
    atomics_order = (U32*)xMEMALLOC_L(MAX_MULTIATOMICS_COUNT * sizeof(U32), 0, eMemMgrTag_Model, 0, 265);

    for (U32 i = 0; i < MAX_MULTIATOMICS_COUNT; i++)
        atomics_order[i] = i;

    U16 size;
    xModelAssetParam* param = zEntGetModelParams(assetID, &size);
    while (param) {
        char buffer[256];
        sprintf(buffer, "Atomic%d", atomics_count);
        if (param->HashID == xStrHash(buffer)) {
            xASSERT_L(279, atomics_count < MAX_MULTIATOMICS_COUNT);
            buffer[0] = '\0';
            sprintf(buffer, "%s", param->String);
            U32 idx = buffer[0] - '0';
            xASSERT_L(283, idx >= 0 && idx < MAX_MULTIATOMICS_COUNT);
            atomics_order[atomics_count++] = idx;
        }

        param = zParamGetNext(param, &size);
    }
}

void zHumanVehiclePlayer::SceneSetup()
{
    zPlayer::SceneSetup();
}

void zHumanVehiclePlayer::Reset()
{
    actionManager.Reset();
    contextManager.Reset();
    zPlayer::Reset();

    modelYaw = asset->ang.x;
    xModelSetScale(model, 1.0f / 12.0f);
    soundSkidLoopHandle = XSNDMGR_INVALID_SND_HANDLE;
    leanLerp = 0.0f;
    isOn = true;
    velocity = xVec3::m_Null;
    hitTimer = 0.0f;

#if DEBUG || RELEASE
    debugDrawDebugLines = false;
#endif
}

void zHumanVehiclePlayer::DefaultBeginUpdate(F32 dt)
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

    xASSERT_FMT_L(353, collis->idx <= k_XENT_MAX_COLL, "The collision index is off, which could potentially cause a bad array access. Please fix this immediately!");
}

void zHumanVehiclePlayer::DefaultUpdate(F32 dt)
{
    zPickup::TryBeGrabbedAll(this);

    zPlayer::DefaultUpdate(dt);

#if DEBUG || RELEASE
    if (!xDebugIDontWantYourXprintsOnMyScreen())
        xprintf("%s %.4f\n", model->Anim->Single->State->Name, model->Anim->Single->Time);
#endif

    CollideTrigger(sc);
    Update_Control();

#if DEBUG || RELEASE
    if (drawPlayerBounds)
        DrawBounds();
#endif

    Update_Particles(dt);

    Update_FX(dt);

    hitTimer -= dt;
}

void zHumanVehiclePlayer::RenderEffects()
{
    zPlayer::RenderEffects();
}

static bool zHumanVehicleSphereCollideCheck(const xSphere* sphere); // UNUSED

class BoundHitsObject
{
    const xSphere* sphere;
    bool* done;

public:
    BoundHitsObject(const xSphere* _sphere, bool* _done); // UNUSED
    bool operator()(xEnt& ent) const; // UNUSED
};

void zHumanVehiclePlayer::BoundUpdateBottomSphere(F32 radius)
{
    xVec3Copy(&bound.sph.center, &frame->mat.pos);

    bound.sph.r = radius;

    GetCenter(&bound.sph.center);
}

void zHumanVehiclePlayer::BoundUpdateMidSphere(U32, F32 radius)
{
    xSphere* midSphere = &extraSpheres[numExtraSpheres];

    numExtraSpheres++;

    xModelGetBoneLocationScaled(midSphere->center, *model, 3);
    midSphere->r = radius;
}

void zHumanVehiclePlayer::BoundUpdate(xVec3*)
{
    BoundUpdateBottomSphere(PLSBOUND_RADIUS);

    numExtraSpheres = 0;

    U32 animFlags = xEntGetAnimFlags(this), playerMove = animFlags & 0xF;

    if (numExtraSpheres + 4 <= MAX_EXTRA_SPHERES) {
        BoundUpdateMidSphere(3, PLSBOUND_MIDRADIUS);
        BoundUpdateMidSphere(9, PLSBOUND_RADIUS);
        BoundUpdateMidSphere(21, PLSBOUND_RADIUS);
        BoundUpdateMidSphere(36, PLSBOUND_RADIUS);
    }

    xBoundUpdate(&bound);
    QuickCullMergeExtraSpheres();
}

#if DEBUG || RELEASE
void zHumanVehiclePlayer::SetupTweaks()
{
    char prefix[128];

    sprintf(prefix, "Player|Human Vehicle|Bound|");

    xAUTOTWEAK(prefix, "PLSBOUND_RADIUS", &PLSBOUND_RADIUS, 0.0f, 1000000000.0f, NULL, NULL, 0, true);

    xTWEAK_BOOL("Player|Human Vehicle|Draw Player Bounds", &drawPlayerBounds, NULL, NULL, 0);
    xTWEAK_BOOL("Player|Human Vehicle|Debug Draw", &debugDrawDebugLines, NULL, NULL, 0);

    zPlayer::SetupTweaks();
}
#endif

void zHumanVehiclePlayer::BeforeAnimMatrices(xAnimPlay*, xQuat*, xVec3*, S32)
{
}

void zHumanVehiclePlayer::Update_Particles(F32)
{
}

void zHumanVehiclePlayer::Update_FX(F32)
{
}

bool zHumanVehiclePlayer::AllowInteraction(zInteraction*)
{
    if (currAction)
        return false;

    return true;
}

F32 zHumanVehiclePlayer::InitialDepenetrationVelocity()
{
    if (((xEntGetAnimFlags(this) & 0x4000000) != 0) == true)
        return 20.0f;

    return zPlayer::InitialDepenetrationVelocity();
}

S32* zHumanVehiclePlayer::GetHeadBones()
{
    return NULL;
}

void zHumanVehiclePlayer::InitActions()
{
    actionManager.Init(1);
    actionManager.Add(this, new (eMemStaticType_0, eMemMgrTag_Player) zHumanVehiclePlayerDrive);
}

xHierarchyBoundInitData* zHumanVehiclePlayer::GetHierarchyBoundInitData() const
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

F32 zHumanVehiclePlayer::GetHierarchyMasterRadius() const
{
    return 3.0f;
}

bool zHumanVehiclePlayer::IsJumping() const
{
    return false;
}

#if DEBUG || RELEASE
const char** zHumanVehiclePlayer::GetBoneNames() const
{
    static const char* gHumanVehicleBoneNames[] = {
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

    return gHumanVehicleBoneNames;
}
#endif

void zHumanVehiclePlayer::GetCenter(xVec3* pOutput)
{
    pOutput->AddScale(frame->mat.pos, frame->mat.up, boundYShift);
}

#if DEBUG || RELEASE
void zHumanVehiclePlayer::DrawBounds() const
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

void zHumanVehiclePlayer::Update_FlyCheatMode(F32)
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

void zHumanVehiclePlayer::LeanUpdate(F32 diffAngle, F32 dt)
{
    F32 desiredLean, diffLerp, diffAbs = iabs(diffAngle);

    if (diffAbs < LEAN_MIN_ANGLE)
        desiredLean = 0.0f;
    else if (diffAbs > LEAN_MAX_ANGLE)
        desiredLean = 1.0f;
    else
        desiredLean = (diffAbs - LEAN_MIN_ANGLE) * (1.0f / (LEAN_MAX_ANGLE - LEAN_MIN_ANGLE));

    if (diffAngle > 0.0f)
        desiredLean = -desiredLean;

    if ((model->Anim->Single->State->UserFlags & 0xF) == 2 || (model->Anim->Single->State->UserFlags & 0xF) == 10 ||
        (model->Anim->Single->State->UserFlags & 0xF) == 3 || (model->Anim->Single->State->UserFlags & 0xF) == 12 ||
        (model->Anim->Single->State->UserFlags & 0xF) == 11)
        desiredLean *= LEAN_MOVE_MAXDEFLECT;

    desiredLean += 1.0f;

    diffLerp = dt * (LEAN_SMOOTH_CONSTANT * (desiredLean - leanLerp));
    if (diffLerp > 0.0f) {
        if (diffLerp < LEAN_SMOOTH_MINRATE * dt)
            diffLerp = LEAN_SMOOTH_MINRATE * dt;

        leanLerp += diffLerp;
        if (leanLerp > desiredLean)
            leanLerp = desiredLean;
    }
    else if (diffLerp < 0.0f) {
        if (diffLerp > -LEAN_SMOOTH_MINRATE * dt)
            diffLerp = -LEAN_SMOOTH_MINRATE * dt;

        leanLerp += diffLerp;
        if (leanLerp < desiredLean)
            leanLerp = desiredLean;
    }
}

void zHumanVehiclePlayer::ParseIni()
{
    zPlayer::ParseIni();

    F32 DEFAULT_ANIM[3] = { 3.0f, 0.5f, 2.5f };
    GetParameterList("AnimWalk", animWalk, 3, DEFAULT_ANIM);
    GetParameterList("AnimRun", animRun, 3, DEFAULT_ANIM);
}

void zHumanVehiclePlayer::HVCalcAnimSpeed(F32 mag, F32* animSpeeds)
{
    CalcAnimSpeed(mag, animSpeeds);
}

void zHumanVehiclePlayer::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    switch (toEvent) {
    case eEventOff:
        isOn = false;
        Set_visible(false);
        return;
    case eEventOn:
        isOn = true;
        Set_visible(true);
        return;
    case eEventHit:
        break;
    }

    zPlayer::HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}

bool zHumanVehiclePlayer::Damage(const zCombatDamageInfo& damageInfo)
{
    if (IsDead() || cheatMode || !globals.settings.TakeDamage)
        return false;

    if (controlOffTimer > 0.0f && !(damageInfo.flags & 0x2))
        return false;

    if (hitTimer > 0.0f)
        return false;

    hitTimer = globals.settings.DamageInvincibility;

    zHitSource hitSource = damageInfo.source;
    bool force_knockback = (damageInfo.flags & 0x1) || hitSource == zHS_KNOCKBACK;

    if (force_knockback)
        velocity = damageInfo.knockback;

    return zPlayer::Damage(damageInfo);
}
