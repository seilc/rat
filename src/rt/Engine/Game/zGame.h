#ifndef ZGAME_H
#define ZGAME_H

#include "types.h"

extern F32 sTimeElapsed;
extern F32 gRealTimeElapsed;
extern U32 gSoak;
extern U32 gzAsyncRenderDisable;
extern char** soaklevels;
extern S32 enableAudioAnnihilator;

S32 zGameIsPaused();
void zGamePreRender();
void zGameDemoText(F32 fTime);

#endif
