#include "xTimer.h"

#include "xRand.h"
#include "xserializer.h"
#include "zBase.h"
#include "zEvent.h"

static U32 sPauseTimerHash[] = {
    0xBC345600, 0xBC345609, // xStrHash("PAUSE_TIMER00"), xStrHash("PAUSE_TIMER09")
    0xBC345683, 0xBC34568C, // xStrHash("PAUSE_TIMER10"), xStrHash("PAUSE_TIMER19")
    0xBC345706, 0xBC34570F, // xStrHash("PAUSE_TIMER20"), xStrHash("PAUSE_TIMER29")
    0xBC345789, 0xBC345792, // xStrHash("PAUSE_TIMER30"), xStrHash("PAUSE_TIMER39")
    0xBC34580C, 0xBC345815, // xStrHash("PAUSE_TIMER40"), xStrHash("PAUSE_TIMER49")
    0xBC34588F, 0xBC345898, // xStrHash("PAUSE_TIMER50"), xStrHash("PAUSE_TIMER59")
    0xBC345912, 0xBC34591B, // xStrHash("PAUSE_TIMER60"), xStrHash("PAUSE_TIMER69")
    0xBC345995, 0xBC34599E, // xStrHash("PAUSE_TIMER70"), xStrHash("PAUSE_TIMER79")
    0xBC345A18, 0xBC345A21, // xStrHash("PAUSE_TIMER80"), xStrHash("PAUSE_TIMER89")
    0xBC345A9B, 0xBC345AA4, // xStrHash("PAUSE_TIMER90"), xStrHash("PAUSE_TIMER99")
};

static F32 GetRandomizedTime(xTimerAsset* tasset)
{
    U32 halfRangeMilli = 1000.0f * tasset->randomRange;
    if (halfRangeMilli == 0) {
        return tasset->seconds;
    }

    S32 offset = xrand_RandomInt32() % (halfRangeMilli * 2) - halfRangeMilli;
    F32 time = tasset->seconds + offset / 1000.0f;
    return time;
}

void xTimerInit(void* b, void* tasset)
{
    xTimerInit((xBase*)b, (xTimerAsset*)tasset);
}

static S32 xTimer_ObjIDIsPauseTimer(U32 id)
{
    if (id == 0xCB3F6340) return TRUE; // xStrHash("PAUSE_TIMER")
    if (id >= 0x016FC9F0 && id <= 0x016FC9F9) return TRUE; // xStrHash("PAUSE_TIMER0"), xStrHash("PAUSE_TIMER9")

    S32 foo = (id >= 0xBC345600);
    S32 bar = (id <= 0xBC345AA4);
    if (foo && bar) {
        for (S32 i = 0; i < 10; i++) {
            if (id >= sPauseTimerHash[i*2] && id <= sPauseTimerHash[i*2+1]) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

void xTimerInit(xBase* b, xTimerAsset* tasset)
{
    xBaseInit(b, tasset);

    xTimer* t = (xTimer*)b;

    t->eventFunc = xTimerEventCB;
    t->tasset = tasset;

    if (t->linkCount) {
        t->link = (xLinkAsset*)((U8*)t->tasset + sizeof(xTimerAsset));
    } else {
        t->link = NULL;
    }

    t->state = 0;
    t->secondsLeft = GetRandomizedTime(tasset);
    t->runsInPause = xTimer_ObjIDIsPauseTimer(b->id);
    t->flags = 0;
}

void xTimerReset(xTimer* t)
{
    xASSERT_L(178, t);
    xASSERT_L(179, t->tasset);

    xBaseReset(t, t->tasset);

    t->state = 0;
    t->secondsLeft = GetRandomizedTime(t->tasset);
    t->flags = 0;
}

void xTimerSave(xTimer* ent, xSerial* s)
{
    xBaseSave(ent, s);

    xASSERT_L(204, ent);
    xASSERT_L(205, s);

    s->Write(ent->state);
    s->Write(ent->secondsLeft);
}

void xTimerLoad(xTimer* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    xASSERT_L(225, ent);
    xASSERT_L(226, s);

    s->Read(&ent->state);
    s->Read(&ent->secondsLeft);
}

void xTimerEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase*, U32)
{
    xTimer* t = (xTimer*)to;

    xASSERT_L(250, to);
    xASSERT_L(251, to->baseType == eBaseTypeTimer);

    switch (toEvent) {
    case eEventRun:
        t->state = 1;
        break;
    case eEventStop:
        if (t->state == 1) {
            t->state = 0;
        }
        break;
    case eEventReset:
    case eEventDebugReset:
        xTimerReset(t);
        break;
    case eEventExpired:
        t->state = 0;
        break;
    case eEventTimerSet:
        t->secondsLeft = toParam[0];
        break;
    case eEventTimerAdd:
        t->secondsLeft += toParam[0];
        break;
    }
}

void xTimerUpdate(xBase* to, xScene* sc, F32 dt)
{
    xASSERT_L(302, to);
    xASSERT_L(303, sc);

    xTimer* t = (xTimer*)to;

    xASSERT_L(307, t->baseType == eBaseTypeTimer);

    if (t->state == 1) {
        t->secondsLeft -= dt;
        if (t->secondsLeft <= 0.0f) {
            zEntEvent(t, t, eEventExpired);
        }
    }
}
