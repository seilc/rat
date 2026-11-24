#include "xCommon.h"
#include "zUICustomControllerManager.h"

#include "xEvent.h"
#include "xFont.h"
#include "xString.h"
#include "xTextAsset.h"
#include "xTRC.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zUI.h"
#include "zUICustomSysMessage.h"
#include "zUIIncredibles.h"
#include "zUIImage.h"
#include "zUIText.h"

namespace {
bool mriIsOnTheLeft = true;
S32 secondPlayerPort = -1;
S32 totalPlayers = -1;
S32 sysMsgType = -1;

void zSystemEventCB(xBase*, xBase*, U32 toEvent, const F32*, xBase*, U32)
{
    switch (toEvent) {
    case eEventSys_Nope:
        break;
    default:
        xASSERT_L(180, 0);
        break;
    }
}
} // namespace

zUICustomControllerManager* pUICustomControllerManager;

zUICustomControllerManager::zUICustomControllerManager(zUI* ui) : zUICustom(ui)
{
    acceptText = NULL;
    characterSelectText = NULL;
    backText = NULL;
}

void zUICustomControllerManager::Init()
{
    portPressedStart = -1;
}

void zUICustomControllerManager::ResetMriOnLeft()
{
    if (!mriIsOnTheLeft) {
    }
}

bool zUICustomControllerManager::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                                             U32 toParamWidgetID)
{
    switch (toEvent) {
    case eEventPadPressRight:
        break;
    case eEventPadPressLeft:
        break;
    case eEventPadPressX:
        if (xTRCPadCountWereActiveTotal() == 2) {
            zUIAutoScreenManagerFocusOnSelect();
            zEntEvent(ui, eEventSuccess);
        }
        break;
    case eEventPadPressStart:
        xASSERT_L(296, portPressedStart > -1);
        totalPlayers = xTRCPadCountWereActiveTotal();
        sysMsgType = 0;
        if (totalPlayers == 2)
            ShowSystemMessage(from, xStrHash("UI TXT JOIN DROP MSG"));
        else {
            S32 startidx = 0;
            if (!xTRCPadFindFirstPadState(startidx, TRC_PadStateMissingDuringGameplay))
                acceptText->visible = true;
            characterSelectText->visible = true;
            ShowSystemMessage(from, xStrHash("UI TXT JOIN GAME MSG"));
        }
        break;
    case eEventUISignalActivatedScreen:
        zUIAutoScreenManagerFocusOffUnselect();
        if (xTRCPadCountWereActiveTotal() == 2) {
            acceptText->visible = true;
            characterSelectText->visible = true;
            secondPlayerText->current.color.r = 220;
            secondPlayerText->current.color.g = 140;
            secondPlayerText->current.color.b = 40;
            zEntEvent(infoImage, eEventVisible);
        }
        if (!mriIsOnTheLeft) {
            zEntEvent(secondPlayerText, eEventUISelect);
        }
        break;
    case eEventUISignalActivateScreen:
        zUIIncrediblesSetPauseType(PAUSE_MAINMENU);
        if (xTRCPadCountWereActiveTotal() == 1) {
            secondPlayerPort = -1;
            infoText->SetText(xTextFindString(xStrHash("UIS TXT SELECT PLYR 2"), NULL));
        }
        break;
    case eEventUISignalDeactivatedScreen:
        zEntEvent(ui, eEventUIFocusOff_Unselect);
        break;
    case eEventUISignalDeactivateScreen:
        characterSelectText->visible = false;
        zUIIncrediblesSetPauseType(PAUSE_NORMAL);
        break;
    case eEventUISysMessageActionDeclined:
    case eEventUISysMessageActionCanceled:
        xASSERT_L(358, sysMsgType != -1);
        switch (sysMsgType) {
        case 0:
            if (totalPlayers == 1) {
                acceptText->visible = false;
                characterSelectText->visible = false;
                xTRCPadSetStatePadPort(pUICustomControllerManager->portPressedStart, TRC_PadInserted);
                xTRCPadReOrder();
            }
            zUIAutoScreenManagerFocusOffUnselect();
            break;
        case 1:
            break;
        default:
            xASSERT_L(377, 0);
            break;
        }
        break;
    case eEventUISysMessageActionConfirmed:
        xASSERT_L(384, sysMsgType != -1);
        switch (sysMsgType) {
        case 0:
            if (totalPlayers == 1) {
                acceptText->visible = true;
                characterSelectText->visible = true;
                secondPlayerText->current.color.r = 220;
                secondPlayerText->current.color.g = 140;
                secondPlayerText->current.color.b = 40;
                infoText->SetText(xTextFindString(xStrHash("UIS TXT PRESS START TO DROP"), NULL));
                zEntEvent(infoImage, eEventVisible);
                secondPlayerPort = pUICustomControllerManager->portPressedStart;
            } else {
                acceptText->visible = false;
                characterSelectText->visible = false;
                secondPlayerText->current.color.r = 0;
                secondPlayerText->current.color.g = 0;
                secondPlayerText->current.color.b = 0;
                infoText->SetText(xTextFindString(xStrHash("UIS TXT SELECT PLYR 2"), NULL));
                zEntEvent(infoImage, eEventInvisible);
            }
            zUIAutoScreenManagerFocusOffUnselect();
            break;
        case 1:
            totalPlayers = 2;
            zSystemEventCB(NULL, NULL, eEventSys_Nope, NULL, NULL, 1);
            ResetMriOnLeft();
            zUIAutoScreenManagerFocusOnSelect();
            zSendUIEvent("UI PREV SCREEN HANDLER", eEventPadPressTriangle);
            break;
        default:
            xASSERT_L(426, 0);
            break;
        }
        break;
    case eEventPadPressTriangle:
        if (xTRCPadCountWereActiveTotal() == 2) {
            sysMsgType = 1;
            pSysMessageManager->Signal(ui, eEventUISysMessageWaitResponse);
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
                NULL, eEventUISysMessageWaitResponse, 0, NULL, from, NULL, 0,
                xTextFindString(xStrHash("UIS TXT RETURN TO MAIN MENU TXT"), NULL));
        } else {
            ResetMriOnLeft();
            zUIAutoScreenManagerFocusOnSelect();
            zSendUIEvent("UI PREV SCREEN HANDLER", eEventPadPressTriangle);
        }
        break;
    default:
        break;
    }

    return zUICustom::HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}

void zUICustomControllerManager::ShowSystemMessage(xBase* from, U32 strHash)
{
    pSysMessageManager->Signal(ui, eEventUISysMessageWaitResponse);
    F32 toParams[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
    ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->SetPortInititatedSystemMessage(portPressedStart);
    ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
        zSystemEventCB, eEventUISysMessageWaitResponse, eEventSys_Nope, toParams, from, NULL, 0,
        xTextFindString(strHash, NULL));
}
