#include "xCommon.h"
#include "zUICustomSysMessage.h"

#include "xEvent.h"
#include "xFont.h"
#include "xGroup.h"
#include "xPad.h"
#include "xString.h"
#include "xTextAsset.h"
#include "zBase.h"
#include "zEvent.h"
#include "zPlayer.h"
#include "zScene.h"
#include "zUI.h"
#include "zUIController.h"
#include "zUIIncredibles.h"
#include "zUIText.h"

bool gDontAutoMenu;
bool gSysMessageManager_active;
bool gPassAnyActveController;
zUI* pSysMessageManager;
zUICustom* pSysMessageManager_Custom;

zUICustomSysMessageManager::zUICustomSysMessageManager(zUI* ui) : zUICustom(ui)
{
    pSysMessageManager = ui;
    pSysMessageManager_Custom = this;

    enablePrevScreenHandler = true;
    infoNoPressTriangle = true;
    cb_function = NULL;
    toCBParams[0] = toCBParams[1] = toCBParams[2] = toCBParams[3] = 0.0f;
    toCBParamWidget = NULL;
    toCBParamWidgetID = 0;
    toCBto = NULL;
    infoTimer = 0.0f;
    padPortInitiatedSystemMessage = -1;
    lastPadPortInitiatedSystemMessage = -1;
    invisibleUICount = 0;
    for (S32 i = 0; i < MAX_INVISIBLE_UI; i++)
        invisibleUI[i] = NULL;
}

void zUICustomSysMessageManager::Init()
{
    xGroup* group = (xGroup*)zSceneFindObject(ui->asset->customWidget);
    xASSERT_L(48, group != NULL && group->baseType == eBaseTypeGroup);

    itemOkExtra = (zUIText*)zSceneFindObject(xStrHash("UI SYS MSG OK EXTRA TEXT"));
    itemNoExtra = (zUIText*)zSceneFindObject(xStrHash("UI SYS MSG NO EXTRA TEXT"));
    itemYesExtra = (zUIText*)zSceneFindObject(xStrHash("UI SYS MSG YES EXTRA TEXT"));
    itemOk = (zUIText*)zSceneFindObject(xStrHash("UI SYS MSG OK TEXT"));
    itemNo = (zUIText*)zSceneFindObject(xStrHash("UI SYS MSG NO TEXT"));
    itemYes = (zUIText*)zSceneFindObject(xStrHash("UI SYS MSG YES TEXT"));
    itemMsg = (zUIText*)zSceneFindObject(xStrHash("UI SYS MSG TEXT"));
    panelLowLeft = (zUIImage*)zSceneFindObject(xStrHash("SYS MSG PANEL LOW LEFT UII"));
    panelLowRight = (zUIImage*)zSceneFindObject(xStrHash("SYS MSG PANEL LOW RIGHT UII"));
    panelUpLeft = (zUIImage*)zSceneFindObject(xStrHash("SYS MSG PANEL UP LEFT UII"));
    panelUpRight = (zUIImage*)zSceneFindObject(xStrHash("SYS MSG PANEL UP RIGHT UII"));

    xASSERT_L(71, itemOk);
    xASSERT_L(72, itemNo);
    xASSERT_L(73, itemYes);
    xASSERT_L(74, itemMsg);

    invisibleUICount = 0;
}

bool zUICustomSysMessageManager::IsSignalAllowed(U32 toEvent)
{
    switch (toEvent) {
    case eEventPadPressX:
        if (infoTimer) {
            return false;
        }
        break;
    case eEventPadPressTriangle:
        if ((!infoNoPressTriangle && itemOk->focus) || infoTimer) {
            return false;
        }
        break;
    }

    return true;
}

void zUICustomSysMessageManager::SetInvisible(zUI* uiElement)
{
    xASSERT_L(107, invisibleUICount > -1 && invisibleUICount < 4);
    // BUG: skips slot 0, count 3 writes past the array
    invisibleUICount++;
    invisibleUI[invisibleUICount] = uiElement;
}

bool zUICustomSysMessageManager::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                                             U32 toParamWidgetID)
{
    zUICustom::HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);

    switch (toEvent) {
    case eEventUISignalActivateScreen:
        itemOk->visible = false;
        break;
    case eEventUISysMessageWaitInfo:
        infoTimer = 3.0f;
    case eEventUISysMessageWaitResponse:
    case eEventUISysMessageWaitConfirm:
    case eEventUISysMessageWaitResponseExtra:
        for (S32 i = 0; i < MAX_INVISIBLE_UI; i++) {
            if (invisibleUI[i])
                invisibleUI[i]->visible = false;
        }
        itemMsg->SetClearLayoutCache(true);

        if (toEvent != eEventUISysMessageWaitInfo) {
            infoTimer = 0.0f;
            if (padPortInitiatedSystemMessage == -1) {
                padPortInitiatedSystemMessage = lastPadPortInitiatedSystemMessage;
            }
        }

        fromUI = (zUI*)from;
        gSysMessageManager_active = true;
        enablePrevScreenHandler = true;
        fromUI->focus = false;
        if (toParamWidgetID) {
            itemMsg->SetText(xTextFindString(toParamWidgetID, NULL));
        }

        xPadClear();
        break;
    case eEventPadPressX:
        if (itemOk->focus || itemYes->focus || (itemOkExtra && itemOkExtra->focus) ||
            (itemYesExtra && itemYesExtra->focus)) {
            fromUI->HandleEvent(ui, eEventUISysMessageActionConfirmed, NULL, NULL, 0);
            if (cb_function) {
                cb_function(from, toCBto, toCBEvent, toCBParams, toCBParamWidget, toCBParamWidgetID);
            }
        }

        if (!(itemOk->focus || itemYes->focus || (itemOkExtra && itemOkExtra->focus) ||
              (itemYesExtra && itemYesExtra->focus))) {
            fromUI->HandleEvent(ui, eEventUISysMessageActionDeclined, NULL, NULL, 0);
        }
    case eEventSceneEnd:
        if (!gSysMessageManager_active) {
            break;
        }
    case eEventPadPressTriangle:
    {
        zUIController* c = (zUIController*)zSceneFindObject(xStrHash("UI PREV SCREEN HANDLER"));
        xASSERT_L(184, c);
        if (zUIIncrediblesGetPauseType() != PAUSE_JOIN && enablePrevScreenHandler) {
            zEntEvent(c, eEventUIFocusOn_Select);
        }
    }
    case eEventUIMotionFinishedOut:
        invisibleUICount = 0;

        itemYes->SetText(xTextFindString(xStrHash("UI TXT YES"), NULL));
        itemNo->SetText(xTextFindString(xStrHash("UI TXT NO"), NULL));

        for (S32 i = 0; i < MAX_INVISIBLE_UI; i++) {
            if (invisibleUI[i]) {
                invisibleUI[i]->visible = true;
                invisibleUI[i] = NULL;
            }
        }

        gSysMessageManager_active = false;
        if (padPortInitiatedSystemMessage != -1) {
            mPad[padPortInitiatedSystemMessage].pressed = 0;
            mPad[padPortInitiatedSystemMessage].released = 0;
            lastPadPortInitiatedSystemMessage = padPortInitiatedSystemMessage;
            padPortInitiatedSystemMessage = -1;
        }

        if (toEvent == eEventPadPressTriangle) {
            fromUI->HandleEvent(ui, eEventUISysMessageActionCanceled, NULL, NULL, 0);
        }

        if (fromUI->IsRestoreFocus()) {
            fromUI->focus = true;
            fromUI->visible = true;
            fromUI->selected = true;
        }

        infoNoPressTriangle = true;

        itemYes->visible = false;
        itemNo->visible = false;
        itemOk->visible = false;
        itemMsg->visible = false;

        itemYes->focus = false;
        itemNo->focus = false;
        itemMsg->focus = false;
        itemOk->focus = false;
        break;
    case eEventUISetMotion:
    default:
        break;
    }

    return true;
}

void zUICustomSysMessageManager::PreUpdate(F32 dt)
{
    if (infoTimer) {
        infoTimer -= dt;
        if (infoTimer <= 0.0f) {
            infoTimer = 0.0f;
            zEntEvent(ui, eEventPadPressX);
        }
    }
}

void zUICustomSysMessageManager::ShowSystemMessage(xBaseEventCallback cb, U32, U32 _toCBEvent,
                                                   const F32* _toCBParams, xBase* _toCBto, xBase* _toCBParamWidget,
                                                   U32 _toCBParamWidgetID, const char* sys_message)
{
    cb_function = cb;
    toCBEvent = _toCBEvent;
    if (_toCBParams) {
        toCBParams[0] = _toCBParams[0];
        toCBParams[1] = _toCBParams[1];
        toCBParams[2] = _toCBParams[2];
        toCBParams[3] = _toCBParams[3];
    } else {
        toCBParams[0] = toCBParams[1] = toCBParams[2] = toCBParams[3] = 0.0f;
    }
    toCBto = _toCBto;
    toCBParamWidget = _toCBParamWidget;
    toCBParamWidgetID = _toCBParamWidgetID;
    itemMsg->SetText(sys_message);
}
