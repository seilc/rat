#ifndef ZCONFIGURATION_H
#define ZCONFIGURATION_H

#include "types.h"

typedef enum _PLAYER_ENUM
{
    REMY,
    HUMAN_VEHICLE,
    RATBALL,
    MG1_PLAYER,
    MG2_RATONBALL_PLAYER,
    MG3_PUFFEROIDS_PLAYER,
    MG4_GRAPESTOMPER_PLAYER,
    MG5_CONGALINE_PLAYER,
    MG6_AVOIDTHESTUFF_PLAYER,
    PLAYER_COUNT
} PLAYER_ENUM;

U32 zConfig_getPlayerEnumIdx(U32 _player_tag);
U32 zConfig_getPlayerTag(U32 sceneID, S32 playerIdx);
S32 zConfig_getPlayerCount(U32 sceneID);
PLAYER_ENUM zConfig_getPlayerEnum(U32 sceneID, S32 playerIdx);
S32 zConfig_getPlayers(U32 sceneID, U32* players);

#if RELEASE
S32 zConfigGetSceneMemReserve(U32 sceneID);
#endif

#endif
