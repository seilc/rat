#ifndef ZMINIGAMEPICKUPS_H
#define ZMINIGAMEPICKUPS_H

#include "xMath3.h"

struct xGroup;
struct xModelInstance;

enum
{
    MAX_PICKUP_TYPES = 16
};

struct zMiniGamePickupType
{
    S32 id;
    F32 likelyhood;
    F32 activeTime;
    U32 soundID;
    U32 modelID;
};

struct zMiniGamePickupAvailable
{
    S32 idx;
    xModelInstance* modelInstance;
    xMat4x3 position;
};

class zMiniGamePickupController
{
protected:
    S32 state;
    F32 initialSpawnTime;
    F32 currentSpawnTime;
    F32 initialShowTime;
    F32 currentShowTime;
    xGroup* spawnGroup;
    zMiniGamePickupType type[MAX_PICKUP_TYPES];
    S32 typeCount;
    zMiniGamePickupAvailable available;

public:
    virtual S32 GetRandomType();
    virtual void SetAvailablePickup(S32 idx);
    virtual void Setup(xGroup* inGroup, F32 spawnTime, F32 showTime, bool resetTypes);
    virtual void Update(F32 dt);
    virtual bool Test(xVec3* pos, F32 radius, S32* availableIdx);
    virtual void Reset();
    virtual void Render();
    virtual void AddType(S32 id, F32 likelyhood, F32 activeTime, U32 modelID, U32 soundID);
    virtual F32 GetActiveTime(S32 id) { return type[id].activeTime; }
};

#endif
