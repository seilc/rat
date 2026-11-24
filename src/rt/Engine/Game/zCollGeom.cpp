#include "xCommon.h"
#include "zCollGeom.h"

#include "containers.h"
#include "iModel.h"
#include "xEnt.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xModel.h"
#include "xOutputMgr.h"
#include "xPad.h"
#include "xstransvc.h"
#include "xString.h"
#include "zBase.h"

struct zCollGeomTable
{
    RpAtomic* baseModel;
    RpAtomic* colModel[1];
    RpAtomic* camcolModel;
};

struct zCollisionGeomTableEntry
{
    U32 sTableCount;
    zCollGeomTable* sTableList;
};

#define MAX_COLLISION_TABLES 20

static zCollisionGeomTableEntry sCollisionGeomTable[MAX_COLLISION_TABLES];
static U32 sCollisionGeomTableCount;

U32 zCollGeom_EntSetup(xEnt* ent)
{
    xASSERT_L(39, ent);

    U32 i, j;
    bool auto_cam_coll = (ent->baseType == eBaseTypeStatic);

    ent->collModel = NULL;
    ent->camcollModel = NULL;

    if (ent->model) {
        for (j = 0; j < sCollisionGeomTableCount; j++) {
            zCollGeomTable* tableList = sCollisionGeomTable[j].sTableList;

            for (i = 0; i < sCollisionGeomTable[j].sTableCount; i++) {
                if (tableList[i].baseModel != ent->model->Data) {
                    continue;
                }

                RpAtomic* imodel = tableList[i].colModel[0];
                if (imodel) {
                    xModelInstance* model = xModelInstanceAlloc(imodel, ent, 0x2000, 0, NULL);
                    ent->collModel = model;
                    ent->collModel->Mat = ent->model->Mat;

                    while (imodel = iModelFile_RWMultiAtomic(imodel)) {
                        xModelInstanceAttach(xModelInstanceAlloc(imodel, ent, 0x2000, 0, NULL), model);
                    }
                } else {
                    imodel = tableList[i].camcolModel;
                    if (imodel) {
                        xOutInfo("COLLIS", xFUNCTION_FMT("adding camcoll model to ent (ent->id 0x%08X).\n"), ent->id);

                        xModelInstance* model = xModelInstanceAlloc(imodel, ent, 0x2000, 0, NULL);
                        ent->camcollModel = model;
                        ent->camcollModel->Mat = ent->model->Mat;

                        while (imodel = iModelFile_RWMultiAtomic(imodel)) {
                            xModelInstanceAttach(xModelInstanceAlloc(imodel, ent, 0x2000, 0, NULL), model);
                        }
                    } else {
                        auto_cam_coll = false;
                    }
                }
            }
        }
    }

    if (auto_cam_coll && !ent->camcollModel) {
        ent->camcollModel = ent->collModel ? ent->collModel : ent->model;
    }

    if (ent->collModel) {
        return 1;
    }

    ent->collModel = ent->model;
    return 0;
}

void zCollGeom_Init()
{
    U32 tmpsize, i, j, k;

    if (!(sCollisionGeomTableCount = xSTAssetCountByType('COLL'))) {
        return;
    }

    if (sCollisionGeomTableCount > MAX_COLLISION_TABLES) {
        xFAIL_FMT_L(142, "The number of collision tables is more than %d.\nIt's safe to skip this assert but not\n"
                         "all collision models will work.", MAX_COLLISION_TABLES);
        sCollisionGeomTableCount = MAX_COLLISION_TABLES;
    }

    for (k = 0; k < sCollisionGeomTableCount; k++) {
        void* data = xSTFindAssetByType('COLL', k, &tmpsize);
        if (!data) {
            continue;
        }

        sCollisionGeomTable[k].sTableCount = *(U32*)data;
        if (sCollisionGeomTable[k].sTableCount == 0) {
            sCollisionGeomTable[k].sTableList = NULL;
            continue;
        }

        sCollisionGeomTable[k].sTableList = (zCollGeomTable*)((U32*)data + 1);

        for (i = 0; i < sCollisionGeomTable[k].sTableCount; i++) {
            if (sCollisionGeomTable[k].sTableList[i].baseModel) {
                U32 hash = (U32)sCollisionGeomTable[k].sTableList[i].baseModel;
                sCollisionGeomTable[k].sTableList[i].baseModel = (RpAtomic*)xSTFindAsset(hash, NULL);

                if (!sCollisionGeomTable[k].sTableList[i].baseModel) {
                    hash = xStrHashCat(hash, ".dff");
                    sCollisionGeomTable[k].sTableList[i].baseModel = (RpAtomic*)xSTFindAsset(hash, NULL);
                }
            }

            for (j = 0; j < 1; j++) {
                if (sCollisionGeomTable[k].sTableList[i].colModel[j]) {
                    sCollisionGeomTable[k].sTableList[i].colModel[j] =
                        (RpAtomic*)xSTFindAsset((U32)sCollisionGeomTable[k].sTableList[i].colModel[j], NULL);
                }
            }

            if (!sCollisionGeomTable[k].sTableList[i].colModel[0]) {
                if (sCollisionGeomTable[k].sTableList[i].camcolModel) {
                    U32 aid = (U32)sCollisionGeomTable[k].sTableList[i].camcolModel;
                    sCollisionGeomTable[k].sTableList[i].camcolModel = (RpAtomic*)xSTFindAsset(aid, NULL);
                    xASSERT_L(193, sCollisionGeomTable[ k ].sTableList[i].camcolModel);
                    xOutInfo("COLLIS", xFUNCTION_FMT("found cam col model (for aid 0x%08X)!\n"), aid);
                } else {
                    xOutInfo("COLLIS", xFUNCTION_FMT("who dunn what with my model?\n"));
                }
            }
        }
    }
}

void zCollGeom_CamEnable(xEnt* ent)
{
    if (!ent->camcollModel) {
        ent->camcollModel = ent->collModel;
    }
}

void zCollGeom_CamDisable(xEnt* ent)
{
    ent->camcollModel = NULL;
}
