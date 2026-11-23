#include "xCommon.h"
#include "zJSPExtraData.h"

#include "zEvent.h"
#include "zScene.h"

zJSPExtraData::jspGroupPair zJSPExtraData::sJspGroupPairs[MAX_PAIRS];
S32 zJSPExtraData::sNumPairs;

void zJSPExtraData::ShowAllAssociatedGroups(bool show)
{
    for (S32 i = 0; i < sNumPairs; i++) {
        sJspGroupPairs[i].pGroup = zSceneFindObject(sJspGroupPairs[i].groupID);
        xASSERT_FMT_L(26, sJspGroupPairs[i].pGroup, "JSP Extra Data is missing a group");
        if (sJspGroupPairs[i].pGroup) {
            if (show) {
                zEntEvent(NULL, sJspGroupPairs[i].pGroup, eEventVisibilityCullOff);
            } else {
                zEntEvent(NULL, sJspGroupPairs[i].pGroup, eEventVisibilityCullOn);
            }
        }
    }
}

void zJSPExtraData::ShowAssociatedGroup(U32 jspAssetId, bool show)
{
    for (S32 i = 0; i < sNumPairs; i++) {
        if (jspAssetId == sJspGroupPairs[i].jspID) {
            if (show) {
                if (!sJspGroupPairs[i].pGroup && sJspGroupPairs[i].groupID) {
                    sJspGroupPairs[i].pGroup = zSceneFindObject(sJspGroupPairs[i].groupID);
                }
                if (sJspGroupPairs[i].pGroup)
                    zEntEvent(sJspGroupPairs[i].pGroup, eEventVisibilityCullOff);
                zEntEvent(sJspGroupPairs[i].pWidget, eEventOn);
            } else {
                if (!sJspGroupPairs[i].pGroup && sJspGroupPairs[i].groupID) {
                    sJspGroupPairs[i].pGroup = zSceneFindObject(sJspGroupPairs[i].groupID);
                }
                if (sJspGroupPairs[i].pGroup)
                    zEntEvent(sJspGroupPairs[i].pGroup, eEventVisibilityCullOn);
                zEntEvent(sJspGroupPairs[i].pWidget, eEventOff);
            }
        }
    }
}

void zJSPExtraData::ClearGroupAssociations()
{
    sNumPairs = 0;
}

void zJSPExtraData::AddPair(U32 jspID, U32 groupID, zJSPExtraData* widget)
{
    xASSERT_L(78, sNumPairs < MAX_PAIRS);
    sJspGroupPairs[sNumPairs].jspID = jspID;
    sJspGroupPairs[sNumPairs].groupID = groupID;
    sJspGroupPairs[sNumPairs].pGroup = NULL;
    sJspGroupPairs[sNumPairs].pWidget = widget;
    sNumPairs++;
}

void zJSPExtraData::Load(xBase& data, xDynAsset& asset, size_t)
{
    ((zJSPExtraData&)data).LoadPrivate((zJSPExtraDataAsset&)asset);
}

void zJSPExtraData::LoadPrivate(const zJSPExtraDataAsset& myAsset)
{
    xBaseInit(this, &myAsset);

    if (linkCount)
        link = (xLinkAsset*)(&myAsset + 1);
    else
        link = NULL;

    AddPair(myAsset.jspID, myAsset.groupID, this);
}
