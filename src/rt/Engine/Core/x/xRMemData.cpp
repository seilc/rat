#include "xRMemData.h"

#include "xMemMgr.h"
#include "xDebug.h"
#include "xOutputMgr.h"

#include <string.h>

#ifdef DEBUG
static U32 g_total_alloc = 0;
#endif

void* RyzMemData::operator new(size_t amt, S32 who, U32 memtag, RyzMemGrow* growCtxt)
{
    void* mem = NULL;

    xVALIDATE(87, who);

    if (!who) {
        who = 'RMEM';
    }

    xASSERTFMT(89, (amt>1), "%s", "Don't be shy");

    S32 dogrow = TRUE;

    if (!growCtxt) {
        dogrow = FALSE;
    } else if (!growCtxt->IsEnabled()) {
        dogrow = FALSE;
    }

    if (dogrow) {
        xASSERTFMT(99, growCtxt->ptr, "%s", "Missing base address");

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

#ifdef DEBUG
    g_total_alloc += amt;
#endif

    return mem;
}

void RyzMemData::operator delete(void*)
{
}

#ifndef NON_MATCHING
void RyzMemGrow::__unused()
{
    xBase* growuser = NULL;
    xVALIDATEMSG(0, 0, "Unexpected");
    xVALIDATEMSG(0, 0, "Don't be shy");
    xASSERTMSG(0, !user, "Already owned - share ok?");
    xASSERTMSG(0, growuser, "Need owner");
    xASSERTMSG(0, !this->ptr, "Init of grow context while still in use");
    xASSERT(0, this->user && this->ptr);
    xOutInfo("RyzMemData", "RMemGrow (0x%08X [0x%08X]) base 0x%08X start\n", 0);
    xASSERTMSG(0, growuser && (growuser == this->user_last), "Under New Mgmt?");
    xASSERTMSG(0, this->ptr_last, "Resume has bad last info");
    xOutInfo("RyzMemData", "RMemGrow (0x%08X [0x%08X]) base 0x%08X resume\n", 0);
    xOutInfo("RyzMemData", "RMemGrow (0x%08X [0x%08X]) base 0x%08X end\n", 0);
    xASSERT(0, this->flg_grow & RMEM_GROW_ENABLED);
    xASSERT(0, this->amt==RMEM_GROWSTUB_SIZE);
    xASSERT(0, this->ptr);
}
#endif
