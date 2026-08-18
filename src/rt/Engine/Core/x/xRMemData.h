#ifndef XRMEMDATA_H
#define XRMEMDATA_H

#include "xBase.h"

#include <stddef.h>

#define RMEM_GROWSTUB_SIZE 32
#define RMEM_GROW_ENABLED (1<<0)

struct RyzMemGrow;

struct RyzMemData
{
    static void* operator new(size_t amt, S32 who, U32 memtag, RyzMemGrow* growCtxt);
    static void operator delete(void*);
};

struct RyzMemGrow
{
    S32 IsEnabled() { return flg_grow & RMEM_GROW_ENABLED; }

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
