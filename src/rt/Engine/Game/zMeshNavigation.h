#ifndef ZMESHNAVIGATION_H
#define ZMESHNAVIGATION_H

#include "xMath2.h"

struct xEnt;
class xNavigationMeshAsset;
class zMeshCircle;
class zMeshPosition;

class zMeshNavigationCommon
{
public:
    static zMeshCircle* AvoidCircles(xEnt* entity, const zMeshPosition* meshPosition, const zMeshCircle& my_circle, const xVec2& dir, xVec2& newDir);
};

U8 zMeshNavigationFindRandomPosition(xNavigationMeshAsset* mesh, const xVec3& shellCenter, F32 innerRadius, F32 outerRadius, xVec3& pointOnMesh, U16 entFilter); // UNUSED
bool zMeshNavigationGetDirection(const zMeshPosition& start, const zMeshPosition& destination, F32 radius, xVec3& direction);

#if DEBUG || RELEASE
void zMeshNavigationAddTweaks();
#endif

#endif
