#ifndef IMEMMGR_H
#define IMEMMGR_H

#include "types.h"

U32 iMemHeapSize(); // UNUSED
void iMemInit();
bool iMemIsInited(); // UNUSED
U32 iMemSniffStackUsage();
void iMemExit();
#if DEBUG
void iMemSet(void* addr, S32 val, unsigned long size);
void iMemVerify(void* addr, S32 val, unsigned long size);
void iMemMgrMallocSetDebugInfo(const char* file, S32 line, const char* function); // UNUSED
#endif
void iMemMgrEnableRenderWareMemoryHack(bool hackEnabled);
void* iMemMgrMalloc(unsigned long size, U32);
#if DEBUG || RELEASE
void* iMemMgrMallocDebug(U32 size); // UNUSED
#endif
void* iMemMgrRealloc(void* mem, unsigned long newSize, U32);
void iMemMgrFree(void* mem);
void* iMemMgrCalloc(unsigned long numObj, unsigned long sizeObj, U32);
bool iMemMgrCheckCollisionWithMallocHeap(void* mem, U32 size);
bool iMemHasTempAllocated();
void* iMemPushTemp(U32 size);
void iMemPopTemp(void* pointer);
#if DEBUG || RELEASE
void iMemMgrXPrintDebugInfo();
void iMemMgrPrintDebugInfo(); // UNUSED
void iMemMgrDumpActiveList();
#endif
U32 iMemMgrTotalArenaUsage();
S32 iMemMgrGetMinFreeSpace();
#if DEBUG || RELEASE
S32 iMemMgrGetMaxFreeSpace(); // UNUSED
void _iMemMgrSceneStartDebug(); // UNUSED
#endif

#endif
