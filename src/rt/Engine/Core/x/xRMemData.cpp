#include "xRMemData.h"

#include "xBase.h"
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

    xVALIDATE_L(87, who);

    if (!who) who = 'RMEM';

    xASSERT_MSG_L(89, (amt>1), "Don't be shy");

    S32 dogrow = TRUE;

    if (!growCtxt) dogrow = FALSE;
    else if (!growCtxt->IsEnabled()) dogrow = FALSE;

    if (dogrow) {
        xASSERT_MSG_L(99, growCtxt->ptr, "Missing base address");

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
        
        mem = xMEMALLOC_L(amt, 0, memtag, who, 112);
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
    "RMemGrow (0x%08X [0x%08X]) base 0x%08X start\n"
)

// RESTORED: from BFBB GC, TSSM GC
RyzMemGrow* RyzMemGrow::Resume(xBase* growuser)
{
    xASSERT_MSG(growuser && (growuser == this->user_last), "Under New Mgmt?");
    xASSERT_MSG(this->ptr_last, "Resume has bad last info");

    this->amt = this->amt_last;
    this->ptr = this->ptr_last;
    this->user = this->user_last;
    this->flg_grow = 0x3;

    xOutInfo("RyzMemData",
             "RMemGrow (0x%08X [0x%08X]) base 0x%08X resume\n",
             this->user,
             this->user ? this->user->id : 0,
             this->ptr);

    return this;
}

void RyzMemGrow::Done()
{
    xOutInfo("RyzMemData",
             "RMemGrow (0x%08X [0x%08X]) base 0x%08X end\n",
             this->user,
             this->user ? this->user->id : 0,
             this->ptr);

    xASSERT(this->flg_grow & RMEM_GROW_ENABLED);
    xASSERT(this->amt==RMEM_GROWSTUB_SIZE);
    xASSERT(this->ptr);

    this->amt_last = this->amt;
    this->ptr_last = this->ptr;
    this->user_last = this->user;

    this->amt = 0;
    this->ptr = NULL;
    this->user = NULL;
    this->flg_grow = 0;
}
// END RESTORED
