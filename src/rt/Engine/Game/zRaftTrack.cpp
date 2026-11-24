#include "xCommon.h"
#include "zRaftTrack.h"

#include "containers.h"
#include "xEnt.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xGroup.h"
#include "xPad.h"
#include "zAutoTweak.h"
#include "zBase.h"
#include "zScene.h"

#include <stdio.h>
#include <string.h>

void zRaftTrack::load(const asset_type& a)
{
    memset(this, 0, sizeof(zRaftTrack));
    xBaseInit(this, &a);
    baseType = eBaseTypeRaftTrack;
    asset = &a;
    trackCount = 0;
    fastTrackCount = 0;

    if (!id) return;

    xBase* obj = zSceneFindObject(asset->track);
    if (!obj) return;

    if (obj->baseType == eBaseTypeGroup) {
        xGroup* group = (xGroup*)obj;
        for (U32 i = 0, end = xGroupGetCount(group); i < end; i++) {
            xBase* entry = xGroupGetItemPtr(group, i);
            if (!entry || (entry->baseType != eBaseTypeStatic && entry->baseType != eBaseTypePlatform)) {
                continue;
            }

            xASSERT_L(76, trackCount < MAX_RAFTTRACKS);
            xEnt* raftTrackEnt = (xEnt*)entry;
            trackEnt[trackCount++] = raftTrackEnt;
            raftTrackEnt->baseFlags &= (U16)~(k_XBASE_IS_VISIBLE_IN_CUTSCENES | k_XBASE_RECEIVES_SHADOWS);
            raftTrackEnt->chkby = k_XENT_COLLTYPE_NONE;
            raftTrackEnt->penby = k_XENT_COLLTYPE_NONE;
        }
    }
    else if (obj->baseType == eBaseTypeStatic || obj->baseType == eBaseTypePlatform) {
        xEnt* raftTrackEnt = (xEnt*)obj;
        trackEnt[trackCount++] = raftTrackEnt;
        raftTrackEnt->baseFlags &= (U16)~(k_XBASE_IS_VISIBLE_IN_CUTSCENES | k_XBASE_RECEIVES_SHADOWS);
        raftTrackEnt->chkby = k_XENT_COLLTYPE_NONE;
        raftTrackEnt->penby = k_XENT_COLLTYPE_NONE;
    }

    obj = zSceneFindObject(asset->fastTrack);
    if (obj) {
        if (obj->baseType == eBaseTypeGroup) {
            xGroup* group = (xGroup*)obj;
            for (U32 i = 0, end = xGroupGetCount(group); i < end; i++) {
                xBase* entry = xGroupGetItemPtr(group, i);
                if (!entry || (entry->baseType != eBaseTypeStatic && entry->baseType != eBaseTypePlatform)) {
                    continue;
                }

                // BUG: checks trackCount, not fastTrackCount
                xASSERT_L(115, trackCount < MAX_RAFTTRACKS);
                xEnt* raftTrackEnt = (xEnt*)entry;
                fastTrackEnt[fastTrackCount++] = raftTrackEnt;
                raftTrackEnt->baseFlags &= (U16)~(k_XBASE_IS_VISIBLE_IN_CUTSCENES | k_XBASE_RECEIVES_SHADOWS);
                raftTrackEnt->chkby = k_XENT_COLLTYPE_NONE;
                raftTrackEnt->penby = k_XENT_COLLTYPE_NONE;
            }
        }
        else if (obj->baseType == eBaseTypeStatic || obj->baseType == eBaseTypePlatform) {
            xEnt* raftTrackEnt = (xEnt*)obj;
            fastTrackEnt[fastTrackCount++] = raftTrackEnt;
            raftTrackEnt->baseFlags &= (U16)~(k_XBASE_IS_VISIBLE_IN_CUTSCENES | k_XBASE_RECEIVES_SHADOWS);
            raftTrackEnt->chkby = k_XENT_COLLTYPE_NONE;
            raftTrackEnt->penby = k_XENT_COLLTYPE_NONE;
        }
    }

    debugInit();
}

void zRaftTrack::load(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(149, asset, asset_size, zRaftTrack::asset_type, zRaftTrack::asset_type::type_name());
    ((zRaftTrack&)data).load((const asset_type&)asset);
}

// UNUSED: needed to match (speculative code)
void zRaftTrack::sceneExit()
{
    xTWEAK_REMOVE("Widgets|Raft Track");
}

#if DEBUG || RELEASE
void zRaftTrack::debugInit()
{
    addTweaks();
}

void zRaftTrack::addTweaks()
{
    char prefix[128] = "Widgets|Raft Track|";
    sprintf(prefix, "Widgets|Raft Track|%s|", xSTAssetNameOrID(asset->id));

    asset_type& a = (asset_type&)*asset;

    zAUTOTWEAK(prefix, "Minimum Speed", &a.minSpeed, 0.0f, 1000.0f, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "Normal Speed", &a.normSpeed, 0.0f, 1000.0f, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "Maximum Speed", &a.maxSpeed, 0.0f, 1000.0f, NULL, NULL, 2);
}
#endif
