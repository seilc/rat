#ifndef IMEMMGR_H
#define IMEMMGR_H

#include "types.h"

void iMemMgrEnableRenderWareMemoryHack(bool hackEnabled);
void* iMemPushTemp(U32 size);
void iMemPopTemp(void* pointer);

#endif
