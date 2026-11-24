#ifndef ZNPC_H
#define ZNPC_H

#include "xBase.h"
#include "xVec3.h"
#include "zNPCAssetIN2.h"

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
    void set_data(zNPCBaseIN2* data);
    zNPCInstance(const xModelAssetInfo* model_asset, U32 type, zNPCAssetIN2* npc_asset, S32 npc_asset_size);
    void Reset();
    void SetMovementRadius(F32 inRadius);
    void SetManualMovementRadiusCenter(xVec3* inCenter);
    void ExitingSpawnState();
    F32 get_update_dist2() const;
    F32 get_surrender_dist2() const;
    bool StoreSpawnInfo(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID); // UNUSED
    void RespawnFromStoredInfo(); // UNUSED
    bool ShouldHaveData();

    virtual void create_data(void* mem) = 0;

    zStoredSpawn storedSpawn;

    void reset_last_frame()
    {
        xVec3Copy(&last_pos, &npc_asset->ent_asset.pos);
        xVec3Copy(&last_orientation, &npc_asset->ent_asset.ang);
    }

    zNPCBaseIN2* get_data() { return npc_data; }

    zNPCBaseIN2* surrender_data()
    {
        zNPCBaseIN2* old_data = npc_data;
        npc_data = NULL;
        return old_data;
    }

    void SetSpawnState(bool inState) { inSpawnState = inState; }

    instance_flags get_flags() const { return flags; }
    U32 get_type() const { return type; }

    zNPCAssetIN2* get_npc_asset() const
    {
        return npc_asset;
    }

    S32 get_npc_asset_size() const
    {
        return npc_asset_size;
    }
};

}

#endif
