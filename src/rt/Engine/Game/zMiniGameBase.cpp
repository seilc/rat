#include "xCommon.h"
#include "zMiniGameBase.h"

#include "xColor.h"
#include "xDraw.h"
#include "xMath.h"
#include "xMath3.h"
#include "xString.h"
#include "xTimer.h"
#include "zCam.h"
#include "zGlobals.h"
#include "zPlayer.h"
#include "zScene.h"

#include <stdio.h>

zMiniGameBase* zMiniGameBase::currentMiniGame;

zMiniGameBase::zMiniGameBase(zMiniGameAsset* a)
{
    xBaseInit(this, a);
    asset = a;

    currentMiniGame = this;
}

void zMiniGameBase::SceneSetup()
{
    zScene* s = globals.sceneCur;
    playerCount = s->baseCount[eBaseTypePlayer];
    gameDone = false;
    SwitchGameState(GAMESTATE_GAME_START);
}

zMiniGameBase* zMiniGameBase::GetGame()
{
    xASSERT_L(34, currentMiniGame);
    return currentMiniGame;
}

void zMiniGameBase::SwitchGameState(eMiniGameState newState)
{
    switch (gameState) {
    case GAMESTATE_GAME_START:
        GameStartExit();
        break;
    case GAMESTATE_STAGE_START:
        StageStartExit();
        break;
    case GAMESTATE_GAME_PLAY:
        GamePlayExit();
        break;
    case GAMESTATE_STAGE_END:
        StageEndExit();
        break;
    case GAMESTATE_GAME_END:
        GameEndExit();
        break;
    }

    gameState = newState;

    switch (gameState) {
    case GAMESTATE_GAME_START:
        GameStartEnter();
        break;
    case GAMESTATE_STAGE_START:
        StageStartEnter();
        break;
    case GAMESTATE_GAME_PLAY:
        GamePlayEnter();
        break;
    case GAMESTATE_STAGE_END:
        StageEndEnter();
        break;
    case GAMESTATE_GAME_END:
        GameEndEnter();
        break;
    }
}

void zMiniGameBase::Update(F32 dt)
{
    switch (gameState) {
    case GAMESTATE_GAME_START:
        GameStartUpdate(dt);
        break;
    case GAMESTATE_STAGE_START:
        StageStartUpdate(dt);
        break;
    case GAMESTATE_GAME_PLAY:
        GamePlayUpdate(dt);
        break;
    case GAMESTATE_STAGE_END:
        StageEndUpdate(dt);
        break;
    case GAMESTATE_GAME_END:
        GameEndUpdate(dt);
        break;
    }
}

zMiniGameType1::zMiniGameType1(zMiniGameAsset* a) : zMiniGameBase(a)
{
    gameType = MG_GAMETYPE_TYPE1;
}

void zMiniGameType1::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    if (globals.mgManager->IsMiniGameRunning()) {
        xASSERT_FMT_L(115, 0, "Currently we only support 1 mini game per scene");
        return;
    }

    xDynAssetValidate_L(119, asset, asset_size, zMiniGameAssetType1, ZMINIGAME_TYPE_PREFIX ":TestConfig1");
    zMiniGameType1* instance = new (&data) zMiniGameType1((zMiniGameAsset*)&asset);
    if (instance)
        globals.mgManager->RegisterMiniGame(instance);
    else
        xASSERT_FMT_L(124, 0, "Failed to create mini game instance");
}

void zMiniGameType1::SceneSetup()
{
    yourWorth = GetAsset()->num;
}

void zMiniGameType1::Update(F32)
{
    xTimer* timer = (xTimer*)zSceneFindObject(xStrHash("PLYR_01_TIMER"));
    const xMat4x3& cammat = zCamGetMatrix();

    F32 sec = 0.0f;
    if (timer)
        sec = timer->secondsLeft;
    if (sec < 0.0f)
        sec = 0.0f;

    xVec3 txtPos;
    txtPos.AddScale(cammat.pos, cammat.up, 5.0f);
    txtPos.AddScale(txtPos, cammat.at, 10.0f);
    xDrawSetColor(g_GREEN);
    char messageTxt[128];
    sprintf(messageTxt, "%f seconds left.\n", sec);
    xDrawText(txtPos, messageTxt);

    if (yourWorth > 0)
        yourWorth--;
}

void zMiniGameType1::Render()
{
}

F32 GetAngleDiff(const xVec3* startDir, const xVec3* endDir)
{
    xASSERT_L(168, startDir);
    xASSERT_L(169, endDir);

    F32 angle_caller = xatan2(endDir->x, endDir->z);
    F32 angle_true = xatan2(startDir->x, startDir->z);

    return xDangleClamp(angle_caller - angle_true);
}

F32 ApplyAngleDiff(xMat3x3* mat, F32 angle_difference, F32 dt, F32 turning_speed)
{
    F32 delta_angle;
    if (dt > 0.0f && turning_speed > 0.0f)
        delta_angle = CLAMP(angle_difference, -turning_speed * dt, turning_speed * dt);
    else
        delta_angle = angle_difference;

    xMat3x3 drot;
    xMat3x3Rot(&drot, &xVec3::m_UnitAxisY, delta_angle);
    xMat3x3Mul(mat, mat, &drot);
    return delta_angle;
}
