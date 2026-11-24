#ifndef ZNPCCUSTOMAVASSET_H
#define ZNPCCUSTOMAVASSET_H

#include "xDynAsset.h"

struct zNPCCustomAVAsset : xDynAsset
{
    static const S32 VERSION = 4;

    U32 animID;
    F32 animDelay;
    U32 soundGroupID;
    F32 soundDelay;
    bool waitOnSound;
    F32 loopTime;
    U32 faceTargetID;
    F32 animBlendTime;
    F32 speedMin;
    F32 speedMax;
    bool randomStartTime;
};

#endif
