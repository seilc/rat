#ifndef ZDESTRUCTIBLE_H
#define ZDESTRUCTIBLE_H

#include "iSnd.h"

struct xEnt;
struct xModelInstance;
struct xDestructibleAsset;
struct zDestructibleState;
struct zDestructible;

struct zDestructible
{
    xDestructibleAsset* asset;
    U32 id;
    U32 cur_hit_pts;
    U32 curstateidx;
    U32 nstates;
    U32 hit_pts;
    U32 hit_filter;
    U32 hit_filter_excluded;
    U32 healthPoints;
    U32 expPoints;
    F32 healthChance;
    F32 expChance;
    U32 launch_flag;
    U32 behaviour;
    U32 base_modelID;
    iSndGroupHandle sg_handle_idle;
    xSndHandle sh_handle_idle;
    xModelInstance* base_model;
    xModelInstance* base_collision;
    zDestructibleState* dstates;
    F32 hit_to_destroy_timer;
    F32 timer;
    F32 respawn_time;
    void(*destroy_notify)(zDestructible*, void*);
    void* notify_context;
    xEnt* root_ent;
    U32 flags;
    S32 owner;
    bool has_animation;
    bool active;
    xModelInstance** baseChildrenList;
    S32 baseChildrenCount;
};

zDestructible* zDestructibleGetFromEntity(xEnt* ent);

#endif
