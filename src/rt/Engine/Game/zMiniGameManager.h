#ifndef ZMINIGAMEMANAGER_H
#define ZMINIGAMEMANAGER_H

#include "zLightWeight.h"
#include "zMiniGameBase.h"

struct zMiniGameManager : zLightweightSystemBase
{
private:
    zMiniGameBase* currentGame;
    S32 userSetPlayerCount;
};

#endif
