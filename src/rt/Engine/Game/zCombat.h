#ifndef ZCOMBAT_H
#define ZCOMBAT_H

#include "xBase.h"
#include "xMath3.h"
#include "zHitParameters.h"

struct zCombatDamageInfo
{
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
