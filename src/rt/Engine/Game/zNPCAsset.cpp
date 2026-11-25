#include "xCommon.h"
#include "zNPCAsset.h"

#include "xordarray.h"
#include "xPad.h"
#include "xstransvc.h"
#include "xString.h"
#include "xutil.h"
#include "zNPCAssetChef.h"
#include "zNPCAssetIN2.h"
#include "zNPCAssetThiefOld.h"
#include "zNPCBaseIN2.h"
#include "zNPCMgr.h"
#include "zNPCTypes.h"
#include "zScene.h"

#include <stdio.h>

struct DynNPCInfo
{
    en_npctyp npcType;
    const char* nam_npcAssetType;
    S32 (*fun_dynAssetValidate)(const xDynAsset*, U32);
    xBase* (*fun_npcCreate)(const xDynAsset*, U32);
    U32 hash_npcAssetType;
};

#define _NPC_VALIDATOR(line, name, namestr, T)                                                    \
static S32 Valid##name(const xDynAsset* dynAsset, U32 dynAssetSize)                               \
{                                                                                                 \
    char fullName[256];                                                                           \
    sprintf(fullName, "Enemy:IN2:%s", namestr);                                                   \
    xDynAssetValidate_L(line, *dynAsset, dynAssetSize, T, fullName);                              \
    return dynAsset->version == T::VERSION;                                                       \
}

#define NPC_VALIDATOR(name, T) _NPC_VALIDATOR(__LINE__, name, #name, T)
#define NPC_VALIDATOR_L(line, name, T) _NPC_VALIDATOR(line, name, #name, T)

NPC_VALIDATOR_L(87, Thief, zNPCAssetThiefOld)
NPC_VALIDATOR_L(88, Chef, zNPCAssetChef)

static S32 ValidTest(const xDynAsset* dynAsset, U32); // UNUSED
static S32 ValidTurret(const xDynAsset*, U32); // UNUSED
static S32 ValidViolet(const xDynAsset*, U32); // UNUSED

static xBase* MakeNPCGeneric(const xDynAsset* dynAsset, U32 dynAssetSize);

static DynNPCInfo g_DynNPCInfo[] = {
    { NPC_TYPE_THIEF, "Enemy:IN2:Thief", ValidThief, MakeNPCGeneric, 0 },
    { NPC_TYPE_CHEF, "Enemy:IN2:Chef", ValidChef, MakeNPCGeneric, 0 },
    { NPC_TYPE_UNKNOWN, NULL, NULL, NULL, 0 },
};

static xBase* MakeNPCGeneric(const xDynAsset* dynAsset, U32 dynAssetSize)
{
    size_t size_expected = 0;
    zNPCAssetIN2* npcass = (zNPCAssetIN2*)dynAsset;
    xEntAsset* entass = &npcass->ent_asset;
    xBaseAsset* baseass = &npcass->ent_asset;

    entass->id = dynAsset->id;
    entass->linkCount = dynAsset->linkCount;
    entass->baseType = eBaseTypeNPC;

    xBase* npc = zNPCMgr_createDynNPCInst(entass, dynAsset, dynAssetSize);
    xASSERT_L(143, npc);

    return npc;
}

void zNPCAsset_Startup()
{
    static S32 needToHash = TRUE;
    if (!needToHash) return;
    needToHash = FALSE;

    DynNPCInfo* npcinf_next = g_DynNPCInfo;
    while (npcinf_next->nam_npcAssetType) {
        DynNPCInfo* npcinf = npcinf_next++;
        npcinf->hash_npcAssetType = xStrHash(npcinf->nam_npcAssetType);
    }
}

S32 zNPCAsset_CountNPCs(S32* typeCount)
{
    zNPCAsset_Startup();

    S32 count_NPC = 0, num_alldyn = xSTAssetCountByType('DYNA');
    for (S32 i = 0; i < num_alldyn; i++) {
        U32 dynAssetSize;
        const xDynAsset* dynAsset = (const xDynAsset*)xSTFindAssetByType('DYNA', i, &dynAssetSize);

        xASSERT_FMT_L(195, dynAsset->type != xStrHash("Enemy:IN2:Test"),
                      "Type NPC Test found.  This type is no longer supported in the engine.  Please use NPC Humanoid instead.");

        S32 isOurs = FALSE;
        DynNPCInfo* npcinf_next = g_DynNPCInfo;
        while (npcinf_next->nam_npcAssetType) {
            DynNPCInfo* npcinf = npcinf_next++;
            if (npcinf->hash_npcAssetType != dynAsset->type)
                continue;

            if (typeCount) {
                typeCount[npcinf->npcType]++;
            }

            isOurs = TRUE;
            break;
        }

        if (!isOurs) {
            continue;
        }

        count_NPC++;
    }

    S32 sum_total = count_NPC;

    return sum_total;
}

void zNPCAsset_CreateNPCs()
{
    S32 num_alldyn = xSTAssetCountByType('DYNA');
    for (S32 i = 0; i < num_alldyn; i++) {
        U32 dynAssetSize;
        const xDynAsset* dynAsset = (const xDynAsset*)xSTFindAssetByType('DYNA', i, &dynAssetSize);
        xASSERT_L(238, dynAsset);
        if (!dynAsset) {
            continue;
        }

        DynNPCInfo* npcinf_next = g_DynNPCInfo;
        while (npcinf_next->nam_npcAssetType) {
            DynNPCInfo* npcinf = npcinf_next++;
            if (npcinf->hash_npcAssetType != dynAsset->type) {
                continue;
            }

#if DEBUG || RELEASE
            S32 rc;
            if (npcinf->fun_dynAssetValidate) {
                rc = npcinf->fun_dynAssetValidate(dynAsset, dynAssetSize);
            }
#endif

            xASSERT_L(264, npcinf->fun_npcCreate);
            xBase* npc = npcinf->fun_npcCreate(dynAsset, dynAssetSize);
            break;
        }
    }
}

void zNPCAsset_RegisterNPCs(zScene*, S32* base_idx)
{
    st_XORDEREDARRAY* npclist = zNPCMgr_GetNPCList();

    S32 num_npc = npclist->cnt;
    for (S32 i = 0; i < num_npc; i++) {
        zNPC::zNPCBaseIN2* npc = (zNPC::zNPCBaseIN2*)npclist->list[i];
        xASSERT_L(284, npc);

        zSceneSet(npc, *base_idx);

        zSceneObjHashtableAdd(npc->id, npc);

        (*base_idx)++;
    }
}

bool zNPCAsset_IsNPC(const xDynAsset& asset)
{
    if (asset.baseType == eBaseTypeNPC) {
        return true;
    }

    if (asset.baseType == eBaseTypeUnknown) {
        for (S32 i = 0; i < ARRAY_LENGTH(g_DynNPCInfo) - 1; i++) {
            if (asset.type == g_DynNPCInfo[i].hash_npcAssetType) {
                return true;
            }
        }
    }

    return false;
}
