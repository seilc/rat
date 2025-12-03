#ifndef ZCOMBATATTACK_H
#define ZCOMBATATTACK_H

#include "xEnt.h"

struct zAnimCacheEntry;
struct zShrapnelAsset;

class zCombatAttack
{
public:
    struct hitBoneInfo
    {
        U16 bone;
        xVec3 boneOffset;
    };

    struct effectBone
    {
        U16 bone;
        zAnimCacheEntry* positionCache;
    };

    struct RumbleEffectParams
    {
        F32 startTime;
        U32 emitterID;
    };

    struct ShrapnelEffectParams
    {
        U32 id;
        zShrapnelAsset* asset;
        F32 startTime;
    };

    struct BlurEffectParams
    {
        F32 start;
        F32 end;
        F32 life;
        F32 alpha;
        F32 fadeInTime;
        F32 fadeOutTime;
    };

    struct FlashEffectParams
    {
        S16 alpha;
        F32 time;
    };

    U32 state;
    F32 attackStart;
    F32 attackEnd;
    F32 attackRadius;
    hitBoneInfo hitBones[4];
    S16 damage;
    U16 source;
    F32 velocityUp;
    F32 velocityAway;
    U32 flags;
    U16 effect;
    U16 hitEffect;
    F32 effectStart;
    F32 effectEnd;
    effectBone effectBones[2];
    RumbleEffectParams rumbleEffect;
    ShrapnelEffectParams shrapnelEffect;
    BlurEffectParams blurEffect;
    FlashEffectParams flashEffect;
    void(*hitCB)(xEnt*, zCombatAttack*, xEnt*, xVec3*, xVec3*);

    zCombatAttack();
};

#endif
