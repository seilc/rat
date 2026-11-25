#include "xCommon.h"
#include "zNPCCoverPoint.h"

#include "xEntMotion.h"
#include "xEvent.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xSpacePartitionTree.h"
#include "zBase.h"
#include "zBehavior.h"
#include "zDestructible.h"
#include "zNPC.h"
#include "zNPCCommon.h"
#include "zPlayer.h"
#include "zPlayerContainer.h"
#include "zScene.h"

static void sNPCCoverPointFindNPC(zNPCCoverPoint* coverPoint, xGroup* coverPointGroup, xGroup* npcGroup)
{
    S32 npcIter, cpCheckIter, bestNPC;
    F32 npcDist2, bestDist2 = -1.0f;
    zNPC::zNPCInstance* inst;
    zNPC::zNPCCommon* npc, *npcCheck;
    zNPCCoverPoint* coverPointCheck;
    bool bestFlag, replacedFlag;

    for (npcIter = 0; npcIter < npcGroup->asset->itemCount; npcIter++) {
        inst = (zNPC::zNPCInstance*)npcGroup->item[npcIter];
        xASSERT_FMT_L(47, inst->baseType == eBaseTypeNPC, "A retreat to cover event has been to sent to a group that contains non-npcs");

        if (!inst->ShouldHaveData() || !inst->get_flags().activated) {
            continue;
        }

        npc = (zNPC::zNPCCommon*)inst->get_data();
        xASSERT_FMT_L(54, npc, "An NPC that was sent a retreat to cover event should be alive, but isn't.  this is a programming bug");
        if (!npc) {
            continue;
        }

        xASSERT_FMT_L(60, npc->baseType == eBaseTypeNPC, "A retreat to cover event has been to sent to a group that contains non-npcs");
        if (!npc->manager.getCurrentBehavior()->CoverTransitionAllowable()) {
            continue;
        }

        npcDist2 = xVec3Dist2(&npc->frame->mat.pos, coverPoint->GetCoverLocation());
        if (bestDist2 != -1.0f && npcDist2 > bestDist2) {
            continue;
        }

        bestFlag = true;

        for (cpCheckIter = 0; cpCheckIter < coverPointGroup->asset->itemCount; cpCheckIter++) {
            coverPointCheck = (zNPCCoverPoint*)coverPointGroup->item[cpCheckIter];
            npcCheck = coverPointCheck->IsSeized();

            if (npc == npcCheck && coverPointCheck == coverPoint) {
                continue;
            }

            if (npcCheck && npc == npcCheck &&
                npcDist2 > xVec3Dist2(&npcCheck->frame->mat.pos, coverPointCheck->GetCoverLocation())) {
                bestFlag = false;
            }
        }

        if (bestFlag) {
            bestNPC = npcIter;
            bestDist2 = npcDist2;
        }
    }

    if (bestDist2 != -1.0f) {
        inst = (zNPC::zNPCInstance*)npcGroup->item[bestNPC];
        npc = (zNPC::zNPCCommon*)inst->get_data();

        replacedFlag = false;
        for (cpCheckIter = 0; cpCheckIter < coverPointGroup->asset->itemCount; cpCheckIter++) {
            coverPointCheck = (zNPCCoverPoint*)coverPointGroup->item[cpCheckIter];
            npcCheck = coverPointCheck->IsSeized();

            if (npc == npcCheck && coverPoint == coverPointCheck) {
                continue;
            }

            if (npcCheck && npc == npcCheck) {
                coverPointCheck->Release(npcCheck);
                coverPoint->Seize(npc);

                sNPCCoverPointFindNPC(coverPointCheck, coverPointGroup, npcGroup);
                replacedFlag = true;
                break;
            }
        }

        if (!replacedFlag) {
            coverPoint->Seize(npc);
        }
    }
}

void zNPCCoverManageRetreatToCover(xBase* from, xGroup* npcGroup, U32 toEvent, const F32* toParam, xGroup* coverPointGroup, U32 toParamWidgetID)
{
    xASSERT_L(146, npcGroup);
    xASSERT_L(147, npcGroup->baseType == eBaseTypeGroup);
    xASSERT_FMT_L(148, coverPointGroup, "No widget was passed with a Retreat to Cover event.");
    xASSERT_FMT_L(149, coverPointGroup->baseType == eBaseTypeGroup, "A widget was passed with a Retreat to Cover event that was not a group.");

    S32 cpIter;
    zNPCCoverPoint* coverPoint;

    for (cpIter = 0; cpIter < coverPointGroup->asset->itemCount; cpIter++) {
        coverPoint = (zNPCCoverPoint*)coverPointGroup->item[cpIter];

        xASSERT_FMT_L(159, coverPoint->baseType == eBaseTypeNPCCoverPoint, "An enemy has been sent a Retreat to Cover event, but the cover point group contains non-cover points");

        if ((CURPLAYERS[0] && coverPoint->GetCoverPlane()->Dist(CURPLAYERS[0]->frame->mat.pos) < 0.0f) ||
            (CURPLAYERS[1] && coverPoint->GetCoverPlane()->Dist(CURPLAYERS[1]->frame->mat.pos) < 0.0f) ||
            (coverPoint->GetCoverObject() && !zDestructibleGetFromEntity(coverPoint->GetCoverObject())->active) ||
            !xBaseIsEnabled(coverPoint)) {
            continue;
        }

        sNPCCoverPointFindNPC(coverPoint, coverPointGroup, npcGroup);
    }

    for (cpIter = 0; cpIter < coverPointGroup->asset->itemCount; cpIter++) {
        coverPoint = (zNPCCoverPoint*)coverPointGroup->item[cpIter];
        if (coverPoint->IsSeized() && coverPoint->IsSeized()->manager.getCurrentBehavior()->CoverTransitionAllowable()) {
            zEntEvent(from, coverPoint->IsSeized(), toEvent, toParam, coverPointGroup->item[cpIter], toParamWidgetID);
        }
    }
}

void zNPCCoverPoint::Init(xBase& data, xDynAsset& dynasset, size_t asset_size)
{
    xDynAssetValidate_L(186, dynasset, asset_size, zNPCCoverPointAsset, "npc:CoverPoint");

    xBaseInit(&data, &dynasset);

    zNPCCoverPoint* newCP = (zNPCCoverPoint*)&data;

    newCP->coverPointAsset = (zNPCCoverPointAsset*)&dynasset;
    newCP->eventFunc = NULL;

    if (newCP->coverPointAsset->assetID) {
        newCP->coverObject = (xEnt*)zSceneFindObject(newCP->coverPointAsset->assetID);
    } else {
        newCP->coverObject = NULL;
    }

    newCP->seizedBy = NULL;

    xMat3x3 eulerMat;
    xMat3x3Euler(
        &eulerMat, (PI / 180.0f) * newCP->coverPointAsset->directionYaw,
        (PI / 180.0f) * newCP->coverPointAsset->directionPitch, (PI / 180.0f) * newCP->coverPointAsset->directionRoll);

    newCP->playerCrossPlane.Init(eulerMat.at, newCP->coverPointAsset->location);
}

bool zNPCCoverPoint::Seize(zNPC::zNPCCommon* seizer)
{
    if (seizedBy) {
        return false;
    }

    seizedBy = seizer;
    return true;
}

bool zNPCCoverPoint::Release(zNPC::zNPCCommon* releaser)
{
    if (seizedBy != releaser) {
        return false;
    }

    seizedBy = NULL;
    return true;
}
