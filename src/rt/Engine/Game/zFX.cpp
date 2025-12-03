#include "zFX.h"

#include "zFXShockwave.h"
#include "zFXScreenDecal.h"
#include "zFXGoo.h"
#include "zNPCProjectileGrubFX.h"
#include "zParSysSnowsplosion.h"

U32 gFXSurfaceFlags = 0;

void zFXModelRender()
{
    zFXShockwaveRender();
    zNPCProjectileGrubFX_RenderModels();
}

void zFXPostModelRender()
{
}

void zFXScreenRender()
{
    zFXScreenDecalRender();
}

void zFXUpdate(F32 dt)
{
    zFXGooUpdate(dt);
    xFXUpdate(dt);
    zFXShockwaveUpdate(dt);
    zFXScreenDecalUpdate(dt);
    zNPCProjectileGrubFX_Update(dt);
}

void zFX_SceneEnter(RpWorld* world)
{
    xFXanimUV2PSetTexture(NULL);
    xFX_SceneEnter(world);
    zFXGoo_SceneEnter();
    zFXShockwaveSceneEnter(6);
    zParSysSnowsplosionSceneSetup();
    zFXScreenDecalSceneEnter();
    zNPCProjectileGrubFX_SceneSetup();
}

void zFX_SceneExit(RpWorld* world)
{
    xFX_SceneExit(world);
    zFXGoo_SceneExit();
    zFXShockwaveSceneExit();
    zNPCProjectileGrubFX_SceneExit();
}

void zFX_SceneReset()
{
    zFXGoo_SceneReset();
    zFXShockwaveSceneReset();
    zFXScreenDecalSceneReset();
    zNPCProjectileGrubFX_SceneReset();
}
