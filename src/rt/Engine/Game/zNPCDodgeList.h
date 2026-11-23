#ifndef ZNPCDODGELIST_H
#define ZNPCDODGELIST_H

#include "types.h"

struct xEnt;
struct xVec3;

void zNPCDodgeListInit();
void zNPCDodgeListReset();
U32 zNPCDodgeListGetDodgeDirection(xEnt* ent, xVec3& poi);
S32 zNPCDodgeListAdd(xEnt* inFlyingObject, xEnt* inTarget); // UNUSED
bool zNPCDodgeListRemove(xEnt* outFlyingObject); // UNUSED
void zNPCDodgeListUpdate(F32);

#endif
