#include "xCommon.h"
#include "zModuleExample.h"

#include "xutil.h"
#include "zModuleMgr.h"

namespace ModuleExample {

class zMODExample : public zModule
{
public:
    virtual void ScenePrepare();
    virtual void SceneFinish();
    virtual void SceneInit();
    virtual void ScenePostInit();
    virtual void SceneSetup();
    virtual void ScenePostSetup();
    virtual void SceneReset();
    virtual void SceneExit();
    virtual void Timestep(F32);
    virtual void PostTimestep(F32);
    virtual void RenderPrep();
    virtual void RenderDone();
    virtual void RenderToBuckets();
    virtual void RenderAlphaLayer(zLayerType alphaLayer);
    virtual void ModulePrepUse();
};

static en_VERBOSE_MSGLEVEL egMgsLvl = DBML_VDBG;
static zMODExample* g_mod_example;

void zModuleExample_Startup()
{
    static zMODExample mod_example;
    g_mod_example = &mod_example;
    zModuleMgr_RegisterModule(&mod_example, IDTAG('X','M','P','L'), "Example Module", MODULE_PRIORITY_NORMAL, MODULE_PRIORITY_NORMAL);

    DBprintf(egMgsLvl, "Startup of the Example Module\n");
}

void zModuleExample_Shutdown()
{
    DBprintf(egMgsLvl, "Shutdown of the Example Module\n");
}

void zMODExample::ModulePrepUse()
{
    DBprintf(egMgsLvl, "ModulePrepUse\n");
    DBprintf(egMgsLvl, "- called when Module registered with Manager\n");
    DBprintf(egMgsLvl, "\n");
}

void zMODExample::ScenePrepare()
{
    DBprintf(egMgsLvl, "ScenePrepare\n");
    DBprintf(egMgsLvl, "- called after HIP loading scene assest\n");
    DBprintf(egMgsLvl, "- called prior to 'Init' processing\n");
    DBprintf(egMgsLvl, "\n");
}

void zMODExample::SceneFinish()
{
    DBprintf(egMgsLvl, "SceneFinish\n");
    DBprintf(egMgsLvl, "- called after all modules 'SceneExit' called\n");
    DBprintf(egMgsLvl, "- called prior to unloading of scene assets\n");
    DBprintf(egMgsLvl, "\n");
}

void zMODExample::SceneInit()
{
    DBprintf(egMgsLvl, "SceneInit\n");
    DBprintf(egMgsLvl, "- called during zSceneInit\n");
    DBprintf(egMgsLvl, "\n");
}

void zMODExample::ScenePostInit()
{
    DBprintf(egMgsLvl, "ScenePostInit\n");
    DBprintf(egMgsLvl, "- called after all modules' SceneInit\n");
    DBprintf(egMgsLvl, "\n");
}

void zMODExample::SceneSetup()
{
    DBprintf(egMgsLvl, "SceneSetup\n");
    DBprintf(egMgsLvl, "- called during zSceneSetup\n");
    DBprintf(egMgsLvl, "\n");
}

void zMODExample::ScenePostSetup()
{
    DBprintf(egMgsLvl, "ScenePostSetup\n");
    DBprintf(egMgsLvl, "- called after all modules' SceneSetup\n");
    DBprintf(egMgsLvl, "\n");
}

void zMODExample::SceneReset()
{
    DBprintf(egMgsLvl, "SceneReset\n");
    DBprintf(egMgsLvl, "- called during zSceneReset\n");
    DBprintf(egMgsLvl, "\n");
}

void zMODExample::SceneExit()
{
    DBprintf(egMgsLvl, "SceneExit\n");
    DBprintf(egMgsLvl, "- called during zSceneExit\n");
    DBprintf(egMgsLvl, "- called after all game objects receive eEventRoomEnd and eEventSceneEnd events\n");
    DBprintf(egMgsLvl, "\n");
}

void zMODExample::Timestep(F32)
{
    DBprintf(egMgsLvl, "Timestep\n");
    DBprintf(egMgsLvl, "- normal update timestep processing for module\n");
    DBprintf(egMgsLvl, "\n");
}

void zMODExample::PostTimestep(F32)
{
    // BUG: prints "Timestep", not "PostTimestep"
    DBprintf(egMgsLvl, "Timestep\n");
    DBprintf(egMgsLvl, "- called after all modules do normal Timestep\n");
    DBprintf(egMgsLvl, "\n");
}

void zMODExample::RenderPrep()
{
    DBprintf(egMgsLvl, "Called before rendering\n");
    DBprintf(egMgsLvl, "- Potential use to allocate special buffers, etc\n");
    DBprintf(egMgsLvl, "\n");
}

void zMODExample::RenderDone()
{
    DBprintf(egMgsLvl, "Called after rendering\n");
    DBprintf(egMgsLvl, "- Potential use to clean up temporary data the module used to render things in batch\n");
    DBprintf(egMgsLvl, "\n");
}

void zMODExample::RenderToBuckets()
{
    DBprintf(egMgsLvl, "RenderToBuckets\n");
    DBprintf(egMgsLvl, "- Called for rendering\n");
    DBprintf(egMgsLvl, "- Should handle bulk of rendering needs for Modules (and its children)\n");
    DBprintf(egMgsLvl, "- xModelRender adds models to bucket rendering\n");
    DBprintf(egMgsLvl, "- Other opaque rendering can occur here\n");
    DBprintf(egMgsLvl, "- Rendering with alpha translucency or other\n");
    DBprintf(egMgsLvl, "  special rendering needs can be rendered using\n");
    DBprintf(egMgsLvl, "  the RenderAlphaLayer for more sorting control\n");
    DBprintf(egMgsLvl, "\n");
}

void zMODExample::RenderAlphaLayer(zLayerType alphaLayer)
{
    DBprintf(egMgsLvl, "RenderAlphaLayer\n");
    DBprintf(egMgsLvl, "- Called for each alpha render layer\n");
    DBprintf(egMgsLvl, "- (Currently layer %d)\n", alphaLayer);
    DBprintf(egMgsLvl, "\n");
}

} // namespace ModuleExample
