#ifndef XSNDMGR_H
#define XSNDMGR_H

#include "types.h"

enum xSndHandle
{
};

enum xSndEffect
{
    xSndEffect_NONE,
    xSndEffect_CAVE,
    xSndEffect_MAX_TYPES
};

void xSndMgrUpdate(F32 dt);
void xSndMgrStopSounds(S16 eSoundCategory, bool bSkipPaused, bool bInReset);
void xSndMgrPauseSounds(S16 eSoundCategory, bool bPaused, bool bPauseFutureSoundsOfThisType);
F32 xSndMixerGetOutputVolume(S16 eSoundCategory);

#endif
