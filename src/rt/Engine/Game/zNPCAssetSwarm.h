#ifndef ZNPCASSETSWARM_H
#define ZNPCASSETSWARM_H

#include "zNPCAsset.h"

struct zNPCAssetSwarm : zNPCAssetRATS
{
    S32 memberNumber;
    F32 spawnRadius;
    U32 attractor;
    U32 repeller;
    U32 flags;
    U32 loopSound;
    U32 oneShotSound;
    F32 oneShotSoundMinTime;
    F32 oneShotSoundMaxTime;
};

#endif
