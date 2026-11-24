#include "xCommon.h"
#include "zGrapple.h"

#include "xEnt.h"
#include "xGroup.h"
#include "zBase.h"
#include "zEvent.h"
#include "zScene.h"

void zGrapple_Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(13, asset, asset_size, zGrappleAsset, "game_object:Grapple");
    zGrapple_Init((zGrapple*)&data, (zGrappleAsset*)&asset);
}

void zGrapple_Init(zGrapple* grapple, zGrappleAsset* asset)
{
    xBaseInit(grapple, asset);

    grapple->grappleAsset = asset;
    grapple->eventFunc = zGrapple_EventCB;

    if (grapple->linkCount)
        grapple->link = (xLinkAsset*)(asset + 1);
    else
        grapple->link = NULL;
}

static zGrapplePoint* sGrapplePoints;
static S32 sNumGrapplePoints;

S32 zGrapple_Recurse(U32 id, zGrapplePoint* gpList, zGrapple* grapple)
{
    U32 i;
    S32 result = 0;
    xBase* base = zSceneFindObject(id);
    if (base) {
        if (base->baseType == eBaseTypeGroup) {
            xGroup* group = (xGroup*)base;
            U32 count = xGroupGetCount(group);
            for (i = 0; i < count; i++) {
                U32 memberID = xGroupGetItem(group, i);
                S32 incr;
                if (gpList) {
                    incr = zGrapple_Recurse(memberID, &gpList[result], grapple);
                } else {
                    incr = zGrapple_Recurse(memberID, NULL, NULL);
                }
                result += incr;
            }
        }
        else if (base->baseFlags & k_XBASE_IS_ENTITY) {
            if (gpList) {
                gpList[result].pos = (xVec3*)&((xEnt*)base)->model->Mat->pos;
                gpList[result].local = (xMat3x3*)((xEnt*)base)->model->Mat;
                gpList[result].offset = &grapple->grappleAsset->offset;
                gpList[result].optr = base;
                gpList[result].grap = grapple;
            }
            result++;
        }
    } else {
        U32 size;
        base = (xBase*)xSTFindAsset(id, &size);
        if (size == sizeof(xVec3)) {
            if (gpList) {
                gpList[result].pos = (xVec3*)base;
                gpList[result].local = NULL;
                gpList[result].offset = NULL;
                gpList[result].optr = base;
                gpList[result].grap = grapple;
            }
            result++;
        }
    }

    return result;
}

void zGrapple_SceneInit(zScene* zsc)
{
    S32 i, count, numGrapples = 0;
    zGrapple* grapple;
    zGrappleAsset* asset;

    count = zsc->baseCount[eBaseTypeGrapple];
    for (i = 0; i < count; i++) {
        grapple = &((zGrapple*)zsc->baseList[eBaseTypeGrapple])[i];
        numGrapples += zGrapple_Recurse(grapple->grappleAsset->object, NULL, NULL);
    }

    if (numGrapples > 0)
        sGrapplePoints = (zGrapplePoint*)xMEMALLOC_L(numGrapples * sizeof(zGrapplePoint), 0, eMemMgrTag_EnvAbstract, 0, 108);
    else
        sGrapplePoints = NULL;
    sNumGrapplePoints = numGrapples;

    numGrapples = 0;
    for (i = 0; i < count; i++) {
        grapple = &((zGrapple*)zsc->baseList[eBaseTypeGrapple])[i];
        numGrapples += zGrapple_Recurse(grapple->grappleAsset->object, &sGrapplePoints[numGrapples], grapple);
    }

    xASSERT_L(120, numGrapples == sNumGrapplePoints);

    for (i = 0; i < sNumGrapplePoints; i++) {
        grapple = sGrapplePoints[i].grap;
        asset = grapple->grappleAsset;
        grapple->flags = asset->grapFlags;
    }
}

void zGrapple_Reset()
{
    for (S32 i = 0; i < sNumGrapplePoints; i++) {
        zGrapple* grapple = sGrapplePoints[i].grap;
        zGrappleAsset* asset = grapple->grappleAsset;
        grapple->flags = asset->grapFlags;
    }
}

void zGrapple_EventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    xASSERT_L(142, to);

    zGrapple* grap = (zGrapple*)to;
    xASSERT_L(145, grap->baseType == eBaseTypeGrapple);

    switch (toEvent) {
    case eEventOn:
        grap->flags |= 1;
        break;
    case eEventOff:
        grap->flags &= ~1;
        break;
    }
}

// UNUSED: needed to match (code from Incredibles GC)
zGrapplePoint* zGrapple_GetGrapplePoint(S32 i)
{
    xASSERT(i < sNumGrapplePoints);
    return &sGrapplePoints[i];
}
