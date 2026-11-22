#ifndef ZHEISTNPCINTERFACEASSET_H
#define ZHEISTNPCINTERFACEASSET_H

#include "xDynAsset.h"

struct zHeistNPCDetectorEnemyInterfaceAsset : xDynAsset
{
    U32 objId;
    U32 startingMovePointId;
    F32 detectionRadius;
    F32 detectionFOV;
    F32 alertRadius;
    F32 alertFOV;
    F32 detectionHeight;
    F32 detectBelowDist;
    F32 speed;
    S32 renderFOVLines;
};

struct zHeistNPCDetectorViewBlockerAsset : xDynAsset
{
    U32 blockerObjectId;
    F32 blockerRadius;
    F32 safeAreaLength;
    F32 safeAreaHeight;
};

struct zHeistGangMemberInterfaceAsset : xDynAsset
{
    U32 gangMemberId;
    U32 startingPathId;
    F32 speed;
};

#endif
