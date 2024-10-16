#ifndef ZNPC_H
#define ZNPC_H

#include "xBase.h"
#include "xVec3.h"

struct xModelAssetInfo;

struct zStoredSpawn
{
    S32 valid;
    xBase* from;
    xBase* to;
    U32 toEvent;
    F32 toParam[4];
    xBase* toParamWidget;
    U32 toParamWidgetID;
};

namespace zNPC {

class zNPCBaseIN2;
struct zNPCAssetIN2;

class zNPCInstance : public xBase
{
protected:
    struct instance_flags
    {
        bool living : 1;
        bool activated : 1;
        bool visibilityCull : 1;
        U32 pad : 29;
    };

    zNPCBaseIN2* npc_data;
    U32 type;
    zNPCAssetIN2* npc_asset;
    S32 npc_asset_size;
    const xModelAssetInfo* model_asset;
    xVec3 last_pos;
    xVec3 last_orientation;
    bool canAttack;
    bool canDetect;
    bool canChase;
    bool hasNuke;
    bool ignoreAllDamage;
    bool inSpawnState;
    bool noMoreSpawning;
    xVec3 moveRadiusCenter;
    bool hasManualMoveRadiusCenter;
    xVec3 manualMoveRadiusCenter;
    F32 moveRadius2;
    F32 moveInnerRadius2;
    instance_flags flags;

public:
    virtual void create_data(void* mem) = 0;

    zStoredSpawn storedSpawn;
};

}

#endif
