#include "xCommon.h"
#include "zCarryableObject.h"

#include "containers.h"
#include "xFont.h"
#include "xPad.h"
#include "zCarryableProperty.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zScene.h"

namespace {

void UpdateWrapper(xEnt* carryable, xScene*, F32 dt)
{
    ((zCarryableObject*)carryable)->Update(dt);
}

void EventWrapper(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                  U32 toParamWidgetID)
{
    ((zCarryableObject*)to)->HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}

} // namespace

zCarryableObject::zCarryableObject(zCarryableObjectAsset& asset) : carryAsset(asset)
{
    property = (const zCarryableProperty*)xSTFindAsset(asset.propertyAsset, NULL);

    U32 tempSize, assetID = xStrHashCat(asset.entAsset.modelInfoID, ".MINF");
    if (xSTFindAsset(assetID, &tempSize)) {
        asset.entAsset.modelInfoID = assetID;
    }

    carryAsset.entAsset.id = asset.id;
    carryAsset.entAsset.baseType = asset.baseType;
    carryAsset.entAsset.linkCount = asset.linkCount;
    carryAsset.entAsset.baseFlags = asset.baseFlags;

    zEntSimpleObj_Init(this, &carryAsset.entAsset, false, true);

    if (asset.linkCount)
        link = (xLinkAsset*)((U8*)&asset + sizeof(zCarryableObjectAsset));
    else
        link = NULL;

    update = UpdateWrapper;
    eventFunc = EventWrapper;
    flags |= k_XENT_IS_STACKED;
    carriedBy = NULL;
}

void zCarryableObject::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(71, asset, asset_size, zCarryableObjectAsset, "Carrying:Carryable Object");
    new (&data) zCarryableObject((zCarryableObjectAsset&)asset);
}

void zCarryableObject::Setup()
{
    zEntSimpleObj_Setup(this);
    xSceneAddEnt(globals.sceneCur, this);
}

void zCarryableObject::Reset()
{
    zEntSimpleObj_Reset(this, globals.sceneCur);
    update = UpdateWrapper;
    eventFunc = EventWrapper;
    flags |= k_XENT_IS_STACKED;
    carriedBy = NULL;
}

void zCarryableObject::Update(F32 dt)
{
    xEntUpdate(this, globals.sceneCur, dt);
}

void zCarryableObject::Drop()
{
    carriedBy = NULL;
    xEntDrop(this);
}

void zCarryableObject::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                                   U32 toParamWidgetId)
{
    switch (toEvent) {
    case eEventReset:
        Reset();
        break;
    default:
        zEntSimpleObjEventCB(from, this, toEvent, toParam, toParamWidget, toParamWidgetId);
        break;
    }
}
