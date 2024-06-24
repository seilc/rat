#include "xRMemData.h"

#include "xMemMgr.h"
#include "xDebug.h"

#include "decomp.h"

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

#ifdef DEBUGRELEASE
    #ifdef DEBUG
        DECOMP_FORCEACTIVE(xRMemData_cpp, "%s(%d) : (Unexpected) in '%s'\n");
        DECOMP_FORCEACTIVE(xRMemData_cpp, "%s(%d) : (Don't be shy) in '%s'\n");
    #endif
    DECOMP_FORCEACTIVE(xRMemData_cpp, "!user");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "Already owned - share ok?");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "growuser");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "Need owner");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "!this->ptr");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "Init of grow context while still in use");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "this->user && this->ptr");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "RMemGrow (0x%08X [0x%08X]) base 0x%08X start\n");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "growuser && (growuser == this->user_last)");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "Under New Mgmt?");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "this->ptr_last");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "Resume has bad last info");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "RMemGrow (0x%08X [0x%08X]) base 0x%08X resume\n");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "RMemGrow (0x%08X [0x%08X]) base 0x%08X end\n");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "this->flg_grow & (1<<0)");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "this->flg_grow & RMEM_GROW_ENABLED");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "this->amt==32");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "this->amt==RMEM_GROWSTUB_SIZE");
    DECOMP_FORCEACTIVE(xRMemData_cpp, "this->ptr");
#endif