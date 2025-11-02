#include "xEvent.h"

#include "xDebug.h"
#include "xString.h"
#include "xOutputMgr.h"
#include "xstransvc.h"
#include "zScene.h"
#include "zEnt.h"
#include "zEvent.h"

#ifdef DEBUGRELEASE
char zEventLogBuf[20][256];
#endif

S32 zEventLogBufInit;
U32 gThisPlayer;
U32 gThisCameraTarget;

void zEntEvent(char* to, U32 toEvent)
{
    xASSERT(52, to);

    U32 id;
    xBase* sendTo;

    id = xStrHash(to);
    sendTo = zSceneFindObject(id);

    if (sendTo) {
        zEntEvent(sendTo, toEvent);
    } else {
        xOutWarn("EVENT", "zEntEvent unable to find %s\n", to);
    }
}

void zEntEvent(U32 toID, U32 toEvent)
{
    xASSERT(77, toID);

    xBase* sendTo;
    
    sendTo = zSceneFindObject(toID);

    if (sendTo) {
        zEntEvent(sendTo, toEvent);
    } else {
        xOutWarn("EVENT", "zEntEvent unable to find object with HashID %x\n", toID);
    }
}

void zEntEvent(U32 toID, U32 toEvent, F32 toParam0, F32 toParam1, F32 toParam2, F32 toParam3)
{
    xASSERT(90, toID);

    xBase* sendTo;
    F32 toParam[4];

    toParam[0] = toParam0;
    toParam[1] = toParam1;
    toParam[2] = toParam2;
    toParam[3] = toParam3;

    sendTo = zSceneFindObject(toID);

    if (sendTo) {
        zEntEvent(sendTo, toEvent, toParam);
    } else {
        xOutWarn("EVENT", "zEntEvent unable to find object with HashID %x\n", toID);
    }
}

void zEntEvent(xBase* from, U32 fromEvent, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID, ForceEvent forceEvent)
{
    xFAIL_AND_RETURN_IF(127, to == NULL);

#ifdef DEBUGRELEASE
    static const S32 MAX_REENTRANT_COUNT = 32;
    static S32 reentrantCount = 0;

    reentrantCount++;
    xASSERTM(134, reentrantCount < MAX_REENTRANT_COUNT, "Circular links detected!");

    zEventDebugLog(from, fromEvent, to, toEvent, toParam);

    if (from && to && from == to && fromEvent == toEvent) {
        xEventDebugLogDump();
        xASSERT(150, fromEvent != toEvent);
    }

    xOutInfo("EVENT", "zEntEvent: %s\n", zEventLogBuf[19]);
#endif

    if (to->baseType != eBaseTypeNPC) {
        if (toEvent == eEventDisable) {
            xBaseDisable(to);
        } else if (toEvent == eEventEnable) {
            xBaseEnable(to);
        }
    }

    if (to->eventFunc && (xBaseIsEnabled(to) || forceEvent == FE_YES || toEvent == eEventDebugReset)) {
        to->eventFunc(from, to, toEvent, toParam, toParamWidget, toParamWidgetID);
        if (to->baseFlags & k_XBASE_IS_ENTITY) {
            if (toEvent == eEventCollisionOn || toEvent == eEventCollision_Visible_On) {
                ((xEnt*)to)->collisionEventReceived = 1;
            } else if (toEvent == eEventCollisionOff || toEvent == eEventCollision_Visible_Off) {
                ((xEnt*)to)->collisionEventReceived = 2;
            }
        }
    }

    if (xBaseIsEnabled(to) && to->linkCount) {
        xASSERTM(209, to->link, "to %s", xSTAssetName(to));

        const xLinkAsset* idx = to->link;
        for (S32 i = 0; i < to->linkCount; i++, idx++) {
            xASSERT(215, idx);

            xBase* sendTo;

            if (toEvent != idx->srcEvent) continue;
            if (idx->chkAssetID && (!from || idx->chkAssetID != from->id)) continue;

            if (idx->dstAssetID == 0x6F7EC01E) { // TODO: Find out what asset this is
                sendTo = zSceneFindObject(gThisPlayer);
            } else {
                sendTo = zSceneFindObject(idx->dstAssetID);
            }
            if (!sendTo) continue;

            xBase* b = NULL;
            if (idx->paramWidgetAssetID) {
                if (idx->paramWidgetAssetID == 0xB3BB6012) { // TODO: Find out what asset this is
                    b = zSceneFindObject(gThisCameraTarget);
                } else {
                    b = zSceneFindObject(idx->paramWidgetAssetID);
                    if (!b) {
                        b = (xBase*)xSTFindAsset(idx->paramWidgetAssetID, NULL);
                    }
                }
            }

            zEntEvent(to, toEvent, sendTo, idx->dstEvent, idx->param, b, idx->paramWidgetAssetID, FE_NO);
        }
    }
    
#ifdef DEBUGRELEASE
    reentrantCount--;
#endif
}

void xEventDebugLogDump()
{
#ifdef DEBUGRELEASE
    iprintf("========================================== EVENT LOG DUMP [BEGIN]\n");

    for (S32 i = 0; i < 20; i++) {
        if (zEventLogBuf[i]) {
            iprintf(zEventLogBuf[i]);
        }
    }

    iprintf("========================================== EVENT LOG DUMP [END]\n");
#endif
}
