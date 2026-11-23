#include "xCommon.h"
#include "zPlayerLocationPlatform.h"

#include "xEnt.h"
#include "xGlobals.h"
#include "xPad.h"
#include "zINPC.h"
#include "zScene.h"

zPlayerLocationPlatformConnector* zPlayerLocationPlatformConnector::head;

zPlayerLocationPlatformConnector::zPlayerLocationPlatformConnector(zPlayerLocationPlatformConnectorAsset* theAsset)
{
    xBaseInit(this, theAsset);

    asset = theAsset;

    xASSERT_FMT_L(35, theAsset->platformId, "The player location platform \"%s\" doesn't have a platform specified", xSceneID2Name(xglobals->sceneCur, theAsset->id));
    platform = (xEnt*)zSceneFindObject(theAsset->platformId);
    // BUG: prints playerId, not platformId
    xASSERT_FMT_L(37, platform, "The player location platform \"%s\" can't find its platform widget (ID=0x%08x)", xSceneID2Name(xglobals->sceneCur, theAsset->id), theAsset->playerId);

    eventFunc = NULL;

    if (linkCount)
        link = (xLinkAsset*)((U8*)theAsset + sizeof(zPlayerLocationPlatformConnectorAsset));
    else
        link = NULL;

    next = NULL;

    firstUpdate = true;

    if (head == NULL) {
        head = this;
    } else {
        zPlayerLocationPlatformConnector* cur = head;
        while (cur->next != NULL) {
            cur = cur->next;
        }
        cur->next = this;
    }
}

void zPlayerLocationPlatformConnector::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(68, asset, asset_size, zPlayerLocationPlatformConnectorAsset, "logic:Player Location Platform Connector");
    new (&data) zPlayerLocationPlatformConnector((zPlayerLocationPlatformConnectorAsset*)&asset);
}

void zPlayerLocationPlatformConnector::Update(F32)
{
    if (firstUpdate) {
        xASSERT_FMT_L(77, asset->playerId, "The player location platform \"%s\" doesn't have a player specified", xSceneID2Name(xglobals->sceneCur, asset->id));
        xBase* tbase = zSceneFindObject(asset->playerId);
        xASSERT_FMT_L(79, tbase, "The player location platform \"%s\" can't find its player widget (ID=0x%08x)", xSceneID2Name(xglobals->sceneCur, asset->id), asset->playerId);

        if (tbase->baseType == eBaseTypePlayer) {
            attachEnt = (xEnt*)tbase;
        }
        else if (zINPC::IsNPCBase(tbase)) {
            attachEnt = zINPC::GetNPCEnt((zNPCBase*)tbase);
        }
        else if (zINPC::IsNPCEntity(tbase)) {
            attachEnt = (xEnt*)tbase;
        } else {
            xASSERT_L(95, 0 &&"Unknown entity type for Player Location Platform");
        }

        firstUpdate = false;
    }

    if (platform->frame) {
        platform->frame->mat = *xEntGetFrame(attachEnt);
    }

    if (platform->model) {
        *(xMat4x3*)platform->model->Mat = *xEntGetFrame(attachEnt);
    }
}

void zPlayerLocationPlatformConnector::UpdateAll(F32 dt)
{
    zPlayerLocationPlatformConnector* cur = head;
    while (cur) {
        cur->Update(dt);
        cur = cur->next;
    }
}

void zPlayerLocationPlatformConnector::Exit()
{
    head = NULL;
}
