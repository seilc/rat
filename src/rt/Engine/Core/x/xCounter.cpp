#include "xCounter.h"

#include "xstransvc.h"
#include "xserializer.h"
#include "xDebugTweak.h"
#include "zBase.h"
#include "zEvent.h"
#include "zUICustomLevelManager.h"
#include "xFont.h"
#include "xScene.h"

#include <string.h>

namespace {

#ifdef DEBUGRELEASE
const char* get_asset_name(xCounterAsset* asset)
{
    const char* name = xSTAssetName(asset);
    if (name && *name != '\0') {
        return name;
    }

    static char buffer[16];
    sprintf(buffer, "0x%08X", asset->id);
    return buffer;
}

void on_update_counter_tweak(const tweak_info& info)
{
    xCounter& c = *(xCounter*)info.context;
    const char* msg_st = NULL;

    if (c.state == 0) {
        msg_st = "Idle";
    } else if (c.state == 1) {
        msg_st = "{r=1}Expired{~:r}";
    } else {
        msg_st = "???";
    }

    sprintf(c.stats, "{f=2}{w*.5}{h*.5}%d/%d %s %s",
            c.count,
            c.asset->count,
            msg_st,
            xBaseIsEnabled(&c) ? "Enabled" : "{r=1}Disabled{~:r}");
}

void add_tweaks(xCounter& counter)
{
    char prefix[128];
    sprintf(prefix, "Widgets|Counters|%s", get_asset_name(counter.asset));

    static tweak_callback cb_update = tweak_callback::create_update(on_update_counter_tweak);

    counter.stats[0] = '\0';

    xTWEAKMESSAGE(prefix, counter.stats, &cb_update, &counter, 0x3);
}
#else
void add_tweaks(xCounter& counter)
{
}
#endif

}

void xCounterInit()
{
    xTWEAKREMOVE("Widgets|Counters");
}

void xCounterInit(void* b, void* asset)
{
    xCounterInit((xBase*)b, (xCounterAsset*)asset);
}

void xCounterInit(xBase* b, xCounterAsset* asset)
{
    xCounter* t = (xCounter*)b;

    xBaseInit(b, asset);

    t->eventFunc = xCounterEventCB;
    t->asset = asset;

    if (t->linkCount) {
        t->link = (xLinkAsset*)((U8*)t->asset + sizeof(xCounterAsset));
    } else {
        t->link = NULL;
    }

    t->state = 0;
    t->count = asset->count;

    add_tweaks(*t);
}

void xCounterReset(xBase* b)
{
    xCounter* t = (xCounter*)b;

    xVALIDATE(131, t);
    xVALIDATE(132, t->asset);

    xBaseInit(t, t->asset);

    if (t->linkCount) {
        t->link = (xLinkAsset*)((U8*)t->asset + sizeof(xCounterAsset));
    } else {
        t->link = NULL;
    }

    t->count = t->asset->count;
    t->state = 0;
}

void xCounterSave(xCounter* ent, xSerial* s)
{
    xBaseSave(ent, s);

    xASSERT(162, ent);
    xASSERT(163, s);

    s->Write(ent->state);
    s->Write(ent->count);
}

void xCounterLoad(xCounter* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    xASSERT(184, ent);
    xASSERT(185, s);

    s->Read(&ent->state);
    s->Read(&ent->count);
}

void xCounterEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32)
{
    xCounter* t = (xCounter*)to;

    xASSERT(208, to);
    xASSERT(209, to->baseType == eBaseTypeCounter);

    switch (toEvent) {
    case eEventDecrement:
        t->count--;
        if (t->count <= 0) {
            zEntEvent(t, t, eEventExpired);
        } else if (t->count > 0 && t->count <= 20) {
            zEntEvent(t, t, t->count + (eEventCount1 - 1));
        }
        break;
    case eEventIncrement:
        t->count++;
        if (t->count <= 0) {
            zEntEvent(t, t, eEventExpired);
        } else if (t->count > 0 && t->count <= 20) {
            zEntEvent(t, t, t->count + (eEventCount1 - 1));
        }
        break;
    case eEventReset:
    case eEventDebugReset:
        t->state = 0;
        xASSERT(256, t->asset);
        t->count = t->asset->count;
        break;
    case eEventExpired:
        t->count = 0;
        t->state = 1;
        break;
    case eEventSetCount:
        if (toParamWidget && toParamWidget->baseType == eBaseTypeCounter) {
            xCounter* paramCounter = (xCounter*)toParamWidget;
            t->count = paramCounter->count;
        } else {
            t->count = toParam[0];
        }
        break;
    case eEventSetCounterToCurrentLevel:
        t->count = zUIIncrediblesGetCurrentSceneNumber();
        break;
    case eEventSetCounterToLevelStatus:
    {
        U32 level = ((xCounter*)toParamWidget)->count;
        if (zUIIncrediblesSceneIsCompleted(level)) {
            t->count = 2;
        } else if (zUIIncrediblesSceneIsUnlocked(level)) {
            t->count = 1;
        } else {
            t->count = 0;
        }
        break;
    }
    default:
        if (toEvent >= eEventCount1 && toEvent <= eEventCount20) {
            t->count = toEvent - (eEventCount1 - 1);
        } else if (toEvent == eEventCount0) {
            t->count = 0;
        }
        break;
    }
}

#ifndef NON_MATCHING
void __unused(xScene* sc, xCounter* t, xCounter* counter, char* s, xtextbox::jot& j)
{
    xASSERT(0, sc);
#ifdef DEBUG
    xASSERT(0, t->baseType == eBaseTypeCounter);
#endif
    strncpy(s, "Counter", 0);
    xASSERT(0, counter->baseType == eBaseTypeCounter);
    sprintf(s, "%d");
    sprintf(s, "%.*s");
    j.reset_flags();
}
#endif
