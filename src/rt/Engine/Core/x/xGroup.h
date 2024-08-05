#ifndef XGROUP_H
#define XGROUP_H

#include "xBase.h"
#include "xGroupAsset.h"

struct xGroup : xBase
{
    xGroupAsset* asset;
    xBase** item;
    U32 last_index;
    U32 ptr_last_index;
    S32 flg_group;
};

enum {
    XGRP_FLG_HASWIDGET = (1<<1),
    XGRP_FLG_HASASSET = (1<<2),
};

void xGroupInit(void* b, void* asset);
void xGroupInit(xBase* b, xGroupAsset* asset);
void xGroupSetup(xGroup* g);
void xGroupSave(xGroup* ent, xSerial* s);
void xGroupLoad(xGroup* ent, xSerial* s);
void xGroupReset(xGroup* ent);
void xGroupEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
U32 xGroupGetCount(xGroup* g);
xBase* xGroupGetItemPtr(xGroup* g, U32 index);
U32 xGroupGetItem(xGroup* g, U32 index);
U32 xGroupGetAny(xGroup* g);
xBase* xGroupGetAnyRandomPtr(xGroup* g);

#endif
