#ifndef ZGAME_H
#define ZGAME_H

#include "types.h"

extern F32 gRealTimeElapsed;
extern U32 gSoak;
extern U32 gzAsyncRenderDisable;
extern char** soaklevels;

S32 zGameIsPaused();
void zGamePreRender();

#endif
