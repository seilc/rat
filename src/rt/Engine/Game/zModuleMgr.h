#ifndef ZMODULEMGR_H
#define ZMODULEMGR_H

#include "xEnt.h"
#include "zLayerType.h"

enum enModulePriority
{
    MODULE_PRIORITY_EARLY,
    MODULE_PRIORITY_NORMAL,
    MODULE_PRIORITY_LATE
};

struct zModule
{
    S32 flg_skipUpdates : 1;
    S32 flg_skipRenders : 1;
    S32 flg_useBucketRender : 1;
    S32 flg_useLayerRender : 1;
    S32 flg_updateWhenPaused : 1;
    S32 flg_updateInCinematic : 1;
    S32 flg_notUsed : 26;

protected:
    S32 tag_module;
    const char* nam_module;
    enModulePriority updatePriority;
    enModulePriority renderPriority;
    zLayerType lastAlphaLayer;

public:
    zModule();
    ~zModule() {}

    virtual void ScenePrepare() {}
    virtual void SceneFinish() {}
    virtual void SceneInit() {}
    virtual void ScenePostInit() {}
    virtual void SceneSetup() {}
    virtual void ScenePostSetup() {}
    virtual void SceneReset() {}
    virtual void SceneExit() {}
    virtual void Timestep(F32) {}
    virtual void PostTimestep(F32) {}
    virtual void RenderPrep() {}
    virtual void RenderDone() {}
    virtual void RenderToBuckets() {}
    virtual void RenderAlphaLayer(zLayerType) {}
    virtual void ModulePrepUse() {}
#if DEBUG || RELEASE
    virtual void AddModuleTweaks() {}
#else
    void AddModuleTweaks() {}
#endif

    static void Mgr_Startup();
    static void Mgr_Shutdown();
    static void Mgr_ScenePrepare();
    static void Mgr_SceneFinish();
    static void Mgr_SceneInit();
    static void Mgr_ScenePostInit();
    static void Mgr_SceneSetup();
    static void Mgr_ScenePostSetup();
    static void Mgr_SceneExit();
    static void Mgr_SceneReset();
    static void Mgr_Timestep(F32 dt);
    static void Mgr_PostTimestep(F32 dt);
    static void Mgr_RenderPrep();
    static void Mgr_RenderToBuckets();
    static void Mgr_RenderLayer(zLayerType endLayer);
    static void Mgr_RenderDone();

    friend S32 OrdComp_updatePriority(void* vkey, void* vitem);
    friend S32 OrdComp_renderPriority(void* vkey, void* vitem);
    friend void zModuleMgr_RegisterModule(zModule* mod, S32 moduleID, const char* moduleName, enModulePriority updPriority, enModulePriority rendPriority);
};

S32 OrdComp_updatePriority(void* vkey, void* vitem);
S32 OrdComp_renderPriority(void* vkey, void* vitem);
void zModuleMgr_RegisterModule(zModule* mod, S32 moduleID, const char* moduleName, enModulePriority updPriority, enModulePriority rendPriority);

#endif
