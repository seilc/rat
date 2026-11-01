#ifndef XPARMGR_H
#define XPARMGR_H

#include "types.h"

void xParMgrInit();
void xParMgrIgnoreFXEvents(S32 numFrames);
void xParMgrKillAllParticles();
void xParMgrLevelReset();
void xParMgrUpdate(F32 elapsedTime);
void xParMgrRender();

#endif
