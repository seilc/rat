#ifndef ZHEISTMOVEMENTPATH_H
#define ZHEISTMOVEMENTPATH_H

#include "xBase.h"
#include "zHeistMovementPathAsset.h"

#include <rwcore.h>

struct xMovePoint;
class zHeistMovementPath;

class zHeistMovementPathManager
{
protected:
    S32 pathCount;
    zHeistMovementPath** pathArray;
    S32 selectedPathIdx;
};

class zHeistMovementPath : public xBase
{
protected:
    static zHeistMovementPathManager* heistMovementPathManager;

    zHeistMovementPathAsset* pathAsset;
    S32 pathArrayIdx;
    S32 numPoints;
    xMovePoint* startingPoint;
    xMovePoint* endingPoint;
    xMovePoint* gangMemberDetectionPoint;
    xMovePoint* enemyDetectionPoint;
    xMovePoint** movePointChain;
    RwIm3DVertex* vertexArray;
    S32 connectStartPathArrayIdx[3];
    S32 connectEndPathArrayIdx[3];
    U32 pathState;
};

#endif
