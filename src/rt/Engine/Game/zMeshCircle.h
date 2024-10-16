#ifndef ZMESHCIRCLE_H
#define ZMESHCIRCLE_H

#include "xMath2.h"
#include "xMath3.h"

struct xEnt;
class xNavigationMeshAsset;

class zMeshCircle
{
public:
    zMeshCircle* masterListNext;
    F32 radius;
    F32 radius2;
    xEnt* ownerEntity;
    xVec3 center;
    xVec2 center2;
    xNavigationMeshAsset* mesh;
    U32 npcActiveFlags;
    zMeshCircle* nextNearbyCircle;

    bool IsActive(U32 npcType) const;

    zMeshCircle();

    void SetPosition(const xVec3& position);
    xVec3& GetCenter() const;
};

#endif
