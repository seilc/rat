#ifndef ZNPCCUSTOMAV_H
#define ZNPCCUSTOMAV_H

#include "xBase.h"
#include "xSnd.h"
#include "zNPCCustomAVAsset.h"

struct xMovePointAsset;
struct xVec3;

namespace zNPC {
class zNPCCommon;
}

enum eCustomAVState
{
    eCUSTOMAVSTATE_Waiting,
    eCUSTOMAVSTATE_Playing,
    eCUSTOMAVSTATE_Looping,
    eCUSTOMAVSTATE_Finished
};

class zNPCCustomAV : public xBase
{
public:
    static void Init(xBase& data, xDynAsset& dynasset, size_t asset_size);

    void Reset();
    void Update(zNPC::zNPCCommon* npc, F32 dt);
    bool HasCompleted();
    bool TurnToTarget();
    bool UsePlayerTarget();
    void GetMovePointTargetPos(xVec3* outPos);
    void SetSpawnCustomAV(U32 id); // UNUSED

protected:
    zNPCCustomAVAsset* asset;
    xMovePointAsset* movepointAsset;
    F32 animDelay;
    F32 soundDelay;
    F32 animLoopTime;
    eCustomAVState animState;
    eCustomAVState soundState;
    xSndGroupHandle soundGroupHandle;
    xSndHandle soundHandle;
};

#endif
