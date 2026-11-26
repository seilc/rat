#include "xCommon.h"
#include "zSpringboard.h"

#include "xAnim.h"
#include "xCollideSweptSphere.h"
#include "xMemory.h"
#include "xModel.h"
#include "zBase.h"
#include "zContextObject.h"
#include "zEnt.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zPlayer.h"
#include "zScene.h"

namespace
{
    xTWEAKABLE F32 SMALL_LAUNCH_HEIGHT = 6.0f;
    xTWEAKABLE F32 SIZE_SPACING = 3.0f;
    xTWEAKABLE F32 TOP_DIST_MAX = 0.05f;
    xTWEAKABLE F32 NORMAL_COS = 0.7f;

    void GenericEntRender(xEnt* ent)
    {
        if (!ent->model || !xEntIsVisible(ent)) {
            return;
        }

        xModelRender(ent->model);
    }
} // namespace

void zSpringboard::Launch()
{
    xASSERT_FMT_L(53, state == Idle && asset->type == zSpringboardAsset::BouncePad || state == Compressing && asset->type == zSpringboardAsset::Trampoline || ( state == Compressed || state == Compressing ) && asset->type == zSpringboardAsset::Chargeboard, "State is wrong when starting to launch from a springboard. This will break things");
    state = Launching;
}

void zSpringboard::Compress()
{
    xASSERT_FMT_L(59, state == Idle || state == Launching, "State is wrong when starting to compress a springboard. This will break things");
    state = Compressing;
}

U32 zSpringboard::CompressCheck(xAnimTransition*, xAnimSingle*)
{
    return state == Compressing;
}

U32 zSpringboard::IdleCB(xAnimTransition*, xAnimSingle*)
{
    state = Idle;
    return 0;
}

U32 zSpringboard::LaunchCheck(xAnimTransition*, xAnimSingle*)
{
    return state == Launching;
}

U32 zSpringboard::LaunchCB(xAnimTransition*, xAnimSingle*)
{
    state = Launching;
    return 0;
}

U32 zSpringboard::ChargeboardCheck(xAnimTransition*, xAnimSingle*)
{
    return asset->type == zSpringboardAsset::Chargeboard;
}

U32 zSpringboard::CompressedCB(xAnimTransition*, xAnimSingle*)
{
    return state = Compressed;
}

xAnimTable* zSpringboard::CreateAnimTable()
{
    xAnimTable* table = xAnimTableNew("Springboard", 0);

    xAnimTableNewState(table, "SpringboardIdle01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "SpringboardCompress01", AnimState_EndTransition, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "SpringboardCompressed01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "SpringboardLaunch01", AnimState_EndTransition, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);

    xAnimTableNewTransition(table, "SpringboardLaunch01", "SpringboardCompress01", anCompressCheck, NULL, 0, 0, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SpringboardIdle01", "SpringboardCompress01", anCompressCheck, NULL, 0, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "SpringboardIdle01", "SpringboardLaunch01", anLaunchCheck, NULL, 0, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "SpringboardCompress*", "SpringboardLaunch01", anLaunchCheck, NULL, 0, 0, 0.0f, 0.0f, 0, 0, 0.15f, NULL);

    xAnimTableNewTransition(table, "SpringboardCompress01", "SpringboardCompressed01", anChargeboardCheck, anCompressedCB, AnimTransition_Default, 0, 0.0f, 0.0f, 1, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "SpringboardCompress01", "SpringboardLaunch01", NULL, anLaunchCB, AnimTransition_Default, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "SpringboardLaunch01", "SpringboardIdle01", NULL, anIdleCB, AnimTransition_Default, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);

    return table;
}

void zSpringboard::EventCB(xBase*, U32 toEvent, const F32*, xBase*, U32)
{
    switch (toEvent) {
    case eEventReset:
        Reset();
        break;
    }
}

void zSpringboard::EventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    xASSERT_FMT_L(130, to->baseType == eBaseTypeSpringboard, "Error: expecting a springboard object");

    if (toEvent == eEventContextFX) {
        zContextObjectManager::AddEntFX((xEnt*)to);
    }

    ((zSpringboard*)to)->EventCB(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}

void zSpringboard::Reset()
{
    xVec3 ang = { asset->yaw, asset->pitch, asset->roll };
    xEntReset(this, asset->location, ang);
}

zSpringboard::zSpringboard(zSpringboardAsset* theAsset) : asset(theAsset)
{
    static xEntAsset fakeAsset;
    xMemorySetV32A32(&fakeAsset, 0, sizeof(fakeAsset));
    fakeAsset.id = asset->id;
    fakeAsset.baseType = eBaseTypeSpringboard;
    fakeAsset.baseFlags = k_XBASE_IS_ENABLED | k_XBASE_IS_VISIBLE_IN_CUTSCENES;
    fakeAsset.flags = k_XENT_IS_VISIBLE | k_XENT_NO_SHADOW;
    fakeAsset.redMult = 1.0f;
    fakeAsset.greenMult = 1.0f;
    fakeAsset.blueMult = 1.0f;
    fakeAsset.scale.assign(1.0f);
    fakeAsset.seeThru = 1.0f;
    fakeAsset.modelInfoID = asset->modelID;

    xEntInit(this, &fakeAsset);
    miscflags |= 0x40;
    U32 assetID = xStrHashCat(asset->modelID, ".MINF");
    zEntParseModelInfo(this, assetID);
    xEntSetup(this);
    collType = k_XENT_COLLTYPE_STAT;
    collLev = k_XENT_COLLLEV_MODEL;

    bound.type = k_XBOUNDTYPE_OBB;
    bound.mat = (xMat4x3*)model->Mat;
    xEntInitForType(this);

    update = (xEntUpdateCallback)zEntUpdate;
    eventFunc = EventCB;
    render = GenericEntRender;
    xSceneAddEnt(globals.sceneCur, this);

    U16 size;
    xModelAssetParam* param = zEntGetModelParams(assetID, &size);
    angle = zParamGetFloat(param, size, "LaunchAngle", 90.0f);
    F32 angleRadians = (PI / 180.0f) * angle;
    cosAngle = icos(angleRadians);
    sinAngle = isin(angleRadians);
    Reset();
}

void zSpringboard::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(189, asset, asset_size, zSpringboardAsset, "Context Object:Springboard");
    new (&data) zSpringboard((zSpringboardAsset*)&asset);
}

void zSpringboard::GetLaunchVelocity(xVec3& vel)
{
    xASSERT_FMT_L(195, xabs( asset->adjustment ) <= SIZE_SPACING / 2.0f, "The adjustment factor is too large, so \t\tit is changing the size catagory of the object. Please get latest widgets.xml or inform a \t\tprogrammer to update it");
    F32 height = asset->adjustment + (SMALL_LAUNCH_HEIGHT + SIZE_SPACING * asset->size);
    F32 magnitude = xsqrt(-2.0f * globals.sceneCur->gravity * height);
    vel.Scale(((xMat4x3*)model->Mat)->up, sinAngle * magnitude);
    vel.AddScale(((xMat4x3*)model->Mat)->at, cosAngle * magnitude);
}

bool zSpringboard::Mount(const SphereCollisionResults& results)
{
    if (results.sphereIndex != 0) {
        return false;
    }

    if (!asset->top) {
        return true;
    }

    xASSERT_FMT_L(214, bound.type == k_XBOUNDTYPE_OBB, "Why isn't this springboard with an OBB?");
    if (results.normal.dot(bound.mat->up) < NORMAL_COS) {
        return false;
    }

    xVec3 cornerPos;
    xMat4x3ToWorld(&cornerPos, bound.mat, &bound.box.box.upper);
    F32 d = bound.mat->up.dot(cornerPos);
    F32 playerD = results.contact.dot(bound.mat->up);
    return xabs(d - playerD) < TOP_DIST_MAX;
}
