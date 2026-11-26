#include "xCommon.h"
#include "zModuleMgr.h"

#include "containers.h"
#include "xFont.h"
#include "xordarray.h"
#include "xPad.h"
#include "zCutsceneMgr.h"
#include "zGame.h"
#include "zGlobals.h"
#include "zModuleMgr_Registry.h"
#include "zScene.h"

#define MAX_MODULES 32

static st_XORDEREDARRAY g_modlist;
static st_XORDEREDARRAY g_modlistUpdateSort;
static st_XORDEREDARRAY g_modlistRenderSort;

void zModule::Mgr_Startup()
{
    XOrdInit(&g_modlist, MAX_MODULES, eMemMgrTag_OrdArrayMgr, FALSE);
    XOrdInit(&g_modlistUpdateSort, MAX_MODULES, eMemMgrTag_OrdArrayMgr, FALSE);
    XOrdInit(&g_modlistRenderSort, MAX_MODULES, eMemMgrTag_OrdArrayMgr, FALSE);

    zModuleMgr_Registry_Startup();
}

void zModule::Mgr_Shutdown()
{
    zModuleMgr_Registry_Shutdown();

    XOrdDone(&g_modlistRenderSort, FALSE);
    XOrdDone(&g_modlistUpdateSort, FALSE);
    XOrdDone(&g_modlist, FALSE);
}

S32 OrdComp_updatePriority(void* vkey, void* vitem)
{
    zModule* key = (zModule*)vkey;
    zModule* item = (zModule*)vitem;

    if (key->updatePriority < item->updatePriority)
        return -1;
    if (key->updatePriority > item->updatePriority)
        return 1;
    return 0;
}

S32 OrdComp_renderPriority(void* vkey, void* vitem)
{
    zModule* key = (zModule*)vkey;
    zModule* item = (zModule*)vitem;

    if (key->renderPriority < item->renderPriority)
        return -1;
    if (key->renderPriority > item->renderPriority)
        return 1;
    return 0;
}

void zModuleMgr_RegisterModule(zModule* mod, S32 moduleID, const char* moduleName, enModulePriority updPriority, enModulePriority rendPriority)
{
    mod->tag_module = moduleID;
    mod->nam_module = moduleName;
    mod->updatePriority = updPriority;
    mod->renderPriority = rendPriority;
    XOrdAppend(&g_modlist, mod);

    XOrdInsert(&g_modlistUpdateSort, mod, OrdComp_updatePriority);
    XOrdInsert(&g_modlistRenderSort, mod, OrdComp_renderPriority);

    mod->ModulePrepUse();
}

zModule::zModule()
{
    flg_skipUpdates = 0;
    flg_skipRenders = 0;
    flg_useBucketRender = 1;
    flg_useLayerRender = 1;
    flg_updateWhenPaused = 1;
    flg_updateInCinematic = 1;
    flg_notUsed = 0;
    tag_module = 0;
    nam_module = NULL;
    updatePriority = MODULE_PRIORITY_NORMAL;
    renderPriority = MODULE_PRIORITY_NORMAL;
}

void zModule::Mgr_ScenePrepare()
{
    S32 num_mod = g_modlistUpdateSort.cnt;
    for (S32 i = 0; i < num_mod; i++) {
        zModule* mod = (zModule*)g_modlistUpdateSort.list[i];

        mod->ScenePrepare();
    }
}

void zModule::Mgr_SceneFinish()
{
    S32 num_mod = g_modlistUpdateSort.cnt;
    for (S32 i = num_mod - 1; i >= 0; i--) {
        zModule* mod = (zModule*)g_modlistUpdateSort.list[i];

        mod->SceneFinish();
    }
}

void zModule::Mgr_SceneInit()
{
    S32 num_mod = g_modlistUpdateSort.cnt;
    for (S32 i = 0; i < num_mod; i++) {
        zModule* mod = (zModule*)g_modlistUpdateSort.list[i];

        mod->AddModuleTweaks();
        mod->SceneInit();
    }
}

void zModule::Mgr_ScenePostInit()
{
    S32 num_mod = g_modlistUpdateSort.cnt;
    for (S32 i = 0; i < num_mod; i++) {
        zModule* mod = (zModule*)g_modlistUpdateSort.list[i];

        mod->ScenePostInit();
    }
}

void zModule::Mgr_SceneSetup()
{
    S32 num_mod = g_modlistUpdateSort.cnt;
    for (S32 i = 0; i < num_mod; i++) {
        zModule* mod = (zModule*)g_modlistUpdateSort.list[i];

        mod->SceneSetup();
    }
}

void zModule::Mgr_ScenePostSetup()
{
    S32 num_mod = g_modlistUpdateSort.cnt;
    for (S32 i = 0; i < num_mod; i++) {
        zModule* mod = (zModule*)g_modlistUpdateSort.list[i];

        mod->ScenePostSetup();
    }
}

void zModule::Mgr_SceneExit()
{
    S32 num_mod = g_modlistUpdateSort.cnt;
    for (S32 i = 0; i < num_mod; i++) {
        zModule* mod = (zModule*)g_modlistUpdateSort.list[i];

        mod->SceneExit();
    }
}

void zModule::Mgr_SceneReset()
{
    S32 num_mod = g_modlistUpdateSort.cnt;
    for (S32 i = 0; i < num_mod; i++) {
        zModule* mod = (zModule*)g_modlistUpdateSort.list[i];

        mod->SceneReset();
    }
}

void zModule::Mgr_Timestep(F32 dt)
{
    S32 isPaused = zGameIsPaused();
    S32 isCinematic = globals.cmgr && globals.cmgr->csn && globals.cmgr->csn->Ready;

    S32 num_mod = g_modlistUpdateSort.cnt;
    for (S32 i = 0; i < num_mod; i++) {
        zModule* mod = (zModule*)g_modlistUpdateSort.list[i];
        if (mod->flg_skipUpdates)
            continue;
        if (isPaused && !mod->flg_updateWhenPaused)
            continue;
        if (isCinematic && !mod->flg_updateInCinematic)
            continue;

        mod->Timestep(dt);
    }
}

void zModule::Mgr_PostTimestep(F32 dt)
{
    S32 isPaused = zGameIsPaused();
    S32 isCinematic = globals.cmgr && globals.cmgr->csn && globals.cmgr->csn->Ready;

    S32 num_mod = g_modlistUpdateSort.cnt;
    for (S32 i = 0; i < num_mod; i++) {
        zModule* mod = (zModule*)g_modlistUpdateSort.list[i];
        if (mod->flg_skipUpdates)
            continue;
        if (isPaused && !mod->flg_updateWhenPaused)
            continue;
        if (isCinematic && !mod->flg_updateInCinematic)
            continue;

        mod->PostTimestep(dt);
    }
}

void zModule::Mgr_RenderPrep()
{
    S32 isPaused = zGameIsPaused();
    S32 isCinematic = globals.cmgr && globals.cmgr->csn && globals.cmgr->csn->Ready;

    S32 num_mod = g_modlistRenderSort.cnt;
    for (S32 i = 0; i < num_mod; i++) {
        zModule* mod = (zModule*)g_modlistRenderSort.list[i];

        mod->lastAlphaLayer = eLAYER_COUNT;

        if (mod->flg_skipRenders)
            continue;
        if (isPaused && !mod->flg_updateWhenPaused)
            continue;
        if (isCinematic && !mod->flg_updateInCinematic)
            continue;

        mod->RenderPrep();
    }
}

void zModule::Mgr_RenderToBuckets()
{
    S32 isPaused = zGameIsPaused();
    S32 isCinematic = globals.cmgr && globals.cmgr->csn && globals.cmgr->csn->Ready;

    S32 num_mod = g_modlistRenderSort.cnt;
    for (S32 i = 0; i < num_mod; i++) {
        zModule* mod = (zModule*)g_modlistRenderSort.list[i];
        if (mod->flg_skipRenders)
            continue;
        if (!mod->flg_useBucketRender)
            continue;
        if (isPaused && !mod->flg_updateWhenPaused)
            continue;
        if (isCinematic && !mod->flg_updateInCinematic)
            continue;

        mod->RenderToBuckets();
    }
}

void zModule::Mgr_RenderLayer(zLayerType endLayer)
{
    S32 isPaused = zGameIsPaused();
    S32 isCinematic = globals.cmgr && globals.cmgr->csn && globals.cmgr->csn->Ready;

    S32 num_mod = g_modlistRenderSort.cnt;
    for (S32 i = 0; i < num_mod; i++) {
        zModule* mod = (zModule*)g_modlistRenderSort.list[i];
        if (mod->flg_skipRenders)
            continue;
        if (!mod->flg_useLayerRender)
            continue;
        if (isPaused && !mod->flg_updateWhenPaused)
            continue;
        if (isCinematic && !mod->flg_updateInCinematic)
            continue;

        S32 start = mod->lastAlphaLayer;
        if (start == eLAYER_COUNT)
            start = 0;
        else
            start++;

        xASSERT_L(432, start<eLAYER_COUNT);

        xASSERT_FMT_L(435, (endLayer >= start), "0x%08X - End: %d start: %d", mod->tag_module, endLayer, start);

        for (S32 j = start; j <= endLayer; j++) {
            zLayerType layer_curr = (zLayerType)j;
            mod->RenderAlphaLayer(layer_curr);
            mod->lastAlphaLayer = layer_curr;
        }
    }
}

void zModule::Mgr_RenderDone()
{
    S32 isPaused = zGameIsPaused();
    S32 isCinematic = globals.cmgr && globals.cmgr->csn && globals.cmgr->csn->Ready;

    S32 num_mod = g_modlistRenderSort.cnt;
    for (S32 i = 0; i < num_mod; i++) {
        zModule* mod = (zModule*)g_modlistRenderSort.list[i];
        if (mod->flg_skipRenders)
            continue;
        if (isPaused && !mod->flg_updateWhenPaused)
            continue;
        if (isCinematic && !mod->flg_updateInCinematic)
            continue;

        mod->RenderDone();
        mod->lastAlphaLayer = eLAYER_COUNT;
    }

    for (S32 i = 0; i < num_mod; i++) {
        zModule* mod = (zModule*)g_modlistRenderSort.list[i];
        mod->lastAlphaLayer = eLAYER_COUNT;
    }
}
