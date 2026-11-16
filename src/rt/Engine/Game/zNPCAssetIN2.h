#ifndef ZNPCASSETIN2_H
#define ZNPCASSETIN2_H

#include "xDynAsset.h"
#include "xEntAsset.h"

struct zNPCAssetIN2 : xDynAsset
{
    xEntAsset ent_asset;
    U32 navigation_mesh_id;
    U32 npcFlags;
    U32 respawnCounterID;
    U32 settingsHashID;
    U32 interestPointerGroupID;
    U32 lightKitID;
};

#endif
