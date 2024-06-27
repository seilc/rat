#ifndef XMEMORY_H
#define XMEMORY_H

#include "types.h"

template <class T>
static inline T* xMEMADVANCE(T* base, U32 bytes)
{
    return (T*)((U8*)base + bytes);
}

#endif
