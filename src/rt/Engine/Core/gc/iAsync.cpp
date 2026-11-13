#include "xCommon.h"
#include "iAsync.h"

#include "iSystem.h"
#include "xutil.h"

#include <rwcore.h>

RwLinkList asyncDirtyFrameList;
bool asyncThreadDone = true;
OSThread asyncThread;
static U8 ThreadStack[8192];
#if DEBUG || RELEASE
U32 asyncStartFrame;
#endif
OSSemaphore asyncSema;
bool needToLock;

void iAsyncStart(void* (*func)(void*))
{
#if DEBUG || RELEASE
    asyncStartFrame = gFrameCount;
#endif

    rwLinkListInitialize(&asyncDirtyFrameList);
    asyncThreadDone = false;
    OSInitSemaphore(&asyncSema, 1);
    needToLock = true;

    OSCreateThread(&asyncThread, func, NULL, ThreadStack + sizeof(ThreadStack), sizeof(ThreadStack), 10,
                   OS_THREAD_ATTR_DETACH);

    OSReport("THREAD iAsyncStart: func=0x%x stack=0x%x to 0x%x\n", func, ThreadStack,
             ThreadStack + sizeof(ThreadStack));

    OSResumeThread(&asyncThread);
    xASSERT_L(54, ((U32*)ThreadStack)[0] == OS_THREAD_STACK_MAGIC);
    OSCheckActiveThreads();
}

S32 iAsyncIsDone()
{
    xASSERT_L(61, ((U32*)ThreadStack)[0] == OS_THREAD_STACK_MAGIC);
    return asyncThreadDone;
}

void iAsyncTerminate()
{
    OSCancelAlarms(IDTAG('A','S','Y','N'));
    asyncThreadDone = true;
    needToLock = false;

    OSCheckActiveThreads();
}

void iAsyncBeginUpdate()
{
    OSCancelAlarms(IDTAG('A','S','Y','N'));
}

void iAsyncEndUpdate()
{
    iSystemSleep(50, IDTAG('A','S','Y','N'));
}

void iAsyncSuspend()
{
    OSCancelAlarms(IDTAG('A','S','Y','N'));
    if (!asyncThreadDone && !OSIsThreadSuspended(&asyncThread)) {
        OSSuspendThread(&asyncThread);
    }
}

void iAsyncResume()
{
    if (!asyncThreadDone && OSIsThreadSuspended(&asyncThread)) {
        OSResumeThread(&asyncThread);
    }
}

void iAsyncEnd()
{
    OSCancelAlarms(IDTAG('A','S','Y','N'));

    if (asyncThreadDone && OSIsThreadSuspended(&asyncThread)) {
        OSResumeThread(&asyncThread);
    }
}
