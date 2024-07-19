#ifndef ZMINIGAMEBASE_H
#define ZMINIGAMEBASE_H

#include "xBase.h"
#include "zMiniGameTypes.h"
#include "zMiniGameAsset.h"

enum eMiniGameState
{
    GAMESTATE_GAME_START,
    GAMESTATE_STAGE_START,
    GAMESTATE_GAME_PLAY,
    GAMESTATE_STAGE_END,
    GAMESTATE_GAME_END
};

class zMiniGameBase : public xBase
{
protected:
    ezMiniGameTypes gameType;
    zMiniGameAsset* asset;
    S32 playerCount;
    bool gameDone;
    xBase* dispatcher;
    eMiniGameState gameState;

public:
    virtual void SwitchGameState(eMiniGameState newState);
    virtual void GameStartUpdate(F32);
    virtual void GameStartEnter();
    virtual void GameStartExit();
    virtual void StageStartUpdate(F32);
    virtual void StageStartEnter();
    virtual void StageStartExit();
    virtual void GamePlayUpdate(F32);
    virtual void GamePlayEnter();
    virtual void GamePlayExit();
    virtual void StageEndUpdate(F32);
    virtual void StageEndEnter();
    virtual void StageEndExit();
    virtual void GameEndUpdate(F32);
    virtual void GameEndEnter();
    virtual void GameEndExit();
    virtual bool IsGameDone();
    virtual bool ShouldUpdatePlayer();
    virtual bool ShouldRenderPlayer();
    virtual bool IsPaused() { return false; }
    virtual void SceneSetup();
    virtual void Reset();
    virtual void Update(F32 dt);
    virtual void Render();
    virtual void PostRender2d() const;
    virtual void PostRender3d() const;
    virtual void Exit();
    virtual void SetRespawnPosition(xMat4x3*);
    virtual void AddTweaks(const char*);
};

#endif
