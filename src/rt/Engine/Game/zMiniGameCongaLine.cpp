#include "xCommon.h"
#include "zMiniGameCongaLine.h"

#include "containers.h"
#include "xCam.h"
#include "xPTankPool.h"
#include "zGlobals.h"
#include "zPlayer.h"
#include "zScene.h"

zMiniGameCongaLine::zMiniGameCongaLine(zMiniGameAsset* a) : zMiniGameBase(a)
{
    gameType = MG_GAMETYPE_CONGALINE;
}

void zMiniGameCongaLine::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    if (globals.mgManager->IsMiniGameRunning()) {
        xASSERT_FMT_L(21, 0, "Currently we only support 1 mini game per scene");
        return;
    }

    xDynAssetValidate_L(25, asset, asset_size, zMiniGameAssetCongaLine, ZMINIGAME_TYPE_PREFIX ":CongaLineConfig");
    zMiniGameCongaLine* instance = new (&data) zMiniGameCongaLine((zMiniGameAsset*)&asset);
    if (instance)
        globals.mgManager->RegisterMiniGame(instance);
    else
        xASSERT_FMT_L(30, 0, "Failed to create mini game instance");
}

void zMiniGameCongaLine::SceneSetup()
{
    Reset();
}

void zMiniGameCongaLine::Update(F32)
{
}

void zMiniGameCongaLine::Render()
{
}

void zMiniGameCongaLine::Reset()
{
}

#if DEBUG || RELEASE
void zMiniGameCongaLine::AddTweaks(const char*)
{
}
#endif
