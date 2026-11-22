#ifndef ZHEISTDISTRACTIONOBJECTASSET_H
#define ZHEISTDISTRACTIONOBJECTASSET_H

#include "zInteractionAsset.h"

struct zHeistDistractionObjectAsset : zTriggerInteractionAsset
{
    U32 enemyToDistractId;
    U32 distractionPointId;
    F32 distractionTime;
    F32 distractionDelay;
};

#endif
