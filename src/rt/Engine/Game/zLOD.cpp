#include "xCommon.h"
#include "zLOD.h"

#include "containers.h"
#include "xCam.h"
#include "xEnt.h"
#include "xMath.h"
#include "xModel.h"
#include "xModelBucket.h"
#include "xScene.h"
#include "xstransvc.h"
#include "zDestructible.h"
#include "zGlobals.h"
#include "zScene.h"

#include <string.h>

struct zLODManager
{
    S32 numextra;
    zLODTable* lod;
    xModelInstance* model;
    F32 adjustNoRenderDist;
};

#define MAX_MANAGED_LODS 2900

static U32 sLODTableCount;
static zLODTable* sLODTableList;
static U32 sManagerIndex;
static U32 sManagerCount;
static F32 sLODFadeDistance = 4.0f;
static zLODManager sManagerList[MAX_MANAGED_LODS];

static void AddToLODList(xEnt* ent)
{
    for (U32 i = 0; i < sLODTableCount; i++) {
        if (sLODTableList[i].baseBucket && (*sLODTableList[i].baseBucket)->OriginalData == ent->model->Data) {
            xASSERT_ONCE_L(45, sManagerCount < MAX_MANAGED_LODS);
            if (sManagerCount >= MAX_MANAGED_LODS) {
                return;
            }

            xModelInstance* minst = ent->model;
            F32 distscale;
            if (sLODTableList[i].flags & k_ZLODTABLE_IGNORE_SCALE)
                distscale = 1.0f;
            else {
                distscale = xsqr(minst->Mat->right.x) + xsqr(minst->Mat->right.y) + xsqr(minst->Mat->right.z);
                if (distscale < 0.0001f) {
                    distscale = 1.0f;
                }
            }

            while (minst) {
                minst->FadeEnd = xsqrt(distscale * sLODTableList[i].noRenderDist);
                minst->FadeStart = minst->FadeEnd - sLODFadeDistance;
                minst = minst->Next;
            }

            sManagerList[sManagerCount].numextra = 0;
            for (xModelInstance* extra = ent->model->Next; extra; extra = extra->Next) {
                sManagerList[sManagerCount].numextra++;
            }
            sManagerList[sManagerCount].lod = &sLODTableList[i];
            sManagerList[sManagerCount].model = ent->model;
            sManagerList[sManagerCount].adjustNoRenderDist = xsqr(10.0f + xsqrt(sLODTableList[i].noRenderDist));

            zDestructible* destructible = zDestructibleGetFromEntity(ent);
            if (destructible) {
                zDestructibleAsset_CopyLODDistances(ent, destructible);
            }

            sManagerCount++;
        }
    }
}

static xEnt* AddToLODList(xEnt* ent, xScene*, void*)
{
    if (!ent->model) {
        return ent;
    }

    AddToLODList(ent);

    return ent;
}

void zLOD_Setup(F32 fadeDistance)
{
    U32 tmpsize, i, j;
    void* data = NULL;

    sLODFadeDistance = fadeDistance;
    sLODTableCount = 0;
    sLODTableList = NULL;
    sManagerCount = 0;

    U32 count = xSTAssetCountByType('LODT');
    if (!count) {
        return;
    }

    for (i = 0; i < count; i++) {
        data = xSTFindAssetByType('LODT', i, &tmpsize);
        sLODTableCount += *(U32*)data;
    }

    if (!sLODTableCount) {
        return;
    }

    sLODTableList = (zLODTable*)xMEMALLOC_L(sLODTableCount * sizeof(zLODTable), 0, eMemMgrTag_Model, 0xEEEE, 138);

    zLODTable* tableCurr = sLODTableList;
    for (i = 0; i < count; i++) {
        data = xSTFindAssetByType('LODT', i, &tmpsize);
        memcpy(tableCurr, (U32*)data + 1, *(U32*)data * sizeof(zLODTable));
        tableCurr += *(U32*)data;
    }

    for (i = 0; i < sLODTableCount; i++) {
        sLODTableList[i].noRenderDist *= sLODTableList[i].noRenderDist;
        if (sLODTableList[i].baseBucket) {
            RpAtomic* model = (RpAtomic*)xSTFindAsset((U32)sLODTableList[i].baseBucket, NULL);
            if (model) {
                sLODTableList[i].baseBucket = xModelBucket_GetBuckets(model);
            } else {
                xFAIL_FMT_L(155, "Zero sized model perhaps - Asset 0x%x\n", sLODTableList[i].baseBucket);
                sLODTableList[i].baseBucket = NULL;
            }
        }

        for (j = 0; j < 3; j++) {
            if (sLODTableList[i].lodBucket[j]) {
                RpAtomic* model = (RpAtomic*)xSTFindAsset((U32)sLODTableList[i].lodBucket[j], NULL);
                if (model) {
                    sLODTableList[i].lodBucket[j] = xModelBucket_GetBuckets(model);
                } else {
                    sLODTableList[i].lodBucket[j] = NULL;
                }
            }

            sLODTableList[i].lodDist[j] *= sLODTableList[i].lodDist[j];
        }
    }

    sManagerCount = 0;
    xSceneForAllEnts(globals.sceneCur, AddToLODList, NULL);
}

void zLOD_Update(U32 percent_update)
{
    xVec3* campos = &globals.cam->mat.pos;

    if (!sManagerCount) {
        return;
    }

    U32 numUpdates = sManagerCount * percent_update / 100;
    if (!numUpdates) {
        numUpdates++;
    }

    while (numUpdates--) {
        sManagerIndex++;
        if (sManagerIndex >= sManagerCount) {
            sManagerIndex = 0;
        }

        zLODManager& lodManager = sManagerList[sManagerIndex];

        zLODTable* lod = lodManager.lod;
        xModelInstance* model = lodManager.model;

        if (!lod) {
            continue;
        }

        xASSERT_L(213, model->Mat);

        F32 camdist2 = 0.0f, distscale;
        if (lod->flags & k_ZLODTABLE_IGNORE_SCALE)
            distscale = 1.0f;
        else {
            distscale = xsqr(model->Mat->right.x) + xsqr(model->Mat->right.y) + xsqr(model->Mat->right.z);
            if (distscale < 0.0001f) {
                distscale = 1.0f;
            }
        }

        if (model->Mat) {
            camdist2 = (xsqr(campos->x - model->Mat->pos.x) + xsqr(campos->y - model->Mat->pos.y) +
                        xsqr(campos->z - model->Mat->pos.z)) / distscale;
        }

        if (camdist2 >= lodManager.adjustNoRenderDist) {
            model->Flags |= ModelInstance_NoRender;

            if (lodManager.numextra) {
                model = model->Next;
                while (model) {
                    model->Flags |= ModelInstance_NoRender;
                    model = model->Next;
                }
            }
        } else {
            S32 i = 0;

            model->Flags &= (U16)~ModelInstance_NoRender;

            if (lod->baseBucket && lod->lodBucket[0]) {
                model->Bucket = lod->baseBucket;
                model->Data = model->Bucket[0]->OriginalData;
            }

            while (i < 3 && lod->lodBucket[i] && camdist2 > lod->lodDist[i]) {
                model->Bucket = lod->lodBucket[i];
                model->Data = model->Bucket[0]->OriginalData;
                i++;
            }

            if (lodManager.numextra) {
                if (i == 0) {
                    model = model->Next;
                    while (model) {
                        model->Flags &= (U16)~ModelInstance_NoRender;
                        model = model->Next;
                    }
                } else {
                    model = model->Next;
                    while (model) {
                        model->Flags |= ModelInstance_NoRender;
                        model = model->Next;
                    }
                }
            }
        }
    }
}

zLODTable* zLOD_Get(xEnt* ent)
{
    if (!ent->model) {
        return NULL;
    }

    for (U32 i = 0; i < sLODTableCount; i++) {
        if (sLODTableList[i].baseBucket && (*sLODTableList[i].baseBucket)->OriginalData == ent->model->Data) {
            return &sLODTableList[i];
        }
    }

    return NULL;
}
