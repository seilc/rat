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

    void Invalidate();
    bool Valid() const;
    bool IsMeshEnabled() const;
    bool UpdatePositionAutoY(const xVec3& newPos, U16 entFilter);
    bool FindAnyValidAutoY(const xVec3& position, U16 entFilter, bool findClosestValid);

    xNavigationMeshAsset* GetMesh() const { return mesh; }

    const xVec3& GetPoint() const { return currentPosition; }
};

#endif
