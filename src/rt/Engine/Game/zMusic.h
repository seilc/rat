#ifndef ZMUSIC_H
#define ZMUSIC_H

#include "types.h"

extern F32 gDialogueMusicLevel;

void zMusicInit();
void zMusicUpdate(F32);
void zMusicKill();
void zMusicPause();
void zMusicUnpause();
void zMusicSetVolume(F32 vol, F32 delay);
void zMusicNewSongEvent(const F32* toParam, U32 toParamWidgetID);
void zMusicTempSongEvent(const F32* toParam, U32 toParamWidgetID);
void zMusicTempSongStopEvent();
F32 zMusicGetCurrentVolume(U32); // UNUSED

#endif
