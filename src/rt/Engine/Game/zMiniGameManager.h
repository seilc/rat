#ifndef ZMINIGAMEMANAGER_H
#define ZMINIGAMEMANAGER_H

#include "zLightWeight.h"
#include "zMiniGameBase.h"

typedef struct _xCounter xCounter;

class zMiniGameManager : public zLightweightSystemBase
{
public:
    bool IsMiniGameRunning() const { return currentGame != NULL; }

    void RegisterMiniGame(zMiniGameBase* game)
    {
        currentGame = game;
    }

    void UnregisterMiniGame()
    {
        currentGame = NULL;
    }

    static void Startup();
    static void SceneInit();

    virtual void SceneSetup();
    virtual void Reset();
    virtual void Update(F32 dt);
    virtual void Render();
    virtual void PostRender2d() const;
    virtual void PostRender3d() const;
    virtual void Exit();

    S32 GetUserSetPlayerCount() { return userSetPlayerCount; }

    bool IsMiniGamePaused()
    {
        if (currentGame) {
            return currentGame->IsPaused();
        }
        return false;
    }

    bool IsParallelCam();

private:
    zMiniGameBase* currentGame;
    S32 userSetPlayerCount;
};

U32 zMiniGameManagerGetSceneID(xCounter* counter);

#endif
