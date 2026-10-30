#ifndef XRMEMDATA_H
#define XRMEMDATA_H

#include "types.h"

#include <stddef.h>

#define RMEM_GROWSTUB_SIZE 32
#define RMEM_GROW_ENABLED (1<<0)

struct xBase;
struct RyzMemGrow;

struct RyzMemData
{
    static void* operator new(size_t amt, S32 who, U32 memtag, RyzMemGrow* growCtxt);
    static void* operator new[](size_t amt, S32 who, U32 memtag, RyzMemGrow* growCtxt);
    static void operator delete(void*);
    static void operator delete[](void*);
    static void operator delete(void*, S32, RyzMemGrow*);
    static void operator delete[](void*, S32, RyzMemGrow*);
#if DEBUG
    static U32 DBG_rmemuse();
#endif
};

struct RyzMemGrow
{
    S32 IsEnabled() { return flg_grow & RMEM_GROW_ENABLED; }
    RyzMemGrow* Init(xBase* growuser, U32 memtag);
    RyzMemGrow* Resume(xBase* growuser);
    void Done();

protected:
    S32 flg_grow;
    S32 amt;
    char* ptr;
    xBase* user;
    S32 amt_last;
    char* ptr_last;
    xBase* user_last;

    friend struct RyzMemData;
};

#endif
