#ifndef ZENT_H
#define ZENT_H

#include "xEnt.h"

struct zEnt : xEnt
{
    xAnimTable* atbl;
};

void zEntParseModelInfo(xEnt* ent, U32 assetID);
xModelInstance* zEntRecurseModelInfo(void* info, xEnt* ent);
void zEntGetShadowParams(xEnt* ent, xVec3* center, xVec3* radius, F32* strength, xEntShadow::radius_enum rtype);
void zEntGetShadowParams(xEnt* ent, xVec3* center, F32* radius, F32* strength, xEntShadow::radius_enum rtype);

void zEventDebugLog(xBase* from, U32 fromEvent, xBase* to, U32 toEvent, const F32*);

#endif
