#ifndef XMEMORYMANAGER_H
#define XMEMORYMANAGER_H

#include "types.h"

#include <dolphin.h>

#ifdef DEBUGRELEASE
#define xMEMORYMANAGERASSERT(line, cond)                                                          \
do {                                                                                              \
    if (!(cond)) {                                                                                \
        OSReport("assert: %d (%s#%d)\n", #cond, __FILE__, (line));                                \
        while (1);                                                                                \
    }                                                                                             \
} while (0)
#else
#define xMEMORYMANAGERASSERT(line, cond)
#endif

static const U32 xMEMORYOPT_ALIGN_MASK = 0x3C;
static const U32 xMEMORYOPT_ALIGN_SHIFT = 2;

class xMemoryManager
{
public:
    void DoInit(void* start, U32 size, bool debugging);
    void* GetArenaStart() const;

#ifdef DEBUGRELEASE
    virtual void* DoAllocate(U32 size, U32 options, const char*, const char*, S32) = 0;
#else
    virtual void* DoAllocate(U32 size, U32 options) = 0;
#endif

    virtual void DoFree(void* pointer) = 0;

#ifdef DEBUGRELEASE
    virtual void* DoReallocate(void* pointer, U32 size, U32 options, const char* file, const char* function, S32 line);
#else
    virtual void* DoReallocate(void* pointer, U32 size, U32 options);
#endif

    virtual U32 DoGetBlockSize(void* pointer) const = 0;

#ifdef DEBUGRELEASE
    virtual void HandleOutOfMemory(U32 size, U32 options, const char* file, const char* function, S32 line);
#else
    virtual void HandleOutOfMemory(U32 size, U32 options);
#endif

private:
    struct DebugAllocationHeader
    {
        const char* file;
        S32 line;
        const char* function;
        U32 size;
        DebugAllocationHeader* prev;
        DebugAllocationHeader* next;
        xMemoryManager* manager;
        U32 magic[1];
    };

    void* arenaStart;
    void* arenaEnd;
    U32 size;
#ifdef DEBUGRELEASE
    U32 callsToAllocate;
    U32 callsToReallocate;
    U32 callsToFree;
#endif
    const char* lastFile;
    const char* lastFunction;
    S32 lastLine;
    U32 debugDataSize;
    DebugAllocationHeader* activeList;
#ifdef DEBUGRELEASE
    U32 debugOverhead;
#endif
};

#endif
