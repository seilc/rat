#ifndef XMEMORYMANAGERFIXED_H
#define XMEMORYMANAGERFIXED_H

#include "xMemoryManager.h"

class xMemoryManagerFixed : public xMemoryManager
{
public:
#ifdef DEBUGRELEASE
    void SetOrigSize(void* pointer, U32 size) const;
    U32 GetOrigSize(void* pointer) const;
#endif

    void Init(void* start, U32 elements, U32 elementSize);

#ifdef DEBUGRELEASE
    virtual void* DoAllocate(U32 size, U32 options, const char*, const char*, S32);
#else
    virtual void* DoAllocate(U32 size, U32 options);
#endif

    virtual void DoFree(void* pointer);

#ifdef DEBUGRELEASE
    virtual void* DoReallocate(void* pointer, U32 size, U32, const char*, const char*, S32);
#else
    virtual void* DoReallocate(void* pointer, U32 size, U32);
#endif

    virtual U32 DoGetBlockSize(void*) const;

    void InitMemory();

private:
    struct FixedHeader
    {
        FixedHeader* next;
    };

    FixedHeader* freeList;
    U32 elements;
    U32 elementSize;
#ifdef DEBUGRELEASE
    U32 allocatedElements;
    U8* origSizeList;
#endif
};

#endif
