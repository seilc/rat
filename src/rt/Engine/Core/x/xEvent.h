#ifndef XEVENT_H
#define XEVENT_H

#include "xBase.h"

enum ForceEvent
{
    FE_YES,
    FE_NO
};

#ifdef DEBUGRELEASE
extern char zEventLogBuf[20][256];
#endif

extern S32 zEventLogBufInit;
extern U32 gThisPlayer;
extern U32 gThisCameraTarget;

void zEntEvent(char* to, U32 toEvent);
void zEntEvent(U32 toID, U32 toEvent);
void zEntEvent(U32 toID, U32 toEvent, F32 toParam0, F32 toParam1, F32 toParam2, F32 toParam3);
void zEntEvent(xBase* from, U32 fromEvent, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID, ForceEvent forceEvent);
void xEventDebugLogDump();

inline void zEntEvent(xBase* to, U32 toEvent)
{
    zEntEvent(NULL, 0, to, toEvent, NULL, NULL, 0, FE_NO);
}

inline void zEntEvent(xBase* to, U32 toEvent, const F32* toParam)
{
    zEntEvent(NULL, 0, to, toEvent, toParam, NULL, 0, FE_NO);
}

inline void zEntEvent(xBase* from, xBase* to, U32 toEvent)
{
    zEntEvent(from, 0, to, toEvent, NULL, NULL, 0, FE_NO);
}

#endif
