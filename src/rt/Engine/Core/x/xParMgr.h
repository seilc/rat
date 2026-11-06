#ifndef XPARMGR_H
#define XPARMGR_H

#include "types.h"

void xParMgrInit();
void xParMgrIgnoreFXEvents(S32 numFrames); // UNUSED
void xParMgrKillAllParticles();
void xParMgrLevelReset(); // UNUSED
void xParMgrUpdate(F32 elapsedTime);
void xParMgrRender(); // UNUSED

#endif
