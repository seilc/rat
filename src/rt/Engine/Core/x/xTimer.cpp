#include "xTimer.h"

#include "xRand.h"
#include "xserializer.h"
#include "zBase.h"
#include "zEvent.h"

static U32 sPauseTimerHash[] = {
    0xBC345600, 0xBC345609,
    0xBC345683, 0xBC34568C,
    0xBC345706, 0xBC34570F,
    0xBC345789, 0xBC345792,
    0xBC34580C, 0xBC345815,
    0xBC34588F, 0xBC345898,
    0xBC345912, 0xBC34591B,
    0xBC345995, 0xBC34599E,
    0xBC345A18, 0xBC345A21,
    0xBC345A9B, 0xBC345AA4,
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
    if (id == 0xCB3F6340) return TRUE;
    if (id >= 0x016FC9F0 && id <= 0x016FC9F9) return TRUE;

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
    xASSERT(178, t);
    xASSERT(179, t->tasset);

    xBaseReset(t, t->tasset);

    t->state = 0;
    t->secondsLeft = GetRandomizedTime(t->tasset);
    t->flags = 0;
}

void xTimerSave(xTimer* ent, xSerial* s)
{
    xBaseSave(ent, s);

    xASSERT(204, ent);
    xASSERT(205, s);

    s->Write(ent->state);
    s->Write(ent->secondsLeft);
}

void xTimerLoad(xTimer* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    xASSERT(225, ent);
    xASSERT(226, s);

    s->Read(&ent->state);
    s->Read(&ent->secondsLeft);
}

void xTimerEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase*, U32)
{
    xTimer* t = (xTimer*)to;

    xASSERT(250, to);
    xASSERT(251, to->baseType == eBaseTypeTimer);

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
    xASSERT(302, to);
    xASSERT(303, sc);

    xTimer* t = (xTimer*)to;

    xASSERT(307, t->baseType == eBaseTypeTimer);

    if (t->state == 1) {
        t->secondsLeft -= dt;
        if (t->secondsLeft <= 0.0f) {
            zEntEvent(t, t, eEventExpired);
        }
    }
}
