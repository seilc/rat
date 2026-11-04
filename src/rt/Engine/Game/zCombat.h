#ifndef ZCOMBAT_H
#define ZCOMBAT_H

#include "xBase.h"
#include "xMath3.h"
#include "zHitParameters.h"

struct zCombatDamageInfo
{
    zCombatDamageInfo() : flags(0) {}
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

#endif
