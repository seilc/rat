#ifndef ZMINIGAMEMANAGER_H
#define ZMINIGAMEMANAGER_H

#include "zLightWeight.h"
#include "zMiniGameBase.h"

class zMiniGameManager : public zLightweightSystemBase
{
public:
    bool IsMiniGamePaused()
    {
        if (currentGame) {
            return currentGame->IsPaused();
        }
        return false;
    }

private:
    zMiniGameBase* currentGame;
    S32 userSetPlayerCount;
};

#endif
