#include "xParMgr.h"

#include "xPar.h"
#include "iParMgr.h"
#include "xDebug.h"
#include "xOutputMgr.h"

static U32 sFrameCount;
static S32 sParMgrInit;

void xParMgrInit()
{
    xTRACEFUNCTION(62, "PAR");

    if (!sParMgrInit) {
        xParMemInit();
        iParMgrInit();
        sParMgrInit = TRUE;
    } else {
        iprintf("killing all particles\n");
        xParMgrKillAllParticles();
        iParMgrInit();
    }
}

#ifndef NON_MATCHING
void xParMgrIgnoreFXEvents(S32 numFrames)
{
    xASSERT(0, numFrames >= 0);
}
#endif

void xParMgrKillAllParticles()
{
}

void xParMgrUpdate(F32 elapsedTime)
{
    U32 numFrames = 1;

    iParMgrUpdate(elapsedTime);
    numFrames = 1;

    xASSERT(289, numFrames < 100);
    xASSERT(290, sFrameCount < 100);

    sFrameCount += numFrames;

    if (sFrameCount > 10) {
        sFrameCount = 1;
    }

    while (sFrameCount >= 1) {
        sFrameCount--;
    }
}
