#include "zTimer.h"

#include "xTimer.h"
#include "zScene.h"

static xBase* zTimeDebugModeIteratorCB(xBase* b, zScene* s, void*);

void zTimerDebugModeCB()
{
    zSceneForAllBase(zTimeDebugModeIteratorCB, eBaseTypeTimer, NULL);
}

static xBase* zTimeDebugModeIteratorCB(xBase* b, zScene* s, void*)
{
    xTimer* t = (xTimer*)b;

    xASSERT(67, b);
    xASSERT(68, s);
    xASSERT(69, b->baseType == eBaseTypeTimer);
    
    xprintf("%s %s (sec=%f)\n",
        zSceneGetName(t),
        t->state == 1 ? "Run"
            : t->state == 2 ? "Expired"
            : t->state == 0 ? "Idle"
            : "???",
        t->secondsLeft);
    
    return b;
}
