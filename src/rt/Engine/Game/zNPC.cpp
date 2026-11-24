#include "xCommon.h"
#include "zNPC.h"

#include "containers.h"
#include "xCam.h"
#include "xEntMotion.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xPtankPool.h"
#include "zMeshPosition.h"
#include "zNPCBaseIN2.h"
#include "zNPCCommon.h"
#include "zNPCLevelSpecific.h"
#include "zNPCNavigation.h"
#include "zScene.h"

#include <string.h>

namespace zNPC {

void zNPCInstance::set_data(zNPCBaseIN2* data)
{
    data->change_instance(this);
    npc_data = data;
}

zNPCInstance::zNPCInstance(const xModelAssetInfo* model_asset, U32 type, zNPCAssetIN2* npc_asset, S32 npc_asset_size)
    : type(type), npc_asset(npc_asset), npc_asset_size(npc_asset_size), model_asset(model_asset)
{
    flags.visibilityCull = true;

    canAttack = canDetect = canChase = true;

    hasNuke = false;

    ignoreAllDamage = false;

    noMoreSpawning = false;

    npc_data = NULL;
    xBaseInit(this, &npc_asset->ent_asset);
    reset_last_frame();
    storedSpawn.valid = FALSE;

    baseFlags = k_XBASE_IS_ENABLED | k_XBASE_IS_VALID | k_XBASE_IS_VISIBLE_IN_CUTSCENES | k_XBASE_RECEIVES_SHADOWS;

    if (npc_asset->ent_asset.baseFlags & k_XBASE_IS_ENABLED) {
        flags.activated = true;
        flags.living = true;
    } else {
        flags.activated = false;
        flags.living = false;
    }

    moveRadiusCenter = npc_asset->ent_asset.pos;

    F32 defaultMovementRadius = 0.0f;
    if (gNPCGameLevel == ezGAMELEVEL_A10x || gNPCGameLevel == ezGAMELEVEL_A30x)
        SetMovementRadius(1000.0f);
    else
        SetMovementRadius(40.0f);
    hasManualMoveRadiusCenter = false;

    if (linkCount < 1) {
        link = NULL;
    } else {
        char* ptr = (char*)this->npc_asset;
        ptr += this->npc_asset_size;
        S32 amt_linkdata = linkCount * sizeof(xLinkAsset);
        xASSERT_FMT_L(94, ((amt_linkdata % 4)==0), "0x%08X", id);
        amt_linkdata = (amt_linkdata + 3) & ~3;
        link = (xLinkAsset*)(ptr - amt_linkdata);
    }
}

void zNPCInstance::Reset()
{
    canAttack = canDetect = canChase = true;
    hasNuke = false;
    ignoreAllDamage = false;
}

void zNPCInstance::SetMovementRadius(F32 inRadius)
{
    moveRadius2 = inRadius;
    moveRadius2 *= moveRadius2;
    moveInnerRadius2 = 0.5f * inRadius;
    moveInnerRadius2 *= moveInnerRadius2;
}

void zNPCInstance::SetManualMovementRadiusCenter(xVec3* inCenter)
{
    hasManualMoveRadiusCenter = true;
    manualMoveRadiusCenter = *inCenter;
    moveRadiusCenter = manualMoveRadiusCenter;
}

void zNPCInstance::ExitingSpawnState()
{
    if (npc_data) {
        if (hasManualMoveRadiusCenter)
            moveRadiusCenter = manualMoveRadiusCenter;
        else
            moveRadiusCenter = *xEntGetPos(npc_data);

        zNPCCommon* npc_common = (zNPCCommon*)npc_data;
        zMeshPosition localMeshPos = npc_common->navigation_behavior->GetMeshPosition();
        bool result = localMeshPos.UpdatePositionAutoY(moveRadiusCenter, 0x400);
        xASSERT_FMT_L(139, result, "Why cant we find the navmesh after spawning for %s?", xSceneID2Name(xglobals->sceneCur, id));
        xVec3Copy(&moveRadiusCenter, &localMeshPos.GetPoint());
    }
}

F32 zNPCInstance::get_update_dist2() const
{
    F32 updateDist = zNPCLevelSpecificGetUpdateDistance();
    return xsqr(updateDist);
}

F32 zNPCInstance::get_surrender_dist2() const
{
    F32 surrenderDist = zNPCLevelSpecificGetSurrenderDistance();
    return xsqr(surrenderDist);
}

// UNUSED: needed to match (code from RotU GC)
bool zNPCInstance::StoreSpawnInfo(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    xASSERT_FMT(npc_data, "an npc is spawning without having data.  This should never happen, contact programming.");

    if (storedSpawn.valid) {
        // BUG: self-compares, and toParam[3] twice
        bool result = storedSpawn.from == from && storedSpawn.to == to && storedSpawn.toEvent == toEvent &&
                      storedSpawn.toParamWidget == storedSpawn.toParamWidget &&
                      storedSpawn.toParamWidgetID == storedSpawn.toParamWidgetID &&
                      storedSpawn.toParam[0] == toParam[0] && storedSpawn.toParam[1] == toParam[1] &&
                      storedSpawn.toParam[3] == toParam[3] && storedSpawn.toParam[3] == toParam[3];
        xASSERT_FMT(result && !noMoreSpawning && !flags.activated, "NPC %s has recieved a second spawn event.  This is a designer bug.  The second spawn event will be ignored.", xSceneID2Name(xglobals->sceneCur, npc_data->id));
        if (!result) {
            return false;
        }
    }

    if (npc_asset->ent_asset.baseFlags & k_XBASE_IS_ENABLED) {
        xASSERT_FMT(0, "NPC %s has recieved a spawn event but was enabled in the editor.  This is a designer bug.  The spawn event will be ignored.", xSceneID2Name(xglobals->sceneCur, npc_data->id));
        return false;
    }

    storedSpawn.valid = TRUE;
    storedSpawn.from = from;
    storedSpawn.to = to;
    storedSpawn.toEvent = toEvent;
    memcpy(storedSpawn.toParam, toParam, sizeof(storedSpawn.toParam));
    storedSpawn.toParamWidget = toParamWidget;
    storedSpawn.toParamWidgetID = toParamWidgetID;
    SetSpawnState(true);
    return true;
}

static void sNPCInstanceSetMatchingEntityVisibilityCullFlag(zNPCBaseIN2* npc, U32 visibilityCullFlag)
{
    if (!(npc->flags & k_XENT_IS_VISIBILITY_CULLED) && visibilityCullFlag) {
        npc->VisibilityCullCleanup();
        xEntVisibilityCullOn(npc);
    }
    else if ((npc->flags & k_XENT_IS_VISIBILITY_CULLED) && !visibilityCullFlag) {
        xEntVisibilityCullOff(npc);
    }
}

bool zNPCInstance::ShouldHaveData()
{
    if (!get_data()) {
        return flags.living && !flags.visibilityCull;
    }

    switch (get_data()->get_type()) {
    default:
        sNPCInstanceSetMatchingEntityVisibilityCullFlag(get_data(), flags.visibilityCull);
        return flags.living && !flags.visibilityCull;

        zNPCCommon* npc = (zNPCCommon*)get_data();
        F32 distToArea2 = xEntGetPos(npc)->Distance2(moveRadiusCenter);
        if (distToArea2 > moveRadius2) {
            if (npc->flags & k_XENT_IS_VISIBILITY_CULLED) {
                xEntVisibilityCullOff(npc);
            }
            return flags.living;
        }
        sNPCInstanceSetMatchingEntityVisibilityCullFlag(get_data(), flags.visibilityCull);
        return flags.living && !flags.visibilityCull;
    }
}

} // namespace zNPC
