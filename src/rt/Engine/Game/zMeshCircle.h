#ifndef ZMESHCIRCLE_H
#define ZMESHCIRCLE_H

#include "xMath2.h"
#include "xMath3.h"
#include "zNPCTypes.h"

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

    bool IsActive(U32 npcType) const
    {
        if (npcType >= eNPCType_CMG_PourNSwirl) {
            return false;
        }

        return (npcActiveFlags & (1 << npcType)) != 0;
    }

    zMeshCircle()
    {
        ownerEntity = NULL;
        Reset();
    }

    void Reset()
    {
        mesh = NULL;
        masterListNext = NULL;
        nextNearbyCircle = NULL;
        npcActiveFlags = 0xFFFF;
    }

    void SetPosition(const xVec3& position)
    {
        center = position;
        center2.assign(position.x, position.z);
    }

    const xVec3& GetCenter() const
    {
        return center;
    }
};

#endif
