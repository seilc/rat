#include "xCommon.h"
#include "zProgressScript.h"

#include "xMemMgr.h"
#include "zBase.h"
#include "zEvent.h"

void zProgressScript_Init(void* b, void* sasset)
{
    zProgressScript_Init((xBase*)b, (xProgressScriptAsset*)sasset);
}

void zProgressScript_Init(xBase* b, xProgressScriptAsset* sasset)
{
    xASSERT_L(43, b);
    xASSERT_L(44, sasset);

    xBaseInit(b, sasset);

    zProgressScript* s = (zProgressScript*)b;

    s->eventFunc = zProgressScript_EventCB;
    s->sasset = sasset;

    if (s->linkCount) {
        s->link = (xLinkAsset*)(s->sasset + 1);
        s->link = (xLinkAsset*)((xProgressScriptEventAsset*)s->link + sasset->eventCount);
    } else {
        s->link = NULL;
    }

    s->events = (xProgressScriptEventAsset*)(s->sasset + 1);

    s->numFlagWords = (sasset->eventCount >> 5) + 1;
    s->flags = (U32*)xMEMALLOC_L(s->numFlagWords * sizeof(U32), 0, eMemMgrTag_Event, sasset->id, 73);

    zProgressScript_Reset(s);
}

void zProgressScript_Reset(zProgressScript* s)
{
    xASSERT_L(80, s);
    xASSERT_L(81, s->sasset);

    xBaseReset(s, s->sasset);

    s->percent = 0.0f;
    s->current = 0;

    for (S32 i = 0; i < s->numFlagWords; i++) {
        s->flags[i] = 0;
    }
}

void zProgressScript_Save(zProgressScript* ent, xSerial* s)
{
    xBaseSave(ent, s);

    xASSERT_L(214, ent);
    xASSERT_L(215, s);
}

void zProgressScript_Load(zProgressScript* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    xASSERT_L(234, ent);
    xASSERT_L(235, s);
}

void zProgressScript_EventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    zProgressScript* s = (zProgressScript*)to;

    xASSERT_L(258, to);
    xASSERT_L(259, to->baseType == eBaseTypeProgressScript);

    switch (toEvent) {
    case eEventDebugReset:
    case eEventReset:
    case eEventScriptReset:
        zProgressScript_Reset(s);
        break;
    }
}
