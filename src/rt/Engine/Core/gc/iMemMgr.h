#ifndef IMEMMGR_H
#define IMEMMGR_H

#include "types.h"

extern S32 the_heap;

void iMemInit();
void iMemExit();
void iMemSet(void* addr, S32 val, unsigned long size);
void iMemVerify(void* addr, S32 val, unsigned long size);
bool iMemMgrCheckCollisionWithMallocHeap(void* mem, U32 size);
bool iMemHasTempAllocated();
void iMemMgrEnableRenderWareMemoryHack(bool hackEnabled);
void* iMemPushTemp(U32 size);
void iMemPopTemp(void* pointer);
void iMemMgrXPrintDebugInfo();
void iMemMgrDumpActiveList();
S32 iMemMgrGetMinFreeSpace();
U32 iMemSniffStackUsage();

#endif
