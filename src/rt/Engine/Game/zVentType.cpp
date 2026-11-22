#include "xCommon.h"
#include "zVentType.h"

#include "zBase.h"
#include "zScene.h"

#include <string.h>

void zVentType::load(const asset_type& a)
{
    memset(this, 0, sizeof(zVentType));
    xBaseInit(this, &a);
    baseType = eBaseTypeVentType;
    asset = &a;
    if (linkCount) link = (xLinkAsset*)(&a + 1);

    if (asset->idleEmitter) {
        xBase* obj = zSceneFindObject(asset->idleEmitter);
        idleEmitter = (xParEmitter*)obj;
    }

    if (asset->warnEmitter) {
        xBase* obj = zSceneFindObject(asset->warnEmitter);
        warnEmitter = (xParEmitter*)obj;
    }

    if (asset->damageEmitter) {
        xBase* obj = zSceneFindObject(asset->damageEmitter);
        damageEmitter = (xParEmitter*)obj;
    }
}

void zVentType::load(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(50, asset, asset_size, zVentType::asset_type, zVentType::asset_type::type_name());
    ((zVentType&)data).load((const asset_type&)asset);
}
