#ifndef ZGAMESTATE_H
#define ZGAMESTATE_H

#include "types.h"

typedef enum _GameOstrich
{
    eGameOstrich_Loading,
    eGameOstrich_PlayingMovie,
    eGameOstrich_InScene,
    eGameOstrich_Total
} eGameOstrich;

enum eGameMode
{
    eGameMode_Boot,
    eGameMode_Intro,
    eGameMode_Title,
    eGameMode_Start,
    eGameMode_Load,
    eGameMode_Options,
    eGameMode_Save,
    eGameMode_Pause,
    eGameMode_WorldMap,
    eGameMode_MonsterGallery,
    eGameMode_ConceptArtGallery,
    eGameMode_Game,
    eGameMode_Count
};

extern eGameMode gGameMode;

S32 zGameModeGet();
eGameOstrich zGameGetOstrich();

#endif
