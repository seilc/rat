#ifndef ZMESHCOLLISION_H
#define ZMESHCOLLISION_H

#include "xMath2.h"

class zMeshCircle;
class zMeshPosition;

bool zMeshCollisionSweptCircleEnemies(zMeshPosition& meshPosition, F32 radius, const xVec2& desiredDpos, zMeshCircle* nearbyCircles);
bool zMeshCollisionDepenetrateEnemies(zMeshPosition& meshPosition, F32 radius, zMeshCircle* nearbyCircles, U32 filterBaseType);

#endif
