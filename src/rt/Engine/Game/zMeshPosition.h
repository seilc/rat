#ifndef ZMESHPOSITION_H
#define ZMESHPOSITION_H

#include "xVec3.h"

class xNavigationMeshAsset;

class zMeshPosition
{
    xNavigationMeshAsset* mesh;
    S32 currentSubMesh;
    S32 currentTriangle;
    xVec3 currentPosition;

public:
    zMeshPosition();

    bool UpdatePositionAutoY(const xVec3& newPos, U16 entFilter);

    const xVec3& GetPoint() const { return currentPosition; }
};

#endif
