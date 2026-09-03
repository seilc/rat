#ifndef XMEMORYMANAGERGENERAL_H
#define XMEMORYMANAGERGENERAL_H

#include "xMemoryManager.h"

class xMemoryManagerGeneral : public xMemoryManager
{
public:
    enum Dir
    {
        Up,
        Down
    };

    enum Strategy
    {
        FirstFit,
        BestFit
    };

    xMemoryManagerGeneral();

    void Init(void* start, U32 size, Dir dir, Strategy strategy, bool debugging);
    void* GetCurrentEnd() const;

#if DEBUG || RELEASE
    U32 GetFragmentedSpace() const;
    U32 GetFreeSpace() const;
    void DumpAllocatedHeap() const;

    U32 GetAllocatedElements() const;
    U32 GetAllocatedSpace() const;
    U32 GetFreeElements() const;
    U32 GetOverheadSpace() const;
    U32 GetReallocShrinks() const;
    U32 GetReallocGrows() const;
    U32 GetReallocMisses() const;
    U32 GetReallocNOPs() const;
#endif

protected:
#if DEBUG || RELEASE
    virtual void* DoAllocate(U32 size, U32 options, const char*, const char*, S32);
#else
    virtual void* DoAllocate(U32 size, U32 options);
#endif

    virtual void DoFree(void* pointer);

#if DEBUG || RELEASE
    virtual void* DoReallocate(void* pointer, U32 size, U32 options, const char* file, const char* function, S32 line);
#else
    virtual void* DoReallocate(void* pointer, U32 size, U32 options);
#endif

    virtual U32 DoGetBlockSize(void* pointer) const;

#if DEBUG || RELEASE
public:
    void SetOrigSize(void* pointer, U32 size) const;
    U32 GetOrigSize(void* pointer) const;
#endif

private:
    struct FreeNode
    {
        FreeNode* next;
        FreeNode* prev;
        U32 size;
        U32 pad[1];
    };

    struct AllocatedNode
    {
        U32 size;
#if DEBUG || RELEASE
        AllocatedNode* next;
        AllocatedNode* prev;
        U16 extraOverhead;
        U16 origSize;
#else
        U32 pad[3];
#endif
    };

    FreeNode* freeStart;
    FreeNode* freeEnd;
    Dir dir;
    Strategy strategy;
#if DEBUG || RELEASE
    U32 allocatedElements;
    U32 freeElements;
    U32 allocatedSpace;
    U32 largestBlock;
    U32 extraOverheadSpace;
    U32 reallocShrinks;
    U32 reallocGrows;
    U32 reallocMisses;
    U32 reallocNOPs;
    AllocatedNode* allocatedHeap;
#endif

    void MergeNodes(FreeNode* node);
    AllocatedNode* ShortenNode(AllocatedNode* allocatedNode, U32 size);

#if DEBUG || RELEASE
    void UpdateLargestBlock();
#endif

    FreeNode* NextBlockOfMemory(FreeNode* node) const
    {
        return (FreeNode*)(dir == Up ?
            (U8*)(node + 1) + node->size :
            (U8*)(node - 1) - node->size);
    }
};

#endif
