#ifndef ZNPCASSET_H
#define ZNPCASSET_H

#include "xDynAsset.h"
#include "xEntAsset.h"

struct zScene;

// zNPCAssetRATS::flags
#define k_ZNPCASSETRATS_SPAWN_AT_START 0x1
#define k_ZNPCASSETRATS_NO_UPDATE_DISTANCE 0x2

struct zNPCAssetRATS : xDynAsset
{
    xEntAsset ent_asset;
    U32 lightKitID;
    U32 npcWalls;
    U32 npcPerception;
    F32 updateDistance;
    U32 flags;
};

void zNPCAsset_Startup();
S32 zNPCAsset_CountNPCs(S32* typeCount);
void zNPCAsset_CreateNPCs();
void zNPCAsset_RegisterNPCs(zScene*, S32* base_idx);
bool zNPCAsset_IsNPC(const xDynAsset& asset);

#endif
