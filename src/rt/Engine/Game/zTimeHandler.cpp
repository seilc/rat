#include "xCommon.h"
#include "zTimeHandler.h"

zTimeHandlerMgr timehandler_mgr;

void zTimeHandlerMgr::update(F32 dt)
{
    for (S32 i = get_size_queue() - 1; i >= 0; i--) {
        zTimeHandlerStruct* thandler = fetch_queue_ptr_offset(i);
        xASSERT_L(16, thandler);
        if (thandler) {
            thandler->cb(dt, thandler);
            thandler->elapsed += dt;
            if (thandler->elapsed > thandler->period)
                get_queue(*thandler);
        }
    }
}
