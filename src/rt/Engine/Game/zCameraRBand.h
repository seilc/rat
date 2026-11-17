#ifndef ZCAMERARBAND_H
#define ZCAMERARBAND_H

#include "xBase.h"
#include "zCameraRBandAsset.h"

struct zCameraRBandObject : xBase
{
    zCameraRBandAsset* asset;
};

void zCameraRBandAsset_Init(xBase& data, xDynAsset& asset, size_t);
void zCameraRBandAsset_Init(zCameraRBandObject* object, zCameraRBandAsset* asset);
void zCameraRBandAsset_EventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);

#endif
