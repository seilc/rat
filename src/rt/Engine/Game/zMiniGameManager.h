#ifndef ZMINIGAMEMANAGER_H
#define ZMINIGAMEMANAGER_H

#include "zLightWeight.h"
#include "zMiniGameBase.h"

class zMiniGameManager : public zLightweightSystemBase
{
public:
    bool IsMiniGameRunning() const { return currentGame != NULL; }

    void RegisterMiniGame(zMiniGameBase* game)
    {
        currentGame = game;
    }

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

#endif
