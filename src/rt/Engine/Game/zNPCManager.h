#ifndef ZNPCMANAGER_H
#define ZNPCMANAGER_H

#include "xBase.h"
#include "xRMemData.h"
#include "zBrainTypes.h"
#include "zNPCTypes.h"

class zNPCBase;
struct xAnimTable;
struct xDynAsset;
struct xScene;
struct zScene;

class zNPCManager : public RyzMemData
{
public:
    void Startup();
#if DEBUG || RELEASE
    void SetupTweaks();
#endif
    void Shutdown();
    void SceneInit();
    void ScenePrepare(S32);
    void SceneFinish();
    void SceneExit();
    void SceneReset();
    void ScenePostInit();
    void SceneSetup();
    xAnimTable* CreateAnimTable(eBrainType brainType);
    S32 AllocateNPCs();
    S32 FindNumberOfNPCs(S32* typeCount);
    void LoadNPCs();
    void LoadNPC(zNPCBase* npc, eNPCType type, const xDynAsset* dynAsset, U32 dynAssetSize);
    void RegisterNPCs(zScene*, S32* base_idx);
    void SceneRender();
    void SceneTimestep(xScene* xscn, F32 dt);

private:
    xBase selfbase;
    zNPCBase* NPCs[eNPCTypeCount];
    S32 numberOfNPC[eNPCTypeCount];
};

void zNPCManagerStartup();
void zNPCManagerShutdown();
void zNPCManagerSceneInit();
void zNPCManagerScenePrepare(S32 npccnt);
void zNPCManagerSceneFinish();
void zNPCManagerSceneExit();
void zNPCManagerSceneReset();
void zNPCManagerScenePostInit();
void zNPCManagerSceneSetup();
void zNPCManagerSceneRender();
void zNPCManagerSceneTimestep(xScene* xscn, F32 dt);
xAnimTable* zNPCManagerCreateAnimTable(eBrainType brainType);
S32 zNPCManagerAllocateNPCs();
void zNPCManagerLoadNPCs();
void zNPCManagerRegisterNPCs(zScene* zscn, S32* base_idx);
zNPCManager* zNPCManagerInstance();

#endif
