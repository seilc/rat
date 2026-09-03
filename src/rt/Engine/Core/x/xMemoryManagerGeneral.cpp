#include "xMemoryManagerGeneral.h"

#include "xMemMgr.h"

#include "decomp.h"

#include <stdio.h>

void xMemoryManagerGeneral::Init(void* start, U32 size, Dir dir, Strategy strategy, bool debugging)
{
    xMEMORYMANAGERASSERT(23, size % 16 == 0);

    DoInit(start, size, debugging);

    freeEnd = freeStart = (FreeNode*)(dir == Up ? start : xMEMADVANCE(start, (S32)(size - sizeof(FreeNode))));

    freeStart->next = NULL;
    freeStart->prev = NULL;
    freeStart->size = size - sizeof(FreeNode);

    this->dir = dir;
    this->strategy = strategy;

#if DEBUG || RELEASE
    allocatedElements = NULL;
    freeElements = 1;
    allocatedSpace = 0;
    extraOverheadSpace = 0;
    largestBlock = freeStart->size;
    reallocGrows = 0;
    reallocShrinks = 0;
    reallocMisses = 0;
    reallocNOPs = 0;
    allocatedHeap = NULL;
#endif
}

void* xMemoryManagerGeneral::GetCurrentEnd() const
{
    if (dir == Up) {
        return freeEnd == NULL ? GetArenaEnd() : NextBlockOfMemory(freeEnd);
    } else {
        return freeEnd == NULL ? GetArenaStart() : freeEnd + 1;
    }
}

#if DEBUG || RELEASE
U32 xMemoryManagerGeneral::GetFragmentedSpace() const
{
    U32 ret = 0;
    for (FreeNode* node = freeStart; node != NULL && node->next != NULL; node = node->next) {
        ret += node->size + sizeof(FreeNode);
    }
    return ret;
}

U32 xMemoryManagerGeneral::GetFreeSpace() const
{
    U32 ret = 0;
    for (FreeNode* node = freeStart; node != NULL; node = node->next) {
        ret += node->size;
    }
    return ret;
}

void xMemoryManagerGeneral::DumpAllocatedHeap() const
{
    AllocatedNode* it = allocatedHeap;
    if (it == NULL) {
        printf("heap empty\n");
        return;
    }

    while (it->next) {
        it = it->next;
    }

    S32 count = 0;
    while (it) {
        printf("%5d: %08X, size=%6d\n", count++, it + 1, it->size);
        it = it->prev;
    }
}
#endif

#if DEBUG || RELEASE
void* xMemoryManagerGeneral::DoAllocate(U32 size, U32 options, const char*, const char*, S32)
#else
void* xMemoryManagerGeneral::DoAllocate(U32 size, U32 options)
#endif
{
    size = (size + 15) & ~15;

    xASSERT(127, (options & xMEMORYOPT_ALIGN_MASK) >> xMEMORYOPT_ALIGN_SHIFT <= 4);
    
#if DEBUG || RELEASE
    bool updateLargestBlock = false;
#endif

    FreeNode* node;

    if ((dir == Up && (options & 0x6) != 0x2) || dir == Down && (options & 0x6) != 0x4) {
        if (strategy == BestFit) {
            node = NULL;
            for (FreeNode* search = freeStart; search != NULL; search = search->next) {
                if (search->size >= size && (node == NULL || search->size < node->size)) {
                    node = search;
                }
            }
        } else {
            node = freeStart;
            while (node != NULL) {
                if (node->size >= size) {
                    break;
                }
                node = node->next;
            }
        }

        if (node == NULL) {
            return node;
        }

#if DEBUG || RELEASE
        if (node->size >= largestBlock) {
            updateLargestBlock = true;
        }
#endif

        if (node->size - sizeof(FreeNode) > size) {
            FreeNode* newNode = (FreeNode*)(dir == Up ? (U8*)node + sizeof(FreeNode) + size : (U8*)node - sizeof(FreeNode) - size);
            newNode->next = node->next;
            if (newNode->next) {
                newNode->next->prev = newNode;
            }
            newNode->prev = node;
            node->next = newNode;
            newNode->size = node->size - size - sizeof(FreeNode);
            node->size = size;

            FreeNode* next = NextBlockOfMemory(node);
            xASSERT(185, next == newNode);

            if (node == freeEnd) {
                freeEnd = newNode;
            }

#if DEBUG || RELEASE
            freeElements++;
#endif
        }
    } else {
        if (strategy == BestFit) {
            node = NULL;
            for (FreeNode* search = freeEnd; search != NULL; search = search->prev) {
                if (search->size >= size && (node == NULL || search->size < node->size)) {
                    node = search;
                }
            }
        } else {
            node = freeEnd;
            while (node != NULL) {
                if (node->size >= size) {
                    break;
                }
                node = node->prev;
            }
        }

        if (node == NULL) {
            return node;
        }

#if DEBUG || RELEASE
        if (node->size >= largestBlock) {
            updateLargestBlock = true;
        }
#endif

        if (node->size - sizeof(FreeNode) > size) {
            FreeNode* newNode = (FreeNode*)(dir == Up ?
                (U8*)node + node->size - size :
                (U8*)node - node->size + size);
            newNode->next = node->next;
            if (newNode->next) {
                newNode->next->prev = newNode;
            }
            newNode->prev = node;
            node->next = newNode;
            newNode->size = size;
            node->size -= size + sizeof(FreeNode);

            FreeNode* next = NextBlockOfMemory(node);
            xASSERT(242, next == newNode);

            if (node == freeEnd) {
                freeEnd = newNode;
            }

            FreeNode* swapNode = node;
            node = newNode;

#if DEBUG || RELEASE
            freeElements++;
#endif
        }
    }

    xASSERT(260, node->size >= size);

    if (node->prev) {
        node->prev->next = node->next;
    } else {
        freeStart = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    } else {
        freeEnd = node->prev;
    }

#if DEBUG || RELEASE
    freeElements--;
#endif

    U32 nodeSize = node->size;

#if DEBUG || RELEASE
    xMEMMGRGENERICMALLOCTALLY(nodeSize);
    allocatedElements++;
    allocatedSpace += nodeSize;

    if (updateLargestBlock) {
        UpdateLargestBlock();
    }
#endif

    AllocatedNode* allocatedNode = (AllocatedNode*)(dir == Up ? node : xMEMADVANCE(node, -(S32)nodeSize));
    allocatedNode->size = nodeSize;

#if DEBUG || RELEASE
    allocatedNode->next = allocatedHeap;
    allocatedNode->prev = NULL;

    if (allocatedHeap) {
        allocatedHeap->prev = allocatedNode;
    }

    allocatedHeap = allocatedNode;

    xASSERT(308, nodeSize - size < 65536);

    allocatedNode->extraOverhead = nodeSize - size;
    extraOverheadSpace += allocatedNode->extraOverhead;
    allocatedNode->origSize = 0;
#endif

    return allocatedNode + 1;
}

void xMemoryManagerGeneral::DoFree(void* pointer)
{
    AllocatedNode* allocatedNode = (AllocatedNode*)pointer - 1;
    U32 size = allocatedNode->size;

#if DEBUG || RELEASE
    if (allocatedNode->prev) {
        allocatedNode->prev->next = allocatedNode->next;
    } else {
        allocatedHeap = allocatedHeap->next;
    }

    if (allocatedNode->next) {
        allocatedNode->next->prev = allocatedNode->prev;
    }

    extraOverheadSpace -= allocatedNode->extraOverhead;
#endif

    if (dir == Down) {
        allocatedNode = xMEMADVANCE(allocatedNode, size);
    }

    FreeNode* node = (FreeNode*)allocatedNode;
    node->prev = NULL;
    node->next = NULL;
    node->size = size;

#if DEBUG || RELEASE
    freeElements++;
#endif

    if (freeStart == NULL) {
        freeEnd = node;
        freeStart = node;
    } else {
        FreeNode* insertBefore = freeStart;
        if (dir == Up) {
            while (insertBefore != NULL && node > insertBefore) {
                insertBefore = insertBefore->next;
            }
        } else {
            while (insertBefore != NULL && node < insertBefore) {
                insertBefore = insertBefore->next;
            }
        }

        if (insertBefore) {
            node->prev = insertBefore->prev;
            node->next = insertBefore;
            insertBefore->prev = node;
            if (node->prev == NULL) {
                freeStart = node;
            } else {
                node->prev->next = node;
            }
        } else {
            freeEnd->next = node;
            node->prev = freeEnd;
            freeEnd = node;
        }

        MergeNodes(node);

#if DEBUG || RELEASE
        allocatedElements--;
        allocatedSpace -= size;
        xMEMMGRGENERICMALLOCREMOVE(size);
#endif
    }
}

#if DEBUG || RELEASE
void* xMemoryManagerGeneral::DoReallocate(void* pointer, U32 size, U32 options, const char* file, const char* function, S32 line)
#else
void* xMemoryManagerGeneral::DoReallocate(void* pointer, U32 size, U32 options)
#endif
{
    bool updateLargestBlock = false;

    size = xALIGN(size, 16);

    AllocatedNode* allocatedNode = (AllocatedNode*)pointer - 1;
    U32 nodeSize = allocatedNode->size;

#if DEBUG || RELEASE
    xMEMMGRGENERICMALLOCREMOVE(allocatedNode->size);
    allocatedSpace -= allocatedNode->size;

    U16 originalExtraOverhead = allocatedNode->extraOverhead;
    extraOverheadSpace -= originalExtraOverhead;
    allocatedNode->extraOverhead = 0;
#endif

    if (size < nodeSize && nodeSize - size >= 32) {
        allocatedNode = ShortenNode(allocatedNode, size);
        
#if DEBUG || RELEASE
        reallocShrinks++;
        allocatedSpace += size;
        xMEMMGRGENERICMALLOCTALLY(size);
#endif

        return allocatedNode + 1;
    }

    if (size <= nodeSize) {
#if DEBUG || RELEASE
        reallocNOPs++;
        allocatedSpace += size;
        xMEMMGRGENERICMALLOCTALLY(size);
        allocatedNode->extraOverhead = nodeSize - size;
        xMEMMGRGENERICMALLOCTALLY(allocatedNode->extraOverhead);
        extraOverheadSpace += allocatedNode->extraOverhead;
#endif

        return pointer;
    }

    FreeNode* afterNode = freeStart;
    if (dir == Up) {
        while (afterNode != NULL && (FreeNode*)allocatedNode > afterNode) {
            afterNode = afterNode->next;
        }
    } else {
        while (afterNode != NULL && (FreeNode*)allocatedNode < afterNode) {
            afterNode = afterNode->next;
        }
    }

    FreeNode* beforeNode = afterNode ? afterNode->prev : freeEnd;
    U32 sizeBeforeNode = 0;
    U32 sizeAfterNode = 0;
    if (dir == Up) {
        if (beforeNode && NextBlockOfMemory(beforeNode) == (FreeNode*)allocatedNode) {
            sizeBeforeNode = beforeNode->size;
        }
        if (afterNode && (FreeNode*)xMEMADVANCE(allocatedNode + 1, nodeSize) == afterNode) {
            sizeAfterNode = afterNode->size;
        }
    } else {
        if (beforeNode && (FreeNode*)xMEMADVANCE(allocatedNode + 1, nodeSize) == xMEMADVANCE(beforeNode, -(S32)beforeNode->size)) {
            sizeBeforeNode = beforeNode->size;
        }
        if (afterNode && afterNode + 1 == (FreeNode*)allocatedNode) {
            sizeAfterNode = afterNode->size;
        }
    }

    if (sizeAfterNode != 0 && nodeSize + sizeAfterNode + sizeof(FreeNode) >= size) {
        sizeBeforeNode = 0;
    } else if (sizeBeforeNode != 0 && nodeSize + sizeBeforeNode + sizeof(FreeNode) >= size) {
        sizeAfterNode = 0;
    } else if (nodeSize + sizeAfterNode + sizeBeforeNode + sizeof(FreeNode) < size) {
        sizeAfterNode = sizeBeforeNode = 0;
    }

    if (sizeAfterNode == 0 && sizeBeforeNode == 0) {
#if DEBUG || RELEASE
        reallocMisses++;
        xMEMMGRGENERICMALLOCTALLY(allocatedNode->size);
        allocatedSpace += allocatedNode->size;
        allocatedNode->extraOverhead = originalExtraOverhead;
        extraOverheadSpace += allocatedNode->extraOverhead;
#endif

#if DEBUG || RELEASE
        void* newPointer = DoAllocate(size, options, file, function, line);
#else
        void* newPointer = DoAllocate(size, options);
#endif
        if (newPointer == NULL) {
            return NULL;
        }

        xMemoryCopyUpA128(newPointer, pointer, nodeSize);
        DoFree(pointer);

        return newPointer;
    }

    const void* dataStart = allocatedNode + 1;

    xASSERT(582, size > nodeSize);
    U32 neededSize = size - nodeSize;

    if ((sizeAfterNode != 0 && dir == Up) || (sizeBeforeNode != 0 && dir == Down)) {
        FreeNode* working = (dir == Up ? afterNode : beforeNode);
        if (neededSize < working->size) {
            allocatedNode->size += neededSize;
            if (dir == Up) {
                FreeNode* newNode = xMEMADVANCE(working, neededSize);
                *newNode = *working;
                newNode->size -= neededSize;
                if (newNode->prev) {
                    newNode->prev->next = newNode;
                } else {
                    freeStart = newNode;
                }
                if (newNode->next) {
                    newNode->next->prev = newNode;
                } else {
                    freeEnd = newNode;
                }
            } else {
                working->size -= neededSize;
            }
            neededSize = 0;
        } else {
            allocatedNode->size += working->size + sizeof(AllocatedNode);
            if (working->prev) {
                working->prev->next = working->next;
            } else {
                freeStart = working->next;
            }
            if (working->next) {
                working->next->prev = working->prev;
            } else {
                freeEnd = working->prev;
            }
#if DEBUG || RELEASE
            freeElements--;
#endif
        }
    }

    if (neededSize != 0 && ((sizeBeforeNode != 0 && dir == Up) || (sizeAfterNode != 0 && dir == Down))) {
        FreeNode* working = (dir == Up ? beforeNode : afterNode);
        AllocatedNode* newAllocatedNode;
        U32 newSpace;

        if (neededSize < working->size) {
            working->size -= neededSize;
            newSpace = neededSize;
            newAllocatedNode = xMEMADVANCE(allocatedNode, -(S32)neededSize);
            if (dir == Down) {
                FreeNode* newNode = xMEMADVANCE(working, -(S32)neededSize);
                *newNode = *working;
                if (newNode->next) {
                    newNode->next->prev = newNode;
                } else {
                    freeEnd = newNode;
                }
                if (newNode->prev) {
                    newNode->prev->next = newNode;
                } else {
                    freeStart = newNode;
                }
            }
        } else {
            if (working->prev) {
                working->prev->next = working->next;
            } else {
                freeStart = working->next;
            }
            if (working->next) {
                working->next->prev = working->prev;
            } else {
                freeEnd = working->prev;
            }
            newSpace = working->size + sizeof(AllocatedNode);
            if (dir == Up) {
                newAllocatedNode = (AllocatedNode*)working;
            } else {
                newAllocatedNode = (AllocatedNode*)xMEMADVANCE(working, -(S32)working->size);
            }
#if DEBUG || RELEASE
            freeElements--;
#endif
        }

        *newAllocatedNode = *allocatedNode;
        allocatedNode = newAllocatedNode;
        allocatedNode->size += newSpace;
#if DEBUG || RELEASE
        if (allocatedNode->next) {
            allocatedNode->next->prev = allocatedNode;
        }
        if (allocatedNode->prev) {
            allocatedNode->prev->next = allocatedNode;
        } else {
            allocatedHeap = allocatedNode;
        }
#endif
    }
    
    void* dataDest = allocatedNode + 1;
    if (dataDest < dataStart) {
        xMemoryCopyUpA128(dataDest, dataStart, size);
    } else if (dataDest > dataStart) {
        xMemoryCopyDownA128(dataDest, dataStart, size);
    }

#if DEBUG || RELEASE
    reallocGrows++;
    allocatedSpace += allocatedNode->size;
    xMEMMGRGENERICMALLOCTALLY(allocatedNode->size);
    allocatedNode->extraOverhead = allocatedNode->size - size;
    extraOverheadSpace += allocatedNode->extraOverhead;

    if (updateLargestBlock) {
        UpdateLargestBlock();
    }
#endif

    return allocatedNode + 1;
}

U32 xMemoryManagerGeneral::DoGetBlockSize(void* pointer) const
{
    return ((AllocatedNode*)pointer - 1)->size;
}

#if DEBUG || RELEASE
void xMemoryManagerGeneral::SetOrigSize(void* pointer, U32 size) const
{
    xASSERT(767, pointer);
    xASSERT(768, ((U32)pointer & 3) == 0);
    AllocatedNode& node = *((AllocatedNode*)pointer - 1);
    xASSERT(770, size <= 0xFFFF);
    node.origSize = size;
}

U32 xMemoryManagerGeneral::GetOrigSize(void* pointer) const
{
    AllocatedNode& node = *((AllocatedNode*)pointer - 1);
    return node.origSize;
}
#endif

void xMemoryManagerGeneral::MergeNodes(FreeNode* node)
{
    if (node->prev && NextBlockOfMemory(node->prev) == node) {
        node->prev->size += node->size + sizeof(FreeNode);
        node->prev->next = node->next;
        if (node->next) {
            node->next->prev = node->prev;
        } else {
            freeEnd = node->prev;
        }
        node = node->prev;
#if DEBUG || RELEASE
        freeElements--;
#endif
    }

    if (node->next && NextBlockOfMemory(node) == node->next) {
        node->size += node->next->size + sizeof(FreeNode);
        node->next = node->next->next;
        if (node->next) {
            node->next->prev = node;
        } else {
            freeEnd = node;
        }
#if DEBUG || RELEASE
        freeElements--;
#endif
    }

#if DEBUG || RELEASE
    if (node->size >= largestBlock) {
        largestBlock = node->size;
    }
#endif
}

xMemoryManagerGeneral::AllocatedNode* xMemoryManagerGeneral::ShortenNode(AllocatedNode* allocatedNode, U32 size)
{
    U32 nodeSize = allocatedNode->size;
    FreeNode* newNode;

    if (dir == Up) {
        newNode = (FreeNode*)xMEMADVANCE(allocatedNode + 1, size);
    } else {
        AllocatedNode* oldAllocatedNode = allocatedNode;
        allocatedNode = xMEMADVANCE(allocatedNode, nodeSize - size);
        newNode = (FreeNode*)(allocatedNode - 1);
        xMemoryCopyDownA128(allocatedNode, oldAllocatedNode, size + sizeof(AllocatedNode));

#if DEBUG || RELEASE
        if (allocatedNode->next) {
            allocatedNode->next->prev = allocatedNode;
        }
        if (allocatedNode->prev) {
            allocatedNode->prev->next = allocatedNode;
        } else {
            allocatedHeap = allocatedNode;
        }
#endif
    }
    
    allocatedNode->size = size;
    
    newNode->next = NULL;
    newNode->prev = NULL;
    newNode->size = nodeSize - size - sizeof(FreeNode);

#if DEBUG || RELEASE
    freeElements++;
#endif

    if (freeStart == NULL) {
        freeEnd = newNode;
        freeStart = newNode;
    } else {
        FreeNode* insertBefore = freeStart;
        if (dir == Up) {
            while (insertBefore != NULL && insertBefore < newNode) {
                insertBefore = insertBefore->next;
            }
        } else {
            while (insertBefore != NULL && insertBefore > newNode) {
                insertBefore = insertBefore->next;
            }
        }
        if (insertBefore == NULL) {
            freeEnd->next = newNode;
            newNode->prev = freeEnd;
            freeEnd = newNode;
        } else {
            if (insertBefore->prev == NULL) {
                freeStart = newNode;
            } else {
                insertBefore->prev->next = newNode;
            }
            newNode->prev = insertBefore->prev;
            newNode->next = insertBefore;
            insertBefore->prev = newNode;
        }
    }
    
    MergeNodes(newNode);

#if DEBUG || RELEASE
    if (newNode->size > largestBlock) {
        largestBlock = newNode->size;
    }
#endif

    return allocatedNode;
}

#if DEBUG || RELEASE
void xMemoryManagerGeneral::UpdateLargestBlock()
{
    largestBlock = 0;
    for (FreeNode* largestCheck = freeStart; largestCheck != NULL; largestCheck = largestCheck->next) {
        if (largestCheck->size > largestBlock) {
            largestBlock = largestCheck->size;
        }
    }
}
#endif

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE(
    "node-links failed!\n",
    "nodes-not-ordered\n",
    "uncombined-nodes-present\n",
    "cycles detected\n",
    "totalSize: %8d, allocated: %8d\n"
)
#endif
