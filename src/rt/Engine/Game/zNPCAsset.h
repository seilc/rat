#ifndef ZNPCASSET_H
#define ZNPCASSET_H

#include "xDynAsset.h"
#include "xEntAsset.h"

struct zNPCAssetRATS : xDynAsset
{
    xEntAsset ent_asset;
    U32 lightKitID;
    U32 npcWalls;
    U32 npcPerception;
    F32 updateDistance;
    U32 flags;
};

#endif
