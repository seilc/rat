#ifndef ZNPCCOVERPOINT_H
#define ZNPCCOVERPOINT_H

#include "xBase.h"
#include "xDynAsset.h"
#include "xGroup.h"
#include "xMath3.h"

struct xEnt;

namespace zNPC {
class zNPCCommon;
}

struct zNPCCoverPointAsset : xDynAsset
{
    static const S32 VERSION = 1;

    xVec3 location;
    F32 directionYaw;
    F32 directionPitch;
    F32 directionRoll;
    U32 assetID;
};

class zNPCCoverPoint : public xBase
{
public:
    static void Init(xBase& data, xDynAsset& dynasset, size_t asset_size);

    void Reset(); // UNUSED
    bool Seize(zNPC::zNPCCommon* seizer);
    bool Release(zNPC::zNPCCommon* releaser);

    zNPC::zNPCCommon* IsSeized() { return seizedBy; }

    xVec3* GetCoverLocation() { return &coverPointAsset->location; }
    xPlane* GetCoverPlane() { return &playerCrossPlane; }
    xEnt* GetCoverObject() { return coverObject; }

protected:
    zNPCCoverPointAsset* coverPointAsset;
    xEnt* coverObject;
    xPlane playerCrossPlane;
    zNPC::zNPCCommon* seizedBy;
};

void zNPCCoverManageRetreatToCover(xBase* from, xGroup* npcGroup, U32 toEvent, const F32* toParam, xGroup* coverPointGroup, U32 toParamWidgetID);

#endif
