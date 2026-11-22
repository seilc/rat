#ifndef ZCOMBAT_H
#define ZCOMBAT_H

#include "xBase.h"
#include "xMath3.h"
#include "zHitParameters.h"
#include "xHierarchyBound.h"
#include "zCombatAttack.h"
#include "zCombatEffects.h"
#include "xSkyDome.h"

struct xEnv;
struct xSweptSphere;

struct zCombatDamageInfo
{
    inline zCombatDamageInfo();
    zCombatDamageInfo(xBase* from, S32 damage, zHitSource source, zHitTarget target, const xVec3& knockback, const xVec3& hitLocation, const xVec3& direction, S32 flags)
        : flags(flags), from(from), damage(damage), source(source), target(target), knockback(knockback), hitLocation(hitLocation), direction(direction)
    {
    }

    S32 flags;
    xBase* from;
    S32 damage;
    zHitSource source;
    zHitTarget target;
    xVec3 knockback;
    xVec3 hitLocation;
    xVec3 direction;
};

struct zCombat
{
    struct sphereInfo
    {
        xSphere sphere;
        xSphere previousSphere;
    };

    static const S32 MAX_HIT_OBJECTS = 24;

    xHierarchyBound bounds;
    U32 lastBoundUpdateTime;
    S16 currentHitPoints;
    S16 maximumHitPoints;
    U16 stateTableSize;
    const zCombatAttack* stateTable;
    const xAnimState* animationState;
    const zCombatAttack* runningAttack;
    F32 runningAttackTimer;
    xEnt* lastNPCDamaged;
    U8 disableMovement;
    U8 runningEffect;
    U8 runningBlur;
    U8 hitting;
    U8 hitEnv;
    U8 hitObject;
    U8 forceReset;
    U8 hitObjectCount;
    xEnt* hitObjects[MAX_HIT_OBJECTS];
    S16 currentDamage;
    S16 lastHitDamage;
    zHitSource lastHitSource;
    zHitTarget lastHitTarget;
    sphereInfo location[4];
    S32 (*envCollisionCB)(xEnt*, xSweptSphere*, xEnv*);
    zCombatEffects::PunchEffect punchEffect;
    U32 effectParam;

    S32 Setup(const xHierarchyBoundInitData* initHierarchy, U32 boundsNumber);
    void Init(const xHierarchyBoundInitData* initHierarchy, F32 masterRadius, const zCombatAttack* stateTable,
              U16 stateTableSize, S32 hitPoints, F32 scale, const char* baseName, const char** boneNames, S32 boneCount);
    void PreUpdate(xEnt*, F32);
    void PostUpdate(xEnt* ent, F32 dt);
    void RenderAfterEffect(xEnt*);

#if DEBUG || RELEASE
private:
    xEnt* DEBUG_ent;
    const char** DEBUG_bones;
    S32 DEBUG_boneCount;
#endif
};

void zCombatDamage(xEnt* ent, const zCombatDamageInfo& damageInfo);

inline zCombat* zCombatGetFrom(xEnt* ent)
{
    if (xSkyDome_EntityIsSkyDome(ent)) {
        return NULL;
    }

    return (zCombat*)ent->user_data;
}

#endif
