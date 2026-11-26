#include "xCommon.h"
#include "zConditional.h"

#include "xEvent.h"
#include "xserializer.h"
#include "xstransvc.h"
#include "zBase.h"
#include "zEvent.h"
#include "zScene.h"
#include "zVar.h"

void zConditionalInit(void* b, void* asset)
{
    zConditionalInit((xBase*)b, (zCondAsset*)asset);
}

void zConditionalInit(xBase* b, zCondAsset* asset)
{
    zConditional* c = (zConditional*)b;

    xBaseInit(b, asset);

    c->eventFunc = zConditionalEventCB;
    c->asset = asset;

    if (c->linkCount)
        c->link = (xLinkAsset*)(c->asset + 1);
    else
        c->link = NULL;

    xASSERT_FMT_L(63, (c->asset->expr1), "asset '%s' is invalid condition(al)", xSTAssetName(c->asset->id));

    for (S32 i = 0; i < eVarEntryCount; i++)
        if (c->asset->expr1 == zVarEntryTable[i].varNameID) {
            c->varEntry = &zVarEntryTable[i];
            break;
        }

    xASSERT_L(74, c->varEntry);
}

void zConditionalReset(zConditional* ent)
{
    xASSERT_L(80, ent);
    xASSERT_L(81, ent->asset);

    xBaseReset(ent, ent->asset);
}

void zConditionalSave(zConditional* ent, xSerial* s)
{
    xBaseSave(ent, s);

    xASSERT_L(100, ent);
    xASSERT_L(101, s);
}

void zConditionalLoad(zConditional* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    xASSERT_L(119, ent);
    xASSERT_L(120, s);
}

U32 zConditional_GetCount(zConditional* c)
{
    xASSERT_L(126, c->asset->expr1);
    zVarEntry* v = NULL;

    void* context = NULL;

    for (S32 i = 0; i < eVarEntryCount; i++)
        if (c->asset->expr1 == zVarEntryTable[i].varNameID) {
            v = &zVarEntryTable[i];
            if (i == eVarEntryCounterValue || i == eVarEntryIsEnabled || i == eVarEntryIsVisible ||
                i == eVarEntryIsReferenceNULL) {
                U32 id = c->asset->value_asset;
                if (id) context = zSceneFindObject(id);
            }
        }

    if (!v)
        return 0;

    U32 temp = v->varCB(context);

    if (temp >= 1 && temp <= 20)
        return eEventCount1 + temp - 1;
    else if (temp == 0)
        return eEventCount0;
    else {
        iprintf("zConditional_GetCount; Out of range (%i), returning 0\n", temp);
        return 0;
    }
}

U32 zConditional_Evaluate(zConditional* c)
{
    xASSERT_L(171, c->asset->expr1);

    U32 temp;
    void* context = NULL;

    if (!c->varEntry)
        return FALSE;

    if (c->varEntry->entry == eVarEntryCounterValue || c->varEntry->entry == eVarEntryIsEnabled ||
        c->varEntry->entry == eVarEntryIsVisible || c->varEntry->entry == eVarEntryIsReferenceNULL ||
        c->varEntry->entry == eVarEntryIsSceneUnlocked) {
        U32 id = c->asset->value_asset;
        if (id) context = zSceneFindObject(id);
    }

    temp = c->varEntry->varCB(context);

    switch (c->asset->op) {
    case k_ZCONDASSET_OP_EQ:
        if (temp == c->asset->constNum)
            return TRUE;
        else
            return FALSE;

    case k_ZCONDASSET_OP_GT:
        if (temp > c->asset->constNum)
            return TRUE;
        else
            return FALSE;

    case k_ZCONDASSET_OP_LT:
        if (temp < c->asset->constNum)
            return TRUE;
        else
            return FALSE;

    case k_ZCONDASSET_OP_GE:
        if (temp >= c->asset->constNum)
            return TRUE;
        else
            return FALSE;

    case k_ZCONDASSET_OP_LE:
        if (temp <= c->asset->constNum)
            return TRUE;
        else
            return FALSE;

    case k_ZCONDASSET_OP_NE:
        if (temp != c->asset->constNum)
            return TRUE;
        else
            return FALSE;

    default:
        iprintf("zConditional_Evaluate; Unknown zCondAsset op, returning FALSE\n");
        return FALSE;
    }
}

void zConditionalEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    zConditional* t = (zConditional*)to;

    xASSERT_L(253, to);
    xASSERT_L(254, to->baseType == eBaseTypeCond);

    switch (toEvent) {
    case eEventEvaluate:
        if (zConditional_Evaluate(t))
            zEntEvent(t, t, eEventTrue);
        else
            zEntEvent(t, t, eEventFalse);
        break;

    case eEvaluateCounterValue:
    {
        U32 event = zConditional_GetCount(t);
        if (event)
            zEntEvent(t, t, event);
        break;
    }

    case eEventTrue:
    case eEventFalse:
        break;

    case eEventReset:
    case eEventDebugReset:
        zConditionalReset(t);
        break;
    }
}
