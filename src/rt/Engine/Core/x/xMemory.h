#ifndef XMEMORY_H
#define XMEMORY_H

#include "xDebug.h"

template <class T>
static inline T* xMEMADVANCE(T* base, S32 bytes)
{
    return (T*)((U8*)base + bytes);
}

template <class T>
static inline T* xMEMADVANCE(T* base, U32 bytes)
{
    return (T*)((U8*)base + bytes);
}

inline U32 xALIGN(U32 base, U32 align)
{
    return (base + align - 1) & ~(align - 1);
}

inline void xMemorySetV32A32(void* memory, U32 value, U32 count)
{
    xASSERT(141, count % 4 == 0 && (U32)memory % 4 == 0);

    U32* memoryIterator = (U32*)memory;

    count /= 4;
    while (count--) {
        *memoryIterator++ = value;
    }
}

inline void xMemoryCopyUpA32(void* dest, const void* source, U32 count)
{
    xASSERT(245, count % 4 == 0 && (U32)dest % 4 == 0 && (U32)source % 4 == 0);

    U32* destIterator = (U32*)dest;
    const U32* sourceIterator = (const U32*)source;

    count /= 4;
    if (count) {
        do {
            *destIterator++ = *sourceIterator++;
        } while (--count);
    }
}

#endif
