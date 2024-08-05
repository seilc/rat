#include "xGroup.h"

#include "xEnt.h"
#include "zScene.h"
#include "zEvent.h"
#include "xRand.h"
#include "zNPCCoverPoint.h"
#include "xOutputMgr.h"

void xGroupInit(void* b, void* asset)
{
    xGroupInit((xBase*)b, (xGroupAsset*)asset);
}

void xGroupInit(xBase* b, xGroupAsset* asset)
{
    xGroup* t = (xGroup*)b;

    xBaseInit(b, asset);

    t->eventFunc = xGroupEventCB;
    t->asset = asset;

    if (t->linkCount > 0) {
        t->link = (xLinkAsset*)((U8*)t->asset + sizeof(xGroupAsset) + sizeof(U32) * asset->itemCount);
    } else {
        t->link = NULL;
    }

    U32 count = xGroupGetCount(t);
    
    t->item = (count > 0) ? (xBase**)xMEMALLOC(sizeof(xBase*) * count, 0, eMemMgrTag_Scene, 0, 53) : NULL;
    t->last_index = 0;
    t->ptr_last_index = 0;
    t->flg_group = 0;
}

void xGroupSetup(xGroup* g)
{
    xASSERT(64, g);

    if (g->flg_group & 0x1) return;

    U32 count = xGroupGetCount(g);
    for (U32 i = 0; i < count; i++) {
        U32 itemID = xGroupGetItem(g, i);

        g->item[i] = zSceneFindObject(itemID);
        if (g->item[i]) {
            g->flg_group |= XGRP_FLG_HASWIDGET;
        } else {
            g->item[i] = (xBase*)xSTFindAsset(itemID, NULL);
            if (g->item[i]) {
                g->flg_group |= XGRP_FLG_HASASSET;
            }
        }
    }
    
    g->flg_group |= 0x1;

    xASSERTFMT(99, ((XGRP_FLG_HASWIDGET|XGRP_FLG_HASASSET) & g->flg_group) != (
                     XGRP_FLG_HASWIDGET|XGRP_FLG_HASASSET),
               "ERROR:  Group 0x%08x  \"%s\" has both widgets and assets!\n",
               g->id, xSTAssetName(g->id));
}

void xGroupSave(xGroup* ent, xSerial* s)
{
    xBaseSave(ent, s);

    xASSERT(119, ent);
    xASSERT(120, s);
}

void xGroupLoad(xGroup* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    xASSERT(138, ent);
    xASSERT(139, s);
}

void xGroupReset(xGroup* ent)
{
    xASSERT(144, ent);
    xASSERT(145, ent->asset);

    xBaseReset(ent, ent->asset);
}

void xGroupEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    xGroup* g = (xGroup*)to;
    xASSERT(167, g);
    xASSERT(168, g->asset);

    switch (toEvent) {
    case eEventReset:
    case eEventDebugReset:
        xGroupReset(g);
        break;
    case eEventDisableGroupContents:
        toEvent = eEventDisable;
        break;
    case eEventEnableGroupContents:
        toEvent = eEventEnable;
        break;
    }

    S32 rand = -1;
    if (g->asset->groupFlags & 0x1) {
        rand = xrand_RandomChoice(g->asset->itemCount);
    }

    U32* idx = (U32*)((U8*)g->asset + sizeof(xGroupAsset));
    switch (toEvent) {
    case eEventNPCRetreatToCover:
        zNPCCoverManageRetreatToCover(from, g, toEvent, toParam, (xGroup*)toParamWidget, toParamWidgetID);
        break;
    case eEventFastVisible:
    case eEventFastInvisible:
    case eEventVisibilityCullOn:
    case eEventVisibilityCullOff:
    {
        void(*entVisEventHandler)(xEnt*);
        switch (toEvent) {
        case eEventFastVisible:
            entVisEventHandler = xEntShow;
            break;
        case eEventFastInvisible:
            entVisEventHandler = xEntHide;
            break;
        case eEventVisibilityCullOn:
            entVisEventHandler = xEntVisibilityCullOn;
            break;
        case eEventVisibilityCullOff:
            entVisEventHandler = xEntVisibilityCullOff;
            break;
        }
        for (S32 i = 0; i < g->asset->itemCount; i++) {
            if (rand == -1 || rand == i) {
                xBase* b = g->item[i];
                if (b) {
                    if (b->baseFlags & k_XBASE_IS_ENTITY) {
                        if (xBaseIsEnabled(b)) {
                            entVisEventHandler((xEnt*)b);
                        }
                    } else {
                        zEntEvent(b, toEvent, toParam, toParamWidget, toParamWidgetID);
                    }
                }
            }
        }
        break;
    }
    default:
        for (S32 i = 0; i < g->asset->itemCount; i++) {
            if (rand == -1 || rand == i) {
                xBase* b = g->item[i];
                if (b) {
                    zEntEvent(b, toEvent, toParam, toParamWidget, toParamWidgetID);
                } else {
                    xOutWarn("GROUP", "Group %s referencing ### object %08X not found!!\n",
                             zSceneGetName(g), idx[i]);
                }
            }
        }
        break;
    }
}

U32 xGroupGetCount(xGroup* g)
{
    xASSERT(279, g);
    xASSERT(280, g->asset);

    return g->asset->itemCount;
}

xBase* xGroupGetItemPtr(xGroup* g, U32 index)
{
    xASSERT(301, g);

    if (!(g->flg_group & 0x1)) {
        xOutWarn("GROUP", "xGroupGetItemPtr called before xGroupSetup.\n");
        xGroupSetup(g);
    }

    return g->item ? g->item[index] : NULL;
}

U32 xGroupGetItem(xGroup* g, U32 index)
{
    xASSERT(319, g);
    xASSERT(320, g->asset);
    xASSERT(323, index < g->asset->itemCount);

    U32* idx = (U32*)((U8*)g->asset + sizeof(xGroupAsset));
    return idx[index];
}

#ifndef NON_MATCHING
void xGroupUpdate(xBase* to, xScene* sc, F32)
{
    xASSERT(0, to);
    xASSERT(0, sc);
}
#endif

U32 xGroupGetAny(xGroup* g)
{
    xASSERT(354, g->asset);
    if (g->asset->itemCount == 0) return 0;
    xASSERT(356, g->last_index < g->asset->itemCount);

    U32* ids = (U32*)((U8*)g->asset + sizeof(xGroupAsset));
    U32 id = ids[g->last_index];

    g->last_index = (g->last_index+1) % g->asset->itemCount;

    return id;
}

#ifndef NON_MATCHING
void xGroupGetAnyPtr(xGroup* g)
{
    xASSERT(0, g->ptr_last_index < g->asset->itemCount);
}
#endif

xBase* xGroupGetAnyRandomPtr(xGroup* g)
{
    xASSERT(384, g->asset);
    if (g->asset->itemCount == 0) return NULL;

    U32 idx = xrand_RandomChoice(g->asset->itemCount);
    return g->item[idx];
}
