#include "xCommon.h"
#include "iMemMgr.h"

#include "xMemMgr.h"
#include "xMemoryManagerFixed.h"
#include "xMemoryManagerGeneral.h"
#include "iDebug.h"
#include "iSystem.h"

#include <rwcore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if DEBUG || RELEASE
#define Allocate(size, options, file, function, line) Allocate(size, options, file, function, line)
#define Reallocate(pointer, size, options, file, function, line) Reallocate(pointer, size, options, file, function, line)
#define DoAllocate(size, options, file, function, line) DoAllocate(size, options, file, function, line)
#define DoReallocate(pointer, size, options, file, function, line) DoReallocate(pointer, size, options, file, function, line)
#define HandleOutOfMemory(size, options, file, function, line) HandleOutOfMemory(size, options, file, function, line)
#else
#define Allocate(size, options, file, function, line) Allocate(size, options)
#define Reallocate(pointer, size, options, file, function, line) Reallocate(pointer, size, options)
#define DoAllocate(size, options, file, function, line) DoAllocate(size, options)
#define DoReallocate(pointer, size, options, file, function, line) DoReallocate(pointer, size, options)
#define HandleOutOfMemory(size, options, file, function, line) HandleOutOfMemory(size, options)
#endif

#define MAX_TEMP 32

#define iMEMMGR_SHOULD_WARN(n) ((n) < 2 || ((n) % 10 == 0 && ((n) < 100 || (n) % 100 == 0) && ((n) < 1000 || (n) % 1000 == 0)))
#define iMEMMGR_KB(x) (((x) + 512) / 1024)

extern "C" RwBool _rwResHeapGetStats(void* resHeap, RwUInt32* totalAllocated, RwUInt32* totalFree, RwUInt32* largestAllocated, RwUInt32* largestFree, RwUInt32* smallestAllocated, RwUInt32* smallestFree);

namespace {

struct
{
    void* data;
    U32 size;
} tempStack[MAX_TEMP];

U32 tempCurrent;
#if DEBUG || RELEASE
U32 tempAllocationCount;
#endif
U32 StackBase;
U32 StackSize;

}

inline U32 GetStackPointer()
{ register U32 result; asm { mr result, r1 } return result; }

namespace {

inline void* GetHeapTop()
{
#if RELEASE
    xASSERT_L(64, gActiveHeap < XMEM_MAX_HEAPS);
#endif
    xMemHeap* heap = &gxHeap[gActiveHeap];
#if RELEASE
    xASSERT_L(66, heap->state_idx < XMEMMAXSTACK);
#endif
    xHeapState* sp = &heap->state[heap->state_idx];
#if RELEASE
    xASSERT_L(68, sp);
    xASSERT_L(69, ((U32)sp & 3) == 0);
#endif
    return tempCurrent == 0 ? (void*)sp->curr : xMEMADVANCE(tempStack[tempCurrent - 1].data, tempStack[tempCurrent - 1].size);
}

void iMemClearStack()
{
    U32 sp = GetStackPointer();
    xASSERT_L(DEBUG ? 92 : 96, sp > StackBase);
    xASSERT_L(DEBUG ? 93 : 97, sp < StackBase + StackSize);
    S32 ei = OSDisableInterrupts();
    memset((void*)(StackBase + 4), 0x77, sp - StackBase);
    *(U32*)StackBase = 0xDEADBABE;
    OSRestoreInterrupts(ei);
    OSCheckActiveThreads();
}

class MemoryManagerMulti : public xMemoryManager
{
public:
    MemoryManagerMulti() {}

    void Init(void* start, U32 size, U32, U32 resourceArenaSize, U32 byte4Count, U32 byte8Count, U32 byte32Count, U32 byte48Count, bool debugging);
    void* GetHeapCurrentEnd() const { return heap.GetCurrentEnd(); }

    void* GetHeapArenaStart() const { return heap.GetArenaStart(); }
    void* GetHeapArenaEnd() const { return heap.GetArenaEnd(); }

#if DEBUG || RELEASE
    void xPrintMetrics() const;
    void PrintMetrics() const;
#endif
    S32 GetMinFreeSpace() const;
#if DEBUG || RELEASE
    S32 GetMaxFreeSpace() const; // UNUSED
#endif

protected:
    virtual void* DoAllocate(U32 size, U32 options, const char*, const char*, S32);
    virtual void DoFree(void* pointer);
    virtual void* DoReallocate(void* pointer, U32 size, U32 options, const char*, const char*, S32);
    virtual U32 DoGetBlockSize(void* pointer) const;
    virtual void HandleOutOfMemory(U32 size, U32 options, const char* file, const char* function, S32 line);

private:
    void* resourceArena;
    bool resourceArenaUsed;
    U32 resourceArenaSize;
    xMemoryManagerFixed fixed4;
    xMemoryManagerFixed fixed8;
    xMemoryManagerFixed fixed32;
    xMemoryManagerFixed fixed48;
    xMemoryManagerGeneral heap;
#if DEBUG || RELEASE
    U32 fixed4Warn;
    U32 fixed8Warn;
    U32 fixed32Warn;
    U32 fixed48Warn;
#endif
};

void MemoryManagerMulti::Init(void* start, U32 size, U32, U32 resourceArenaSize, U32 byte4Count, U32 byte8Count, U32 byte32Count, U32 byte48Count, bool debugging)
{
    DoInit(start, size, debugging);

    U8* startByte = (U8*)start;

    resourceArena = xMEMALIGN(startByte, 32);
    resourceArenaUsed = false;
    this->resourceArenaSize = resourceArenaSize + GetDebugDataSize();
    startByte += this->resourceArenaSize;

    fixed4.Init(xMEMALIGN(startByte, 4), byte4Count, 4 + GetDebugDataSize());
    startByte = (U8*)fixed4.GetArenaEnd();

    fixed8.Init(xMEMALIGN(startByte, 8), byte8Count, 8 + GetDebugDataSize());
    startByte = (U8*)fixed8.GetArenaEnd();

    fixed32.Init(xMEMALIGN(startByte, 32), byte32Count, 32 + GetDebugDataSize());
    startByte = (U8*)fixed32.GetArenaEnd();

    fixed48.Init(xMEMALIGN(startByte, 48), byte48Count, 48 + GetDebugDataSize());
    startByte = (U8*)fixed48.GetArenaEnd();

    startByte = xMEMALIGN(startByte, 32);

    size -= startByte - (U8*)start;
    heap.Init(startByte, size - 16, xMemoryManagerGeneral::Down, xMemoryManagerGeneral::FirstFit, false);

#if DEBUG || RELEASE
    fixed4Warn = 0;
    fixed8Warn = 0;
    fixed32Warn = 0;
    fixed48Warn = 0;
#endif
}

void* MemoryManagerMulti::DoAllocate(U32 size, U32 options, const char*, const char*, S32)
{
    if (size == resourceArenaSize && !resourceArenaUsed) {
        resourceArenaUsed = true;
        return resourceArena;
    }

#if DEBUG || RELEASE
    bool warned = false;
#endif

    if (size <= fixed4.GetElementSize()) {
        void* test = fixed4.Allocate(size, options, __FILE__, __FUNCTION__, DEBUG ? 217 : 221);
        if (test) {
#if DEBUG || RELEASE
            fixed4.SetOrigSize(test, size);
#endif
            return test;
        }

#if DEBUG || RELEASE
        fixed4Warn++;
        if (iMEMMGR_SHOULD_WARN(fixed4Warn))
            printf("WARNING - out of 4 byte allocations, consider increasing the amount (%d)\n", fixed4Warn);
        warned = true;
#endif
    }

    if (size <= fixed8.GetElementSize()) {
        void* test = fixed8.Allocate(size, options, __FILE__, __FUNCTION__, DEBUG ? 235 : 239);
        if (test) {
#if DEBUG || RELEASE
            fixed8.SetOrigSize(test, size);
#endif
            return test;
        }

#if DEBUG || RELEASE
        if (!warned) {
            fixed8Warn++;
            if (iMEMMGR_SHOULD_WARN(fixed8Warn))
                printf("WARNING - out of 8 byte allocations, consider increasing the amount (%d)\n", fixed8Warn);
            warned = true;
        }
#endif
    }

    if (size <= fixed32.GetElementSize()) {
        void* test = fixed32.Allocate(size, options, __FILE__, __FUNCTION__, DEBUG ? 256 : 260);
        if (test) {
#if DEBUG || RELEASE
            fixed32.SetOrigSize(test, size);
#endif
            return test;
        }

#if DEBUG || RELEASE
        if (!warned) {
            fixed32Warn++;
            if (iMEMMGR_SHOULD_WARN(fixed32Warn))
                printf("WARNING - out of 32 byte allocations, consider increasing the amount (%d)\n", fixed32Warn);
            warned = true;
        }
#endif
    }

    if (size <= fixed48.GetElementSize()) {
        void* test = fixed48.Allocate(size, options, __FILE__, __FUNCTION__, DEBUG ? 277 : 281);
        if (test) {
#if DEBUG || RELEASE
            fixed48.SetOrigSize(test, size);
#endif
            return test;
        }

#if DEBUG || RELEASE
        if (!warned) {
            fixed48Warn++;
            if (iMEMMGR_SHOULD_WARN(fixed48Warn))
                printf("WARNING - out of 48 byte allocations, consider increasing the amount (%d)\n", fixed48Warn);
            warned = true;
        }
#endif
    }

#if DEBUG || RELEASE
    U32 oldSize = size;
#endif
    if (size <= 48) {
        size = (size + 47) & ~31;
        size -= 16;
    }

    void* raw = heap.Allocate(size, options, __FILE__, __FUNCTION__, DEBUG ? 322 : 326);

#if DEBUG || RELEASE
    if (oldSize <= 48)
        heap.SetOrigSize(raw, oldSize);
    else
        heap.SetOrigSize(raw, 0);
#endif

    if (raw < GetHeapTop()) {
        char buffer[128];

#if DEBUG || RELEASE
        PrintMetrics();
        heap.DumpAllocatedHeap();
#endif

        sprintf(buffer, "Heap overlaps malloc pool\nTop: %08X; Ptr: %08X\nOptions: %08X", GetHeapTop(), raw, options);
        iExceptionMemCrash("MemoryManagerMulti", size, buffer);
        OSReport("### Over by %d bytes (%6.2f KB)\n", (U32)raw - (U32)GetHeapTop(), ((U32)raw - (U32)GetHeapTop()) / 1024.0f);
    }

    return raw;
}

void* MemoryManagerMulti::DoReallocate(void* pointer, U32 size, U32 options, const char*, const char*, S32)
{
    if (fixed4.Owns(pointer) && size <= fixed4.GetElementSize())
        return pointer;
    if (fixed8.Owns(pointer) && size > fixed4.GetElementSize() && size <= fixed8.GetElementSize())
        return pointer;
    if (fixed32.Owns(pointer) && size > fixed8.GetElementSize() && size <= fixed32.GetElementSize())
        return pointer;
    if (fixed48.Owns(pointer) && size > fixed32.GetElementSize() && size <= fixed48.GetElementSize())
        return pointer;

    if (heap.Owns(pointer))
        return heap.Reallocate(pointer, size, options, __FILE__, __FUNCTION__, DEBUG ? 366 : 370);

    return xMemoryManager::DoReallocate(pointer, size, options, __FILE__, __FUNCTION__, DEBUG ? 369 : 373);
}

void MemoryManagerMulti::DoFree(void* pointer)
{
    if (pointer < fixed4.GetArenaStart()) {
        xASSERT_L(DEBUG ? 377 : 381, resourceArenaUsed && pointer == resourceArena);
        resourceArenaUsed = false;
    }
    else if (pointer < fixed4.GetArenaEnd())
        fixed4.Free(pointer);
    else if (pointer < fixed8.GetArenaEnd()) {
#if DEBUG || RELEASE
        U32 origSize = fixed8.GetOrigSize(pointer);
        if (origSize <= fixed4.GetElementSize())
            fixed4Warn--;
#endif

        fixed8.Free(pointer);
    }
    else if (pointer < fixed32.GetArenaEnd()) {
#if DEBUG || RELEASE
        U32 origSize = fixed32.GetOrigSize(pointer);
        if (origSize <= fixed4.GetElementSize())
            fixed4Warn--;
        else if (origSize <= fixed8.GetElementSize())
            fixed8Warn--;
#endif

        fixed32.Free(pointer);
    }
    else if (pointer < fixed48.GetArenaEnd()) {
#if DEBUG || RELEASE
        U32 origSize = fixed48.GetOrigSize(pointer);
        if (origSize <= fixed4.GetElementSize())
            fixed4Warn--;
        else if (origSize <= fixed8.GetElementSize())
            fixed8Warn--;
        else if (origSize <= fixed32.GetElementSize())
            fixed32Warn--;
#endif

        fixed48.Free(pointer);
    } else {
#if DEBUG || RELEASE
        U32 size = heap.GetOrigSize(pointer);
        if (size) {
            if (size <= fixed4.GetElementSize()) {
                xASSERT_L(DEBUG ? 426 : 430, fixed4Warn>0);
                fixed4Warn--;
                heap.SetOrigSize(pointer, 0);
            }
            else if (size <= fixed8.GetElementSize()) {
                xASSERT_L(DEBUG ? 432 : 436, fixed8Warn>0);
                fixed8Warn--;
                heap.SetOrigSize(pointer, 0);
            }
            else if (size <= fixed32.GetElementSize()) {
                xASSERT_L(DEBUG ? 438 : 442, fixed32Warn>0);
                fixed32Warn--;
                heap.SetOrigSize(pointer, 0);
            }
            else if (size <= fixed48.GetElementSize()) {
                xASSERT_L(DEBUG ? 444 : 448, fixed48Warn>0);
                fixed48Warn--;
                heap.SetOrigSize(pointer, 0);
            }
        }
#endif

        heap.Free(pointer);
    }
}

U32 MemoryManagerMulti::DoGetBlockSize(void* pointer) const
{
    if (pointer < fixed4.GetArenaStart()) {
        xASSERT_L(DEBUG ? 459 : 463, resourceArenaUsed);
        return resourceArenaSize;
    }

    if (pointer < fixed4.GetArenaEnd())
        return fixed4.GetBlockSize(pointer);
    if (pointer < fixed8.GetArenaEnd())
        return fixed8.GetBlockSize(pointer);
    if (pointer < fixed32.GetArenaEnd())
        return fixed32.GetBlockSize(pointer);
    if (pointer < fixed48.GetArenaEnd())
        return fixed48.GetBlockSize(pointer);

    return heap.GetBlockSize(pointer);
}

#if DEBUG || RELEASE
void MemoryManagerMulti::xPrintMetrics() const
{
    xMemHeap* hp = &gxHeap[gActiveHeap];
    xHeapState* sp = &hp->state[hp->state_idx];
    U32 minSpace;

    xprintf("HEAP TOP      = %08X\n", sp->curr);
    xprintf(". USED        = %5uk/%5uk\n", iMEMMGR_KB(sp->curr - hp->hard_base), iMEMMGR_KB(hp->size));
    xprintf(". FREE        = %5uk\n", iMEMMGR_KB(hp->size - (sp->curr - hp->hard_base)));
    xprintf("MALLOC BOTTOM = %08X\n", heap.GetCurrentEnd());
    xprintf(". ELMNTS A/F  = %5u/%5u\n", heap.GetAllocatedElements(), heap.GetFreeElements());
    xprintf(". ALLOCATED   = %5uk/%5uk\n", iMEMMGR_KB(heap.GetAllocatedSpace()), iMEMMGR_KB(heap.GetSize()));
    xprintf(". OVERHEAD    = %5uk\n", iMEMMGR_KB(heap.GetOverheadSpace()));
    xprintf(". FRAGMENTATN = %5uk\n", iMEMMGR_KB(heap.GetFragmentedSpace()));
    xprintf(". FREE        = %5uk\n", iMEMMGR_KB(heap.GetFreeSpace()));
    xprintf(". RLC S/G/M/N = %5u/%5u/%5u/%5u\n", heap.GetReallocShrinks(), heap.GetReallocGrows(), heap.GetReallocMisses(), heap.GetReallocNOPs());

    minSpace = GetMinFreeSpace(); xprintf("MIN REM SPACE = %5uk\n", iMEMMGR_KB(minSpace));
    xprintf("MAX REM SPACE = %5uk\n", iMEMMGR_KB(minSpace + heap.GetFragmentedSpace()));

    xprintf("4  BYTE ALLOC = %d / %d\n", fixed4.GetAllocatedElements(), fixed4.GetElements());
    xprintf("8  BYTE ALLOC = %d / %d\n", fixed8.GetAllocatedElements(), fixed8.GetElements());
    xprintf("32 BYTE ALLOC = %d / %d\n", fixed32.GetAllocatedElements(), fixed32.GetElements());
    xprintf("48 BYTE ALLOC = %d / %d\n", fixed48.GetAllocatedElements(), fixed48.GetElements());
    xprintf("RW ARENA      = %5uk\n", iMEMMGR_KB(RwResourcesGetArenaSize()));
    xprintf(". USED        = %5uk, frame=%5uk\n", iMEMMGR_KB(iMemMgrTotalArenaUsage()), iMEMMGR_KB(RwResourcesGetArenaUsage()));
    xprintf("CALLS M/R/F   = %6u/%6u/%6u\n", GetCallsToAllocate(), GetCallsToReallocate(), GetCallsToFree());
    xprintf("TEMP ALLOCS   = %5u (%1u active)\n", tempAllocationCount, tempCurrent);
    xprintf("STACK USAGE   = %5uk\n", iMEMMGR_KB(iMemSniffStackUsage()));
}

void MemoryManagerMulti::PrintMetrics() const
{
    xMemHeap* hp = &gxHeap[gActiveHeap];
    xHeapState* sp = &hp->state[hp->state_idx];

    printf("HEAP TOP      = %08X\n", sp->curr);
    printf(". USED        = %5uk/%5uk\n", iMEMMGR_KB(sp->curr - hp->hard_base), iMEMMGR_KB(hp->size));
    printf(". FREE        = %5uk\n", iMEMMGR_KB(hp->size - (sp->curr - hp->hard_base)));
    printf("MALLOC BOTTOM = %08X\n", heap.GetCurrentEnd());
    printf(". ELMNTS A/F  = %5u/%5u\n", heap.GetAllocatedElements(), heap.GetFreeElements());
    printf(". ALLOCATED   = %5uk%/%5uk\n", iMEMMGR_KB(heap.GetAllocatedSpace()), iMEMMGR_KB(heap.GetSize()));
    printf(". OVERHEAD    = %5uk\n", iMEMMGR_KB(heap.GetOverheadSpace()));
    printf(". FRAGMENTATN = %5uk\n", iMEMMGR_KB(heap.GetFragmentedSpace()));
    printf(". FREE        = %5uk\n", iMEMMGR_KB(heap.GetFreeSpace()));
    printf(". RLC S/G/M/N = %5u/%5u/%5u/%5u\n", heap.GetReallocShrinks(), heap.GetReallocGrows(), heap.GetReallocMisses(), heap.GetReallocNOPs());

#if RELEASE
    U32 minSpace = (U32)heap.GetCurrentEnd() - sp->curr;
    printf("MIN REM SPACE = %5uk\n", iMEMMGR_KB(minSpace));
    printf("MAX REM SPACE = %5uk\n", iMEMMGR_KB(minSpace + heap.GetFragmentedSpace()));
#endif

    printf("4  BYTE ALLOC = %d / %d\n", fixed4.GetAllocatedElements(), fixed4.GetElements());
    printf("8  BYTE ALLOC = %d / %d\n", fixed8.GetAllocatedElements(), fixed8.GetElements());
    printf("32 BYTE ALLOC = %d / %d\n", fixed32.GetAllocatedElements(), fixed32.GetElements());
    printf("48 BYTE ALLOC = %d / %d\n", fixed48.GetAllocatedElements(), fixed48.GetElements());
    printf("RW ARENA      = %5uk\n", iMEMMGR_KB(RwResourcesGetArenaSize()));
    printf(". USED        = %5uk, frame=%5uk\n", iMEMMGR_KB(iMemMgrTotalArenaUsage()), iMEMMGR_KB(RwResourcesGetArenaUsage()));
    printf("CALLS M/R/F   = %6u/%6u/%6u\n", GetCallsToAllocate(), GetCallsToReallocate(), GetCallsToFree());
    printf("TEMP ALLOCS   = %5u (%1u active)\n", tempAllocationCount, tempCurrent);
    printf("STACK USAGE   = %5uk\n", iMEMMGR_KB(iMemSniffStackUsage()));
}
#endif

S32 MemoryManagerMulti::GetMinFreeSpace() const
{
    xMemHeap* hp = &gxHeap[gActiveHeap];
    xHeapState* sp = &hp->state[hp->state_idx];

    return (U32)heap.GetCurrentEnd() - sp->curr;
}

void MemoryManagerMulti::HandleOutOfMemory(U32 size, U32 options, const char* file, const char* function, S32 line)
{
    char optional[256];

#if DEBUG
    sprintf(optional, "%s#%d, func=%s\noptions=%08X", file, line, function, options);
#else
    sprintf(optional, "options=%08X", options);
#endif

    iExceptionMemCrash("xMemoryManager::Allocate", size, optional);

    xFAIL_MSG_L(DEBUG ? 570 : 574, ("Out of memory allocating %d bytes", size));
}

MemoryManagerMulti memoryManager;
#if DEBUG || RELEASE
xMemoryManagerGeneral debugMemoryManager;
#endif

}

static bool memInited;
S32 the_heap;
void* hiheap_start;
void* hiheap_end;
U32 hiheap_size;

extern char _stack_end[];
extern char _stack_addr[];

void iMemInit()
{
    if (memInited) {
        return;
    }

    void* rhs, *heap_end;
    U32 HeapBase, HeapSize;

    new (&memoryManager) MemoryManagerMulti;

    tempCurrent = 0;
#if DEBUG || RELEASE
    tempAllocationCount = 0;
#endif

    hiheap_start = (void*)(((U32)OSGetArenaLo() + 31) & ~31);
    hiheap_end = (void*)((U32)OSGetArenaHi() & ~31);

    heap_end = (void*)((U32)hiheap_start + 150 * 1024);
    rhs = OSInitAlloc(hiheap_start, heap_end, 1);
    the_heap = OSCreateHeap(rhs, heap_end);
    if (the_heap >= 0) {
        OSSetCurrentHeap(the_heap);
    } else {
        exit(-5);
    }

    hiheap_start = (void*)((U32)hiheap_start + 150 * 1024);
    hiheap_size = (U32)hiheap_end - (U32)hiheap_start;

    memoryManager.Init(hiheap_start, hiheap_size, 0, 1024 * 1024, 768, 768, 512, 850, false);

    HeapBase = (U32)memoryManager.GetHeapArenaStart();
    HeapSize = (U32)memoryManager.GetHeapArenaEnd() - (U32)memoryManager.GetHeapArenaStart();

    StackBase = (U32)_stack_end;
    StackSize = (U32)_stack_addr - (U32)_stack_end;

    iMemClearStack();

    gMemInfo.system.addr = 0;
    gMemInfo.system.size = 1024 * 1024;
    gMemInfo.system.flags = 0x20;

    gMemInfo.stack.addr = (U32)_stack_addr;
    gMemInfo.stack.size = (U32)_stack_end - (U32)_stack_addr;
    gMemInfo.stack.flags = 0x820;

    gMemInfo.DRAM.addr = HeapBase;
    gMemInfo.DRAM.size = HeapSize;
    gMemInfo.DRAM.flags = 0x820;

    memInited = true;
}

U32 iMemSniffStackUsage()
{
    U32 min = StackBase, max = StackBase + StackSize - 4, usage;

    U32* p = NULL, *lp;
    do {
        lp = p;
        p = (U32*)((((min + max) >> 1) & ~3) | 0x80000000);

        if (*p == 0x77777777 || *p == 0xDEADBABE) {
            min = (U32)p;
        } else {
            max = (U32)p;
        }
    } while (p != lp);

    usage = StackBase + StackSize - max;
    return usage;
}

void iMemExit()
{
    xASSERT_L(DEBUG ? 759 : 763, gMemInfo.DRAM.addr != NULL);

    free((void*)gMemInfo.DRAM.addr);
    gMemInfo.DRAM.addr = NULL;
}

#if DEBUG
void iMemSet(void* addr, S32 val, unsigned long size)
{
    memset(addr, val, size);
}

void iMemVerify(void* addr, S32 val, unsigned long size)
{
    char* p = (char*)addr;

    xASSERT_L(800, p);
    while (size--) {
        if (*p != val) {
            OSReport("xMemCheck:  guard band overwrite detected at %08X\n", p);
        }

        xASSERT_L(805, *p == val);
        p++;
    }
}

static const char* lastFile = "(renderware-malloc)";
static const char* lastFunction = "(unknown)";
static S32 lastLine;
#endif

static bool sMemMgrEnableRenderWareMemoryHack;

void iMemMgrEnableRenderWareMemoryHack(bool hackEnabled)
{
    sMemMgrEnableRenderWareMemoryHack = hackEnabled;
}

void* iMemMgrMalloc(unsigned long size, U32)
{
    if (size == 200 && sMemMgrEnableRenderWareMemoryHack) {
        return NULL;
    }

    if (size > 48) {
        size = (size + 47) & ~31;
        size -= 16;
    }

#if DEBUG
    void* ret = memoryManager.Allocate(size, 0, lastFile, lastFunction, lastLine);
    lastFile = "(renderware-malloc)";
    lastLine = 0;
    lastFunction = "(unknown)";
#else
    void* ret = memoryManager.Allocate(size, 1, __FILE__, __FUNCTION__, 865);
#endif

    if (size > 48)
        xASSERT_L(DEBUG ? 868 : 872, ((U32)ret & 0x0000001F) == 0);

    return ret;
}

void* iMemMgrRealloc(void* mem, unsigned long newSize, U32)
{
    void* ret;

    if (newSize > 48) {
        newSize = (newSize + 47) & ~31;
        newSize -= 16;
    }

#if DEBUG || RELEASE
    if (debugMemoryManager.Owns(mem)) {
        ret = debugMemoryManager.Reallocate(mem, newSize, 0, __FILE__, __FUNCTION__, DEBUG ? 894 : 898);
        return ret;
    }
#endif

#if DEBUG
    ret = memoryManager.Reallocate(mem, newSize, 0, "(renderware-realloc)", "unknown", 0);
#else
    ret = memoryManager.Reallocate(mem, newSize, 0, __FILE__, __FUNCTION__, 909);
#endif

    if (newSize > 48)
        xASSERT_L(DEBUG ? 913 : 917, ((U32)ret & 0x0000001F) == 0);

    return ret;
}

void iMemMgrFree(void* mem)
{
#if DEBUG || RELEASE
    if (debugMemoryManager.Owns(mem)) {
        debugMemoryManager.Free(mem);
        return;
    }
#endif

    memoryManager.Free(mem);
}

void* iMemMgrCalloc(unsigned long numObj, unsigned long sizeObj, U32)
{
    return memoryManager.Allocate(numObj * sizeObj, 1, __FILE__, __FUNCTION__, DEBUG ? 937 : 941);
}

bool iMemMgrCheckCollisionWithMallocHeap(void* mem, U32 size)
{
    bool ret = xMEMADVANCE(mem, size) >= memoryManager.GetHeapCurrentEnd() && mem < memoryManager.GetArenaEnd();

    if (ret) {
#if DEBUG || RELEASE
        memoryManager.PrintMetrics();
#endif
        return true;
    }

    return false;
}

bool iMemHasTempAllocated()
{
    return tempCurrent != 0;
}

void* iMemPushTemp(U32 size)
{
#if DEBUG || RELEASE
    tempAllocationCount++;
#endif
    size = (size + 15) & ~15;

    if (tempCurrent) {
        if (tempCurrent >= MAX_TEMP) {
#if DEBUG || RELEASE
            iExceptionMemCrash("iMemPushTemp", size, "Out of temporary allocations!\n");
#endif
            iprintf("iMemPushTemp RETURNING NULL: tempCurrent(%d) >= MAX_TEMP(%d)\n", tempCurrent, MAX_TEMP);
            return NULL;
        }

        tempStack[tempCurrent].data = xMEMADVANCE(tempStack[tempCurrent - 1].data, tempStack[tempCurrent - 1].size);
    } else {
        xMemHeap* heap = &gxHeap[gActiveHeap];
        xHeapState* sp = &heap->state[heap->state_idx];
        tempStack[0].data = xMEMALIGN((void*)sp->curr, 16);
    }

    tempStack[tempCurrent].size = size;

    void* end = xMEMADVANCE(tempStack[tempCurrent].data, tempStack[tempCurrent].size);

    if (end > memoryManager.GetHeapCurrentEnd()) {
        iprintf("iMemPushTemp RETURNING NULL: end(%p) > memoryManager.GetHeapCurrentEnd()(%p)\n", end, memoryManager.GetHeapCurrentEnd());
        tempStack[tempCurrent].size = 0;
        return NULL;
    }

    return tempStack[tempCurrent++].data;
}

void iMemPopTemp(void* pointer)
{
    bool found = false;
    for (S32 i = 0; i < tempCurrent; i++)
        if (tempStack[i].data == pointer) {
            tempStack[i].data = NULL;
            tempStack[i].size = 0;
            found = true;

            if (i + 1 == tempCurrent) {
                tempCurrent--;
                while (tempCurrent && !tempStack[tempCurrent - 1].data)
                    tempCurrent--;
            }
        }
    xASSERT_L(DEBUG ? 1025 : 1029, found);
}

#if DEBUG || RELEASE
void iMemMgrXPrintDebugInfo()
{
    memoryManager.xPrintMetrics();
}

void iMemMgrDumpActiveList()
{
    memoryManager.DumpActiveList();
}
#endif

U32 iMemMgrTotalArenaUsage()
{
#if DEBUG
    U32 totalAllocated = 0, totalFree = 0, largestAllocated = 0, largestFree = 0, smallestAllocated = 0, smallestFree = 0;
    if (hackResourceArenaPointer) {
        _rwResHeapGetStats(hackResourceArenaPointer, &totalAllocated, &totalFree, &largestAllocated, &largestFree, &smallestAllocated, &smallestFree);
    }

    return totalAllocated;
#else
    return 0;
#endif
}

S32 iMemMgrGetMinFreeSpace()
{
    return memoryManager.GetMinFreeSpace();
}
