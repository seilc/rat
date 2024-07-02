#ifndef ZENT_H
#define ZENT_H

#include "xEnt.h"

struct zEnt : xEnt
{
    xAnimTable* atbl;
};

void zEventDebugLog(xBase* from, U32 fromEvent, xBase* to, U32 toEvent, const F32*);

#endif
