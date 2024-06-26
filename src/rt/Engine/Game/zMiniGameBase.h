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

struct zMiniGameBase : xBase
{
protected:
    ezMiniGameTypes gameType;
    zMiniGameAsset* asset;
    S32 playerCount;
    U8 gameDone;
    xBase* dispatcher;
    eMiniGameState gameState;

public:
    void* __vptr; // TEMP
};

#endif
