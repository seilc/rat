#include "xCommon.h"
#include "zNPCManager.h"

#include "xAnim.h"
#include "xEnt.h"
#include "xEntMotion.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xstransvc.h"
#include "xString.h"
#include "zAssetTypes.h"
#include "zBase.h"
#include "zCommonPlayer.h"
#include "zCompStateMachine.h"
#include "zDebugTweakBranch.h"
#include "zICompLogic.h"
#include "zNPCAsset.h"
#include "zNPCBase.h"
#include "zNPCCommon.h"
#include "zPlayer.h"
#include "zScene.h"

#include <stdio.h>

bool gNPCManagerInitialized;
zNPCManager* gNPCManager;

void zNPCManagerStartup()
{
    if (gNPCManagerInitialized) {
        return;
    }

    gNPCManager = new ('NPCM', eMemMgrTag_Enemy, NULL) zNPCManager;
    xASSERT_L(67, gNPCManager);

    gNPCManager->Startup();
    gNPCManagerInitialized = true;
}

void zNPCManagerShutdown()
{
    if (!gNPCManagerInitialized) {
        return;
    }

    zNPCManager* mgr = zNPCManagerInstance();
    if (mgr) {
        mgr->Shutdown();
        delete mgr;
        gNPCManager = NULL;
    }

    gNPCManagerInitialized = false;
}

void zNPCManagerSceneInit()
{
    zNPCManager* npcManager = zNPCManagerInstance();
    npcManager->SceneInit();
}

void zNPCManagerScenePrepare(S32 npccnt)
{
    zNPCManager* npcManager = zNPCManagerInstance();
    npcManager->ScenePrepare(npccnt);
}

void zNPCManagerSceneFinish()
{
    zNPCManager* npcManager = zNPCManagerInstance();
    npcManager->SceneFinish();
}

void zNPCManagerSceneExit()
{
    zNPCManager* npcManager = zNPCManagerInstance();
    npcManager->SceneExit();
}

void zNPCManagerSceneReset()
{
    zNPCManager* npcManager = zNPCManagerInstance();
    npcManager->SceneReset();
}

void zNPCManagerScenePostInit()
{
    zNPCManager* npcManager = zNPCManagerInstance();
    npcManager->ScenePostInit();
}

void zNPCManagerSceneSetup()
{
    zNPCManager* npcManager = zNPCManagerInstance();
    npcManager->SceneSetup();
}

void zNPCManagerSceneRender()
{
    zNPCManager* npcManager = zNPCManagerInstance();
    npcManager->SceneRender();
}

void zNPCManagerSceneTimestep(xScene* xscn, F32 dt)
{
    zNPCManager* npcManager = zNPCManagerInstance();
    npcManager->SceneTimestep(xscn, dt);
}

xAnimTable* zNPCManagerCreateAnimTable(eBrainType brainType)
{
    zNPCManager* npcManager = zNPCManagerInstance();
    return npcManager->CreateAnimTable(brainType);
}

S32 zNPCManagerAllocateNPCs()
{
    zNPCManager* npcManager = zNPCManagerInstance();
    return npcManager->AllocateNPCs();
}

void zNPCManagerLoadNPCs()
{
    zNPCManager* npcManager = zNPCManagerInstance();
    npcManager->LoadNPCs();
}

void zNPCManagerRegisterNPCs(zScene* zscn, S32* base_idx)
{
    zNPCManager* npcManager = zNPCManagerInstance();
    npcManager->RegisterNPCs(zscn, base_idx);
}

zNPCManager* zNPCManagerInstance()
{
    xASSERT_L(170, gNPCManager);
    return gNPCManager;
}

void zNPCManager::Startup()
{
    selfbase.id = 'NPCM';
    selfbase.baseType = eBaseTypeNPCManager;
    zNPCTypes_Prepare();

#if DEBUG || RELEASE
    SetupTweaks();
#endif

    const zNPCTypesTable* npcTypesTableEntry = zNPCTypes_GetTable();
    while (npcTypesTableEntry->npcType != eNPCTypeCount) {
        if (npcTypesTableEntry->compParametersInitFunction)
            npcTypesTableEntry->compParametersInitFunction();
        npcTypesTableEntry++;
    }
}

#if DEBUG || RELEASE
void zNPCManager::SetupTweaks()
{
    char buffer[256];
    const zNPCTypesTable* npcTypesTableEntry = zNPCTypes_GetTable();
    while (npcTypesTableEntry->npcType != eNPCTypeCount) {
#if RELEASE
        sprintf(buffer, "NPC|%s", npcTypesTableEntry->name);
#else
        sprintf(buffer, "NEW NPC|TYPES|%s", npcTypesTableEntry->name);
#endif

        if (npcTypesTableEntry->addDebugTweaksFunction) {
            zDebugTweakBranch::Create(buffer, npcTypesTableEntry->addDebugTweaksFunction);
        }
        npcTypesTableEntry++;
    }
}
#endif

void zNPCManager::Shutdown()
{
}

void zNPCManager::SceneInit()
{
    const zNPCTypesTable* npcTypesTableEntry = zNPCTypes_GetTable();
    while (npcTypesTableEntry->npcType != eNPCTypeCount) {
        if (npcTypesTableEntry->sceneInitFunction)
            npcTypesTableEntry->sceneInitFunction();
        npcTypesTableEntry++;
    }
}

void zNPCManager::ScenePrepare(S32)
{
    const zNPCTypesTable* npcTypesTableEntry = zNPCTypes_GetTable();
    while (npcTypesTableEntry->npcType != eNPCTypeCount) {
        if (npcTypesTableEntry->scenePrepareFunction)
            npcTypesTableEntry->scenePrepareFunction();
        npcTypesTableEntry++;
    }

    npcTypesTableEntry = zNPCTypes_GetTable();
    while (npcTypesTableEntry->npcType != eNPCTypeCount) {
        if (npcTypesTableEntry->combatDamageInitFunction)
            npcTypesTableEntry->combatDamageInitFunction();
        npcTypesTableEntry++;
    }
}

void zNPCManager::SceneFinish()
{
}

void zNPCManager::SceneExit()
{
#if !RELEASE
    for (U32 npcType = 0; npcType < eNPCTypeCount; npcType++) {
        for (S32 i = 0; i < numberOfNPC[npcType]; i++) {
            NPCs[npcType][i].SceneExit();
        }
    }
#endif

    const zNPCTypesTable* npcTypesTableEntry = zNPCTypes_GetTable();
    while (npcTypesTableEntry->npcType != eNPCTypeCount) {
        if (npcTypesTableEntry->sceneExitFunction)
            npcTypesTableEntry->sceneExitFunction();
        npcTypesTableEntry++;
    }
}

#if !RELEASE
void zNPCManager::SceneReset()
{
    for (U32 npcType = 0; npcType < eNPCTypeCount; npcType++) {
        for (S32 i = 0; i < numberOfNPC[npcType]; i++) {
            NPCs[npcType][i].SceneReset();
        }
    }

    const zNPCTypesTable* npcTypesTableEntry = zNPCTypes_GetTable();
    while (npcTypesTableEntry->npcType != eNPCTypeCount) {
        if (npcTypesTableEntry->sceneResetFunction)
            npcTypesTableEntry->sceneResetFunction();
        npcTypesTableEntry++;
    }
}
#endif

void zNPCManager::ScenePostInit()
{
}

void zNPCManager::SceneSetup()
{
    const zNPCTypesTable* npcTypesTableEntry = zNPCTypes_GetTable();
    while (npcTypesTableEntry->npcType != eNPCTypeCount) {
        if (npcTypesTableEntry->sceneSetupFunction)
            npcTypesTableEntry->sceneSetupFunction();
        npcTypesTableEntry++;
    }
}

xAnimTable* zNPCManager::CreateAnimTable(eBrainType brainType)
{
    eNPCType npcType = zNPCTypes_GetNPCType(brainType);
    if (npcType == eNPCType_None) {
        xFAIL_ONCE_L(DEBUG ? 339 : 310, "%s", "Invalid brain type encountered trying to search for animation table.");
    }

#if DEBUG
    xAnimTable* table = xAnimTableNew(GetBrainName(brainType), 0x2b);
#else
    xAnimTable* table = xAnimTableNew("AnimTable", 0x2b);
#endif

    const zNPCTypesTable* npcTypeTableEntry = zNPCTypes_GetTableEntry(npcType);

    if (npcTypeTableEntry->createAnimTableFunction == NULL) {
        zICompLogic::CreateAnimTable(table);
    } else {
        npcTypeTableEntry->createAnimTableFunction(table);
    }

    return table;
}

S32 zNPCManager::AllocateNPCs()
{
    S32 totalNPCCount = FindNumberOfNPCs(numberOfNPC);

    const zNPCTypesTable* npcTypesTableEntry = zNPCTypes_GetTable();
    while (npcTypesTableEntry->npcType != eNPCTypeCount) {
        eNPCType curNPCType = npcTypesTableEntry->npcType;
        xASSERT_FMT_L(DEBUG ? 378 : 349, curNPCType>=eNPCType_First || curNPCType<eNPCTypeCount, "There is something wrong with NPC Types Table. This should never happen!");

        if (numberOfNPC[curNPCType] > 0) {
            xASSERT_FMT_L(DEBUG ? 384 : 355, npcTypesTableEntry->allocateNPCFunction!=0, "This NpcType has no allocateNPCFunction associated with it. Please add a allocateNPCFunction in the field of this NPCType");

            NPCs[curNPCType] = npcTypesTableEntry->allocateNPCFunction(numberOfNPC[curNPCType]);
        }

        npcTypesTableEntry++;
    }

    return totalNPCCount;
}

S32 zNPCManager::FindNumberOfNPCs(S32* typeCount)
{
    for (S32 i = 0; i < eNPCTypeCount; i++) {
        typeCount[i] = 0;
    }

    S32 NPCNumber = 0, num_alldyn = xSTAssetCountByType('DYNA');
    for (S32 i = 0; i < num_alldyn; i++) {
        U32 dynAssetSize;
        const xDynAsset* dynAsset = (const xDynAsset*)xSTFindAssetByType('DYNA', i, &dynAssetSize);

        xASSERT_L(DEBUG ? 414 : 385, dynAsset);
        if (!dynAsset) {
            continue;
        }

        const zNPCTypesTable* npcTypesTableEntry = zNPCTypes_GetTable();
        while (npcTypesTableEntry->npcType != eNPCTypeCount) {
            if (dynAsset->type == npcTypesTableEntry->hashDynAssetTypeName) {
                eNPCType curNPCType = npcTypesTableEntry->npcType;
                xASSERT_FMT_L(DEBUG ? 426 : 397, curNPCType>=eNPCType_First || curNPCType<eNPCTypeCount, "There is something wrong with NPC Types Table. This should never happen!");

                if (npcTypesTableEntry->validateAssetFunction) {
                    npcTypesTableEntry->validateAssetFunction(dynAsset, dynAssetSize, npcTypesTableEntry->dynAssetTypeName);
                }

                typeCount[curNPCType]++;
                NPCNumber++;

                if (npcTypesTableEntry->childrenType != eNPCType_None) {
                    curNPCType = npcTypesTableEntry->childrenType;
                    xASSERT_FMT_L(DEBUG ? 445 : 416, curNPCType>=eNPCType_First || curNPCType<eNPCTypeCount, "There is something wrong with NPC Types Table. This should never happen!");

                    S32 numChildren = 0;
                    if (npcTypesTableEntry->countChildrenInAsset) {
                        numChildren = npcTypesTableEntry->countChildrenInAsset(dynAsset, dynAssetSize, npcTypesTableEntry->dynAssetTypeName);
                    }

                    typeCount[curNPCType] += numChildren;
                    NPCNumber += numChildren;
                }

                break;
            }

            npcTypesTableEntry++;
        }
    }

    return NPCNumber;
}

void zNPCManager::LoadNPCs()
{
    S32 curNPCNum[eNPCTypeCount];
    for (S32 i = 0; i < eNPCTypeCount; i++) {
        curNPCNum[i] = 0;
    }

    S32 num_alldyn = xSTAssetCountByType('DYNA');
    for (S32 i = 0; i < num_alldyn; i++) {
        U32 dynAssetSize;
        const xDynAsset* dynAsset = (const xDynAsset*)xSTFindAssetByType('DYNA', i, &dynAssetSize);

        xASSERT_L(DEBUG ? 485 : 456, dynAsset);
        if (!dynAsset) {
            continue;
        }

        const zNPCTypesTable* npcTypesTableEntry = zNPCTypes_GetTable();
        while (npcTypesTableEntry->npcType != eNPCTypeCount) {
            if (dynAsset->type == npcTypesTableEntry->hashDynAssetTypeName) {
                eNPCType curNPCType = npcTypesTableEntry->npcType;
                zNPCBase* curNPC = &NPCs[curNPCType][curNPCNum[curNPCType]];
                LoadNPC(curNPC, curNPCType, dynAsset, dynAssetSize);

                curNPCNum[curNPCType]++;

                if (npcTypesTableEntry->childrenType != eNPCType_None) {
                    S32 numChildren = 0;
                    curNPCType = npcTypesTableEntry->childrenType;
                    if (npcTypesTableEntry->countChildrenInAsset) {
                        numChildren = npcTypesTableEntry->countChildrenInAsset(dynAsset, dynAssetSize, npcTypesTableEntry->dynAssetTypeName);
                    }

                    zNPCBase* firstChildNPC = &NPCs[curNPCType][curNPCNum[curNPCType]];

                    for (S32 j = 0; j < numChildren; j++) {
                        zNPCBase* curChildNPC = &NPCs[curNPCType][curNPCNum[curNPCType]];
                        LoadNPC(curChildNPC, curNPCType, dynAsset, dynAssetSize);

                        curChildNPC->SetParent(curNPC);

                        curNPCNum[curNPCType]++;
                    }

                    curNPC->SetChildren(firstChildNPC, numChildren);
                }

                break;
            }

            npcTypesTableEntry++;
        }
    }

#if !RELEASE
    for (U32 npcType = 0; npcType < eNPCTypeCount; npcType++) {
        for (S32 i = 0; i < numberOfNPC[npcType]; i++) {
            NPCs[npcType][i].SceneEnter();
        }
    }
#endif
}

void zNPCManager::LoadNPC(zNPCBase* npc, eNPCType type, const xDynAsset* dynAsset, U32 dynAssetSize)
{
    xASSERT_L(DEBUG ? 552 : 515, dynAsset);

    zNPCAssetRATS* npcass = (zNPCAssetRATS*)dynAsset;
    xEntAsset* entass = &npcass->ent_asset;
    xBaseAsset* baseass = &npcass->ent_asset;

    entass->id = dynAsset->id;
    entass->linkCount = dynAsset->linkCount;
    entass->baseType = eBaseTypeNPC;

    xASSERT_FMT_L(DEBUG ? 573 : 536, entass->modelInfoID, "'0x%08X' not assigned a model", entass->id);

    U32 size;
    xModelAssetInfo* modelInfoAssetMINF, *modelInfoAsset;

    U32 aid_minf = xStrHashCat(entass->modelInfoID, ".MINF");
    modelInfoAssetMINF = (xModelAssetInfo*)xSTFindAsset(aid_minf, &size);

    modelInfoAsset = (xModelAssetInfo*)xSTFindAsset(entass->modelInfoID, &size);

    if (modelInfoAssetMINF) {
        entass->modelInfoID = aid_minf;
        npc->Load(type, modelInfoAssetMINF, (zNPCAssetRATS*)dynAsset, dynAssetSize);
    }
    else if (modelInfoAsset) {
        npc->Load(type, modelInfoAsset, (zNPCAssetRATS*)dynAsset, dynAssetSize);
    } else {
        xFAIL_FMT_L(DEBUG ? 610 : 573, "NPC \"%s\" is missing model \"%s\"", xSTAssetName(entass->id), xSTAssetName(entass->modelInfoID));
    }
}

void zNPCManager::RegisterNPCs(zScene*, S32* base_idx)
{
    for (U32 npcType = 0; npcType < eNPCTypeCount; npcType++) {
        for (S32 i = 0; i < numberOfNPC[npcType]; i++) {
            zNPCBase* curNPC = &NPCs[npcType][i];

            zSceneSet(curNPC, *base_idx);

            zSceneObjHashtableAdd(curNPC->id, curNPC);

            (*base_idx)++;
        }
    }
}

void zNPCManager::SceneRender()
{
    for (U32 npcType = 0; npcType < eNPCTypeCount; npcType++) {
        if (zNPCTypes_GetTableEntry((eNPCType)npcType)->isChildNPC) {
            continue;
        }

        for (S32 i = 0; i < numberOfNPC[npcType]; i++) {
            NPCs[npcType][i].BaseRender();
        }
    }
}

#if RELEASE
void zNPCManager::SceneReset()
{
    for (U32 npcType = 0; npcType < eNPCTypeCount; npcType++) {
        if (zNPCTypes_GetTableEntry((eNPCType)npcType)->isChildNPC) {
            continue;
        }

        for (S32 i = 0; i < numberOfNPC[npcType]; i++) {
            NPCs[npcType][i].BaseReset();
        }
    }

    const zNPCTypesTable* npcTypesTableEntry = zNPCTypes_GetTable();
    while (npcTypesTableEntry->npcType != eNPCTypeCount) {
        if (npcTypesTableEntry->sceneResetFunction)
            npcTypesTableEntry->sceneResetFunction();
        npcTypesTableEntry++;
    }
}
#endif

void zNPCManager::SceneTimestep(xScene* xscn, F32 dt)
{
    xVec3* playerPos = &CURPLAYERS[0]->frame->mat.pos;

    for (U32 npcType = 0; npcType < eNPCTypeCount; npcType++) {
        if (zNPCTypes_GetTableEntry((eNPCType)npcType)->isChildNPC) {
            continue;
        }

        for (S32 i = 0; i < numberOfNPC[npcType]; i++) {
#if RELEASE
            NPCs[npcType][i].BaseTimeStep(playerPos, xscn, dt);
#else
            NPCs[npcType][i].TimeStep(playerPos, xscn, dt);
#endif
        }
    }
}
