#ifndef ZMINIGAMEMANAGER_H
#define ZMINIGAMEMANAGER_H

#include "zLightWeight.h"
#include "zMiniGameBase.h"

class zMiniGameManager : public zLightweightSystemBase
{
private:
    zMiniGameBase* currentGame;
    S32 userSetPlayerCount;
};

#endif
