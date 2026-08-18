#include "xRMemData.h"

#include "xMemMgr.h"
#include "xDebug.h"
#include "xOutputMgr.h"

#include "decomp.h"

#include <string.h>

#if DEBUG
static U32 g_total_alloc = 0;
#endif

void* RyzMemData::operator new(size_t amt, S32 who, U32 memtag, RyzMemGrow* growCtxt)
{
    void* mem = NULL;

    xVALIDATE(87, who);

    if (!who) {
        who = 'RMEM';
    }

    xASSERTM(89, (amt>1), "%s", "Don't be shy");

    S32 dogrow = TRUE;

    if (!growCtxt) {
        dogrow = FALSE;
    } else if (!growCtxt->IsEnabled()) {
        dogrow = FALSE;
    }

    if (dogrow) {
        xASSERTM(99, growCtxt->ptr, "%s", "Missing base address");

        xOutInfo("RyzMemData",
                 "RMemGrow (0x%08X [0x%08X]) base 0x%08x add (0x%08X) %d\n",
                 growCtxt->user,
                 growCtxt->user ? growCtxt->user->id : 0,
                 growCtxt->ptr,
                 who,
                 amt);
    
        mem = xMEMGROWALLOC(amt, memtag);
    } else {
        xOutInfo("RyzMemData",
                 "RMemAlloc for [0x%08X] size %d\n",
                 who,
                 amt);
        
        mem = xMEMALLOC(amt, 0, memtag, who, 112);
    }

    memset(mem, 0, 4);

#if DEBUG
    g_total_alloc += amt;
#endif

    return mem;
}

void RyzMemData::operator delete(void*)
{
}

DECOMP_FORCEACTIVE(
    DEBUG ? "%s(%d) : (Unexpected) in '%s'\n" : 0,
    DEBUG ? "%s(%d) : (Don't be shy) in '%s'\n" : 0,
    "!user",
    "Already owned - share ok?",
    "growuser",
    "Need owner",
    "!this->ptr",
    "Init of grow context while still in use",
    "this->user && this->ptr",
    "RMemGrow (0x%08X [0x%08X]) base 0x%08X start\n",
    "growuser && (growuser == this->user_last)",
    "Under New Mgmt?",
    "this->ptr_last",
    "Resume has bad last info",
    "RMemGrow (0x%08X [0x%08X]) base 0x%08X resume\n",
    "RMemGrow (0x%08X [0x%08X]) base 0x%08X end\n",
    "this->flg_grow & (1<<0)",
    "this->flg_grow & RMEM_GROW_ENABLED",
    "this->amt==32",
    "this->amt==RMEM_GROWSTUB_SIZE",
    "this->ptr"
)
