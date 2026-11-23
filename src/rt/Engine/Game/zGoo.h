#ifndef ZGOO_H
#define ZGOO_H

#include "xEnt.h"

void zGooInit(S32 nobj);
void zGooExit();
S32 zGooNumAllocd(); // UNUSED
S32 zGooNumFree(); // UNUSED
S32 zGooNumUsed(); // UNUSED
S32 zGooAdd(xEnt* obj, F32 depth, S32 freezeGroup);
S32 zGooIs(xEnt* obj, F32& depth, U32 playerCheck);
void zGooCollsBegin(); // UNUSED
void zGooCollsEnd(); // UNUSED
void zGooStopTide(); // UNUSED
void zGooMeltFinished(RpAtomic* atomic);
xEnt* zGooFindEnt(RpAtomic* atomic); // UNUSED

inline S32 zGooIs(xEnt* obj)
{
    F32 dummy;
    return zGooIs(obj, dummy, FALSE);
}

#endif
