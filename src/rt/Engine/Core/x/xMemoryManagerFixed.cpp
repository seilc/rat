#include "xMemoryManagerFixed.h"

#include "xMemMgr.h"
#include "decomp.h"

#if DEBUG || RELEASE
#define DoAllocate(size, options, file, function, line) DoAllocate(size, options, file, function, line)
#define DoReallocate(pointer, size, options, file, function, line) DoReallocate(pointer, size, options, file, function, line)
#else
#define DoAllocate(size, options, file, function, line) DoAllocate(size, options)
#define DoReallocate(pointer, size, options, file, function, line) DoReallocate(pointer, size, options)
#endif

#if DEBUG || RELEASE
inline U8 ConvertToFixedSizeEnum(U32 size)
{
    if (size <= 4) return 1;
    if (size <= 8) return 2;
    if (size <= 32) return 3;
    if (size <= 48) return 4;
    return 0;
}

void xMemoryManagerFixed::SetOrigSize(void* pointer, U32 size) const
{
    U32 start = (U32)GetArenaStart();
    U32 index = ((U32)pointer - start) / elementSize;
    origSizeList[index] = ConvertToFixedSizeEnum(size);
}

U32 xMemoryManagerFixed::GetOrigSize(void* pointer) const
{
    U32 index = ((U32)pointer - (U32)GetArenaStart()) / elementSize;
    switch (origSizeList[index]) {
    case 1: return 4;
    case 2: return 8;
    case 3: return 32;
    case 4: return 48;
    }
    return 0;
}
#endif

DECOMP_FORCEACTIVE(
    "assert: %d (%s#%d)\n",
    "elementSize > 0 && elementSize >= sizeof(FixedHeader)",
    "xMemoryManagerFixed.cpp"
)

void xMemoryManagerFixed::Init(void* start, U32 elements, U32 elementSize)
{
    xMEMORYMANAGERASSERT(89, elementSize > 0 && elementSize >= sizeof(FixedHeader) && elements > 0);

#if DEBUG || RELEASE
    DoInit(start, elements * (elementSize + 1), false);
#else
    DoInit(start, elements * elementSize, false);
#endif

    this->elements = elements;
    this->elementSize = elementSize;

    InitMemory();
}

void* xMemoryManagerFixed::DoAllocate(U32 size, U32 options, const char*, const char*, S32)
{
    xASSERT(108, (options & xMEMORYOPT_ALIGN_MASK) >> xMEMORYOPT_ALIGN_SHIFT <= 2);

    if (!freeList) return NULL;

    xASSERT(115, size <= elementSize);

    FixedHeader* ret = freeList;

    freeList = freeList->next;

#if DEBUG || RELEASE
    allocatedElements++;

    if (xMemMgrGenericMallocGetTag() < eMemMgrTag_NumTags) {
        xMemMgrGenericMallocTally(elementSize, xMemMgrGenericMallocGetTag());
    }
#endif

    return ret;
}

void xMemoryManagerFixed::DoFree(void* pointer)
{
    S32 TESTVALUE = (S32)((S32)pointer - (S32)GetArenaStart());
    xASSERT(138, TESTVALUE % elementSize == 0);

    FixedHeader* header = (FixedHeader*)pointer;
    header->next = freeList;
    this->freeList = header;

#if DEBUG || RELEASE
    if (xMemMgrGenericMallocGetTag() < eMemMgrTag_NumTags) {
        xMemMgrGenericMallocRemove(elementSize, xMemMgrGenericMallocGetTag());
    }

    allocatedElements--;
#endif
}

void* xMemoryManagerFixed::DoReallocate(void* pointer, U32 size, U32, const char*, const char*, S32)
{
    xASSERT(158, size <= elementSize);
    return pointer;
}

U32 xMemoryManagerFixed::DoGetBlockSize(void*) const
{
    return elementSize;
}

void xMemoryManagerFixed::InitMemory()
{
    freeList = (FixedHeader*)GetArenaStart();

    FixedHeader* cur = (FixedHeader*)freeList;
    for (U32 i = 0; i < elements - 1; i++) {
        cur->next = xMEMADVANCE(cur, elementSize);
        cur = cur->next;
    }
    cur->next = NULL;

#if DEBUG || RELEASE
    allocatedElements = 0;
    origSizeList = (U8*)xMEMADVANCE(cur, elementSize);
#endif
}
