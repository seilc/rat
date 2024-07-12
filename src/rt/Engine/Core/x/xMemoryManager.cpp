#include "xMemoryManager.h"

#include <stdio.h>

#ifdef DEBUGRELEASE
#define Allocate(size, options, file, function, line) Allocate(size, options, file, function, line)
#define Reallocate(pointer, size, options, file, function, line) Reallocate(pointer, size, options, file, function, line)
#define DoAllocate(size, options, file, function, line) DoAllocate(size, options, file, function, line)
#define DoReallocate(pointer, size, options, file, function, line) DoReallocate(pointer, size, options, file, function, line)
#define HandleOutOfMemory(size, options, file, function, line) HandleOutOfMemory(size, options, file, function, line)
#define _SetupDebugBlock(memory, size, file, function, line) SetupDebugBlock(memory, size, file, function, line)
#else
#define Allocate(size, options, file, function, line) Allocate(size, options)
#define Reallocate(pointer, size, options, file, function, line) Reallocate(pointer, size, options)
#define DoAllocate(size, options, file, function, line) DoAllocate(size, options)
#define DoReallocate(pointer, size, options, file, function, line) DoReallocate(pointer, size, options)
#define HandleOutOfMemory(size, options, file, function, line) HandleOutOfMemory(size, options)
#define _SetupDebugBlock(memory, size, file, function, line) SetupDebugBlock(memory, size, NULL, NULL, 0)
#endif

namespace {
    S32 zeroMemoryAddress = 0;

    U32 MEM_CLEAR_VALUE = 0xD00D00D5;
    U32 MEM_DEALLOC_VALUE = 0x1A13D00D;
}

void* xMemoryManager::Allocate(U32 size, U32 options, const char* file, const char* function, S32 line)
{
#ifdef DEBUGRELEASE
    callsToAllocate++;
#endif

    if (size == 0) {
        return &zeroMemoryAddress;
    }

    size = xALIGN(size, 4);

    void* memory = DoAllocate(size + debugDataSize, options, file, function, line);
    if (!memory) {
        HandleOutOfMemory(size, options, file, function, line);
        return NULL;
    }

    if (IsDebugging()) {
        memory = _SetupDebugBlock(memory, size, file, function, line);
        if (!(options & 0x1)) {
            xMemorySetV32A32(memory, MEM_CLEAR_VALUE, size);
        }
    }

    if (options & 0x1) {
        xMemorySetV32A32(memory, 0, size);
    }

    return memory;
}

void xMemoryManager::Free(void* pointer)
{
#ifdef DEBUGRELEASE
    callsToFree++;
#endif

    if (!pointer || pointer == &zeroMemoryAddress) return;

    xASSERT(81, Owns(pointer));

    if (IsDebugging()) {
        xASSERT(86, IsValidPointer(pointer));

        U32 size;
        pointer = RemoveDebugBlock(pointer, &size);
        xMemorySetV32A32(pointer, MEM_DEALLOC_VALUE, size);
    }

    DoFree(pointer);
}

void* xMemoryManager::Reallocate(void* pointer, U32 size, U32 options, const char* file, const char* function, S32 line)
{
    if (!pointer) {
        return Allocate(size, options, file, function, line);
    }

#ifdef DEBUGRELEASE
    callsToReallocate++;
#endif

    if (size == 0) {
        if (pointer != &zeroMemoryAddress) {
            Free(pointer);
        }
        return &zeroMemoryAddress;
    }

    if (IsDebugging()) {
        pointer = RemoveDebugBlock(pointer, NULL);
    }

    size = xALIGN(size, 4);

    void* newBlock = DoReallocate(pointer, size + debugDataSize, options, file, function, line);
    if (!newBlock) {
        HandleOutOfMemory(size, options, file, function, line);
        return NULL;
    }

    if (IsDebugging()) {
        newBlock = _SetupDebugBlock(newBlock, size, file, function, line);
    }

    return newBlock;
}

U32 xMemoryManager::GetBlockSize(void* pointer) const
{
    xASSERT(138, Owns(pointer));

    if (pointer == &zeroMemoryAddress) {
        return 0;
    }

    return DoGetBlockSize(pointer) - debugDataSize;
}

#ifndef NON_MATCHING
bool xMemoryManager::InActiveList(const void*, U32) const
{
    xMEMADVANCE(activeList, (U32)0);
    return false;
}
#endif

bool xMemoryManager::IsValidPointer(const void* pointer) const
{
    DebugAllocationHeader* list = activeList;
    while (list) {
        if (xMEMADVANCE(pointer, -(S32)sizeof(DebugAllocationHeader)) == list) {
            return true;
        }
        list = list->next;
    }
    return false;
}

void xMemoryManager::DumpActiveList() const
{
    DebugAllocationHeader* list = activeList;
    S32 count = 0;
    while (list) {
        printf("%4d: %6d bytes at %08X %s#%d (%s)\n",
               count,
               list->size,
               (U32)(list+1),
               list->file,
               list->line,
               list->function);
        list = list->next;
        count++;
    }
    printf("total: %d\n", count);
}

#ifndef NON_MATCHING
bool xMemoryManager::ValidateActiveList() const
{
    S32 first = 0, second = 0, i = 0;
    DebugAllocationHeader* header = NULL;
    DebugAllocationTrailer* trailer = NULL;
    xASSERT(0, first != second);
    xASSERT(0, Owns( header ));
    xASSERT(0, header->magic[ i ] == header->MAGIC);
    xASSERT(0, trailer->magic[ i ] == trailer->MAGIC);
    return false;
}
#endif

void xMemoryManager::DoInit(void* start, U32 size, bool debugging)
{
    arenaStart = start;
    arenaEnd = (void*)((U32)start + size);
    this->size = size;
#ifdef DEBUGRELEASE
    callsToAllocate = 0;
    callsToReallocate = 0;
    callsToFree = 0;
    debugOverhead = 0;
#endif
    debugDataSize = debugging ? (sizeof(DebugAllocationHeader) + sizeof(DebugAllocationTrailer)) : 0;
    activeList = NULL;
}

void xMemoryManager::HandleOutOfMemory(U32 size, U32 options, const char* file, const char* function, S32 line)
{
}

void* xMemoryManager::DoReallocate(void* pointer, U32 size, U32 options, const char* file, const char* function, S32 line)
{
    U32 copySize = DoGetBlockSize(pointer);
    if (copySize > size) copySize = size;

    void* newBlock = DoAllocate(size, options, file, function, line);
    if (!newBlock) {
        return NULL;
    }

    xMemoryCopyUpA32(newBlock, pointer, copySize);
    DoFree(pointer);

    return newBlock;
}

void* xMemoryManager::SetupDebugBlock(void* memory, U32 size, const char* file, const char* function, S32 line)
{
    DebugAllocationHeader* header = (DebugAllocationHeader*)memory;
    xASSERT(260, !IsValidPointer(header+1));

    DebugAllocationTrailer* trailer = (DebugAllocationTrailer*)xMEMADVANCE(header+1, size);

    header->file = file;
    header->function = function;
    header->line = line;
    header->size = size;
    header->next = activeList;
    header->prev = NULL;
    header->manager = this;

    if (activeList) {
        activeList->prev = header;
    }

    activeList = header;

    for (S32 i = 0; i < header->NUM_MAGIC; i++) {
        header->magic[i] = header->MAGIC;
    }
    
    for (S32 i = 0; i < trailer->NUM_MAGIC; i++) {
        trailer->magic[i] = trailer->MAGIC;
    }

#ifdef DEBUGRELEASE
    debugOverhead += debugDataSize;
#endif

    return (void*)(header+1);
}

void* xMemoryManager::RemoveDebugBlock(void* memory, U32* size)
{
    // Equivalent (regalloc)

    xASSERT(292, IsValidPointer( memory ));

    DebugAllocationHeader* header = (DebugAllocationHeader*)((U32)memory - sizeof(DebugAllocationHeader));
    DebugAllocationTrailer* trailer = (DebugAllocationTrailer*)xMEMADVANCE(memory, header->size);

    xASSERT(299, header->manager == this);

    for (S32 i = 0; i < header->NUM_MAGIC; i++) {
        xASSERT(303, header->magic[i] == header->MAGIC);
    }
    
    for (S32 i = 0; i < trailer->NUM_MAGIC; i++) {
        xASSERT(305, trailer->magic[i] == trailer->MAGIC);
    }

    if (header->prev) {
        header->prev->next = header->next;
    } else {
        activeList = header->next;
    }

    if (header->next) {
        header->next->prev = header->prev;
    }

#ifdef DEBUGRELEASE
    debugOverhead -= debugDataSize;
#endif

    if (size) {
        *size = header->size + debugDataSize;
    }

    return (void*)header;
}
