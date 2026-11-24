#ifndef ZNPCASSETSWARM_H
#define ZNPCASSETSWARM_H

#include "zNPCAsset.h"

// zNPCAssetSwarm::flags
#define k_ZNPCASSETSWARM_SEEK_CHECK_OBJECT_USAGE 0x1
#define k_ZNPCASSETSWARM_FLEE_CHECK_OBJECT_USAGE 0x2

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
