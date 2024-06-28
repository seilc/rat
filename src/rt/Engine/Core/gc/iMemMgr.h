#ifndef IMEMMGR_H
#define IMEMMGR_H

#include "types.h"

void* iMemPushTemp(U32 size);
void iMemPopTemp(void* pointer);

#endif
