#include "xCommon.h"
#include "zScript.h"

#include "xDebugTweak.h"
#include "xEvent.h"
#include "xPad.h"
#include "xstransvc.h"
#include "xTRC.h"
#include "zBase.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zHashes.h"
#include "zPlayer.h"
#include "zScene.h"

void zScriptInit(void* b, void* tasset)
{
    zScriptInit((xBase*)b, (xScriptAsset*)tasset);
}

void zScriptInit(xBase* b, xScriptAsset* tasset)
{
    xASSERT_L(49, b);
    xASSERT_L(50, tasset);

    xBaseInit(b, tasset);

    zScript* t = (zScript*)b;
    b->eventFunc = zScriptEventCB;
    t->tasset = tasset;

#if DEBUG || RELEASE
    static U32 checkScriptInitID = HASH_CHKPOINT_RES_SCRIPT_07;
    if (t->id == checkScriptInitID) {
        S32 asdf = 0;
    }
#endif

    if (t->linkCount) {
        t->link = (xLinkAsset*)(t->tasset + 1);
        t->link = (xLinkAsset*)((xScriptEventAsset*)t->link + tasset->eventCount);
    } else {
        t->link = NULL;
    }

    zScriptReset(t);
}

void zScriptReset(zScript* t)
{
    xASSERT_L(85, t);
    xASSERT_L(86, t->tasset);

    xBaseReset(t, t->tasset);

    t->time = 0.0f;
    t->current = (xScriptEventAsset*)(t->tasset + 1);
    t->state = 0;
    t->scaleFactor = t->tasset->scaleFactor;
}

void zScriptSave(zScript* ent, xSerial* s)
{
    xBaseSave(ent, s);

    xASSERT_L(110, ent);
    xASSERT_L(111, s);
}

void zScriptLoad(zScript* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    xASSERT_L(130, ent);
    xASSERT_L(131, s);
}

static bool zScriptExecuteEvents(zScript* t, F32 start, F32 end);

void zScriptEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase*, U32)
{
    zScript* t = (zScript*)to;

    xASSERT_L(153, to);
    xASSERT_L(154, to->baseType == eBaseTypeScript);

    switch (toEvent) {
    default:
        break;
    case eEventRun:
        t->state = 1;
        if (t->time == 0.0f) {
            zScriptExecuteEvents(t, 0.0f, 0.000000001f);

            if (t->state == 1) {
                t->time = 0.000000001f;
            }
        }
        break;
    case eEventStop:
        if (t->state == 1 || t->state == 2) {
            t->state = 0;
        }
        break;
    case eEventTimerSet:
        if (toParam) {
            t->time = toParam[0];
            t->current = (xScriptEventAsset*)(t->tasset + 1);
        }
        break;
    case eEventExpired:
        zScriptReset(t);
        xBaseEnable(t);
        if (t->tasset->loop) {
            t->state = 1;
        }
        break;
    case eEventScriptReset:
        zScriptReset(t);
        break;
    case eEventReset:
    case eEventDebugReset:
        zScriptReset(t);
        break;
    case eEventSetScaleFactor:
        if (toParam) {
            t->scaleFactor = toParam[0];
        }
        break;
    case eEventPause:
        if (t->state == 1) {
            t->state = 2;
        }
        break;
    case eEventUnpause:
        if (t->state == 2) {
            t->state = 1;
        }
        break;
    }
}

static bool zScriptExecuteEvents(zScript* t, F32 start, F32 end)
{
    xASSERT_L(234, t);
    xASSERT_L(235, t->tasset);

    xScriptEventAsset* endEvent = (xScriptEventAsset*)(t->tasset + 1);
    endEvent += t->tasset->eventCount;

#if DEBUG || RELEASE
    static U32 checkScriptID = HASH_CHKPOINT_RES_SCRIPT_07;
    if (t->id == checkScriptID) {
        S32 asdf = 0;
    }
#endif

    F32 lastAbsTime = 0.0f;

    for (; t->current != endEvent; t->current++) {
        if (lastAbsTime > t->current->time) {
            S32 current = t->current - (xScriptEventAsset*)(t->tasset + 1);
            if (current == 0) {
                xFAIL_FMT_L(258, "The script %s is not in order! The first event has a bad time: %f ", xSTAssetNameOrID(t->id), t->current->time);
            } else {
                xFAIL_FMT_L(260, "The script %s is not in order! Event number %i is at time %f where the time for the event before it is %f ", xSTAssetNameOrID(t->id), current, t->current->time, t->current[-1].time);
            }
        }

        lastAbsTime = t->current->time;
        if (t->current->time >= start) {
            if (t->current->time < end) {
                if (t->current->paramEvent != eEventScriptNoop && t->current->enabled) {
                    U32 widget;
                    if (t->current->widget == HASH_THIS_PLYR) {
                        widget = gThisPlayer;
                        xASSERT_FMT_L(279, t->current->time == 0.0f, "Trying to use 'THIS_PLYR' at non time zero");
                    } else {
                        widget = t->current->widget;
                    }

                    xBase* b = zSceneFindObject(widget);
                    if (b) {
                        xBase* paramWidget = NULL;
                        if (t->current->paramWidget) {
                            paramWidget = zSceneFindObject(t->current->paramWidget);
                            if (!paramWidget) {
                                paramWidget = (xBase*)xSTFindAsset(t->current->paramWidget, NULL);
                            }
                        }

                        if (b != t) {
                            zEntEvent(t, b, t->current->paramEvent, t->current->param, paramWidget, t->current->paramWidget);
                        } else {
                            xScriptEventAsset* curr = t->current;
                            t->current++;
                            zEntEvent(t, t, curr->paramEvent, curr->param, paramWidget, curr->paramWidget);
                            return true;
                        }
                    }
                }

                if (t->state == 0) {
                    return true;
                }
            } else {
                return false;
            }
        }
    }

    zEntEvent(t, eEventExpired);
    return true;
}

void zScriptUpdate(xBase* to, xScene* sc, F32 dt)
{
    xASSERT_L(339, to);
    xASSERT_L(340, sc);

    zScript* t = (zScript*)to;
    xASSERT_L(343, t->baseType == eBaseTypeScript);

#if DEBUG || RELEASE
    S32 i = -1; xTRCPadGetPadPort(i, TRC_PadActive);
    xPad* padToCheck = &mPad[i];

    static F32 script_accel = 100.0f;
    xTEMPTWEAKS_BEGIN(prefix)
        xAUTOTWEAK(prefix, "script_accel", &script_accel, 1.0f, 1000000000.0f, NULL, NULL, 0, true);
    xTEMPTWEAKS_END

    if (globals.settings.AccelScripts && (padToCheck->on & 0x100)) {
        if (to == globals.sceneCur->baseList[eBaseTypeScript])
            xprintf("{g=0}AccelScripts = %.2f{~:g}\n", script_accel);
        dt *= script_accel;
    }
#endif

    if (t->state == 1) {
        if (!zScriptExecuteEvents(t, t->time, t->time + dt)) {
            xASSERT_L(380, t->scaleFactor);
            t->time += dt / t->scaleFactor;
        }
    }
}
