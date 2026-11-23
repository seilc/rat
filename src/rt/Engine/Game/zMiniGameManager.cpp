#include "xCommon.h"
#include "zMiniGameManager.h"

#include "containers.h"
#include "iCamera.h"
#include "xCam.h"
#include "xCounter.h"
#include "xDebugTweak.h"
#include "xFile.h"
#include "xPTankPool.h"
#include "xstransvc.h"
#include "xutil.h"
#include "zGlobals.h"
#include "zMiniGameDefendTheBase.h"
#include "zMiniGameGrapeStomper.h"
#include "zMiniGameRatOnBall.h"
#include "zPlayer.h"
#include "zScene.h"

#include <string.h>

namespace
{
    const char* TWEAK_PREFIX = "MiniGame";
    bool parallel;
}

struct PlayerCreationData
{
    U32 gameType;
    void (*create)(xDynAsset*);
};

static PlayerCreationData playerCreationTable[] = {
    { xStrHash("MiniGame:DefendBaseConfig"), zMiniGameDefendTheBase::CreatePlayers },
    { xStrHash("MiniGame:RatOnBallConfig"), zMiniGameRatOnBall::CreatePlayers },
    { xStrHash("MiniGame:GrapeStomperConfig"), zMiniGameGrapeStomper::CreatePlayers },
};

void zMiniGameManager::Startup()
{
    static zMiniGameManager mgr;
    globals.mgManager = &mgr;
    mgr.UnregisterMiniGame();
    mgr.userSetPlayerCount = 1;
}

void zMiniGameManager::SceneInit()
{
    zMiniGameManager* mgr = globals.mgManager;
    zLightweightManager::RegisterLightweight(mgr);

    S32 dynSize = xSTAssetCountByType('DYNA');
    for (S32 i = 0, cnt = 0; i < dynSize; i++) {
        U32 assetSize;
        xDynAsset* a = (xDynAsset*)xSTFindAssetByType('DYNA', i, &assetSize);
        if (!a) {
            continue;
        }

        for (S32 j = 0; j < ARRAY_LENGTH(playerCreationTable); j++) {
            if (a->type == playerCreationTable[j].gameType) {
                playerCreationTable[j].create(a);
                return;
            }
        }
    }
}

static void SetCameraProj(const tweak_info& info)
{
    parallel = strstr(info.name.text, "Parallel") != NULL;
    iCameraSetOrthoProjection(xglobals->screen->icam, parallel);
}

#if DEBUG || RELEASE
static const tweak_callback SetCamProjCB = { SetCameraProj };
#endif

void zMiniGameManager::SceneSetup()
{
    if (IsMiniGameRunning()) {
        currentGame->SceneSetup();
#if DEBUG || RELEASE
        currentGame->AddTweaks(TWEAK_PREFIX);

        const char* prefix = TWEAK_PREFIX;
        xAUTOTWEAK_MESSAGE(prefix, "|Camera Projection|Parallel", "Change", &SetCamProjCB, NULL, 0, false);
        xAUTOTWEAK_MESSAGE(prefix, "|Camera Projection|Perspective", "Change", &SetCamProjCB, NULL, 0, false);
#endif
    }
}

bool zMiniGameManager::IsParallelCam()
{
    return parallel;
}

void zMiniGameManager::Reset()
{
    if (IsMiniGameRunning())
        currentGame->Reset();
}

void zMiniGameManager::Update(F32 dt)
{
    if (IsMiniGameRunning()) {
        currentGame->Update(dt);
    }
}

void zMiniGameManager::Render()
{
    if (IsMiniGameRunning()) {
        currentGame->Render();
    }
}

void zMiniGameManager::PostRender2d() const
{
}

void zMiniGameManager::PostRender3d() const
{
}

void zMiniGameManager::Exit()
{
    zMiniGameManager* mgr = globals.mgManager;
    mgr->UnregisterMiniGame();

#if DEBUG || RELEASE
    xDebugRemoveTweaks(TWEAK_PREFIX, NULL, NULL);
#endif
}

static U32 sMiniGameSceneMap[MG_GAMETYPE_COUNT][MAX_PLAYERS_PER_SCENE] = {
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { IDTAG('M','G','5','6'), IDTAG('M','G','5','5'), 0, 0 },
    { IDTAG('M','G','5','7'), IDTAG('M','G','5','7'), IDTAG('M','G','5','7'), IDTAG('M','G','5','7') },
    { 0, IDTAG('M','G','6','0'), 0, 0 },
    { IDTAG('M','G','5','8'), IDTAG('M','G','6','5'), 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
};

U32 zMiniGameManagerGetSceneID(xCounter* counter)
{
    xASSERT_L(146, counter->count > 0 && counter->count < MG_GAMETYPE_COUNT);
    return sMiniGameSceneMap[counter->count][globals.mgManager->GetUserSetPlayerCount() - 1];
}
