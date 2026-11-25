#include "xCommon.h"
#include "zPushableObject.h"

#include "containers.h"
#include "xDebugTweak.h"
#include "xFont.h"
#include "xMemory.h"
#include "xPad.h"
#include "zBase.h"
#include "zEnt.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zScene.h"

namespace
{
    xTWEAKABLE F32 SPEED_KNOCKBACK = 8.0f;
    xTWEAKABLE F32 GRID_CELL_SIZE = 1.0f;
    xTWEAKABLE F32 DECEL = 0.5f * xsqr(SPEED_KNOCKBACK) / GRID_CELL_SIZE;

#if DEBUG || RELEASE
    void CalcDecel(const tweak_info&)
    {
        DECEL = 0.5f * xsqr(SPEED_KNOCKBACK) / GRID_CELL_SIZE;
    }
#endif

    void MoveWrapper(xEnt* ent, xScene* scene, F32 dt, xEntFrame* frame)
    {
        ((zPushableObject*)ent)->Move(scene, dt, frame);
    }

    void EventWrapper(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
    {
        ((zPushableObject*)to)->HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
    }
} // namespace

#if DEBUG || RELEASE
void zPushableObject::AddStaticTweaks()
{
    static tweak_callback calcDecelCB = { CalcDecel };
    const char* prefix = "Player|Context Objects|Push/Pull|";
    xAUTOTWEAK(prefix, "SPEED_KNOCKBACK", &SPEED_KNOCKBACK, 0.0f, 100.0f, &calcDecelCB, NULL, 0, true);
    xAUTOTWEAK(prefix, "GRID_CELL_SIZE", &GRID_CELL_SIZE, 0.0f, 100.0f, &calcDecelCB, NULL, 0, true);
}
#endif

F32 zPushableObject::GetGridCellSize()
{
    return GRID_CELL_SIZE;
}

bool zPushableObject::CheckPos(const xVec3& destPos)
{
    xMat4x3 matrix;
    xMat3x3Copy(&matrix, &frame->mat);
    matrix.pos = destPos;
    return !xOBBHitsScene(bound.box.box, matrix, globals.sceneCur, this, k_XENT_COLLTYPE_PC,
                          k_XENT_COLLTYPE_ENV | k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_STAT);
}

zPushableObject::zPushableObject(const Asset& theAsset) : asset(theAsset)
{
    static xEntAsset fakeAsset;
    static bool needInit = true;
    if (needInit) {
        xMemorySetV32A32(&fakeAsset, 0, sizeof(fakeAsset));
        fakeAsset.baseType = eBaseTypePushableObject;
        fakeAsset.baseFlags = k_XBASE_IS_ENABLED | k_XBASE_IS_VISIBLE_IN_CUTSCENES;
        fakeAsset.flags = k_XENT_IS_VISIBLE | k_XENT_NO_SHADOW;
        fakeAsset.redMult = 1.0f;
        fakeAsset.greenMult = 1.0f;
        fakeAsset.blueMult = 1.0f;
        fakeAsset.scale.assign(1.0f);
        fakeAsset.seeThru = 1.0f;
        fakeAsset.modelInfoID = asset.modelID;
        needInit = false;
    }

    fakeAsset.id = asset.id;

    xEntInit(this, &fakeAsset);

    zEntParseModelInfo(this, asset.modelID);
    xEntSetup(this);
    collType = k_XENT_COLLTYPE_DYN;
    collLev = k_XENT_COLLLEV_MODEL;

    bound.type = k_XBOUNDTYPE_OBB;
    bound.mat = (xMat4x3*)model->Mat;
    xEntInitForType(this);

    eventFunc = EventWrapper;
    move = MoveWrapper;

    Reset();
}

void zPushableObject::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(113, asset, asset_size, Asset, "Context Object:Pushable Object");
    new (&data) zPushableObject((Asset&)asset);
}

F32 zPushableObject::RoundToGrid(F32 pos)
{
    F32 grid = pos / GRID_CELL_SIZE;
    S32 cell;

    if (pos > 0.0f) {
        cell = grid + 0.5f;
    } else {
        cell = grid - 0.5f;
    }

    return cell * GRID_CELL_SIZE;
}

void zPushableObject::Move(xScene*, F32 dt, xEntFrame* frame)
{
    if (speed <= 0.0f) {
        return;
    }

    F32 newSpeed = speed - DECEL * dt;
    if (newSpeed > 0.0f) {
        frame->dpos.Scale(dir, 0.5f * dt * (newSpeed + speed));
        speed = newSpeed;
    } else {
        speed = 0.0f;
        xASSERT_FMT_L(148, xfeq0( dir.x ) || xfeq0( dir.z ), "Dir should be in one of the cardinal directions");
        if (xfeq0(dir.x)) {
            F32 gridPos = RoundToGrid(frame->mat.pos.z);
            frame->dpos.z = gridPos - frame->mat.pos.z;
        } else {
            F32 gridPos = RoundToGrid(frame->mat.pos.x);
            frame->dpos.x = gridPos - frame->mat.pos.x;
        }
    }

    frame->mode |= k_XENT_SET_DPOS;
}

void zPushableObject::Reset()
{
    xVec3 pos = { asset.locationX, asset.locationY, asset.locationZ };
    xEntReset(this, pos, xVec3::m_Null);
    speed = 0.0f;
}

void zPushableObject::EntityHit(xEnt* hit)
{
    if (!xfeq0(speed)) {
        return;
    }

    xVec3 diff;
    diff.Sub(frame->mat.pos, hit->frame->mat.pos);
    F32 atDot = frame->mat.at.dot(diff);
    F32 rightDot = frame->mat.right.dot(diff);
    if (xabs(atDot) > xabs(rightDot)) {
        dir = frame->mat.at;
        if (atDot < 0.0f) {
            dir.negate();
        }
    } else {
        dir = frame->mat.right;
        if (rightDot < 0.0f) {
            dir.negate();
        }
    }

    xVec3 destPos;
    destPos.AddScale(frame->mat.pos, dir, GRID_CELL_SIZE);
    if (CheckPos(destPos)) {
        speed = SPEED_KNOCKBACK;
    }
}

void zPushableObject::HandleEvent(xBase* from, U32 toEvent, const F32*, xBase*, U32)
{
    switch (toEvent) {
    case eEventHit:
        EntityHit((xEnt*)from);
        break;
    case eEventReset:
        Reset();
    }
}
