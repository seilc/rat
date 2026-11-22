#ifndef ZHEISTMOVEMENTPATHASSET_H
#define ZHEISTMOVEMENTPATHASSET_H

#include "xDynAsset.h"

struct zHeistMovementPathAsset : xDynAsset
{
    U32 startingPointId;
    U32 endingPointId;
    U32 connectStartPathIds[3];
    U32 connectEndPathIds[3];
    U32 gangMemberDetectionPointId;
    U32 enemyDetectionPointId;
};

#endif
