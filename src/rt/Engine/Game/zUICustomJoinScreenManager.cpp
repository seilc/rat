#include "xCommon.h"
#include "zUICustomJoinScreenManager.h"

#include "xEvent.h"
#include "xString.h"
#include "xTextAsset.h"
#include "xTRC.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zUI.h"
#include "zUICustomSysMessage.h"
#include "zUIIncredibles.h"
#include "zUIText.h"

zUICustom* pUICustomDropJoinSceneManager;

namespace {
void zSystemEventCB(xBase*, xBase*, U32 toEvent, const F32*, xBase*, U32)
{
    switch (toEvent) {
    case eEventSys_Nope:
        xASSERT_L(27, zUIGetPadPortInitiatedPause() != -1);
        xFAIL_MSG_L(28, "This is invalid for this game, since it is single player");
        break;
    default:
        xASSERT_L(33, 0);
    }
}

bool bJoinDropActionConfirmed;
} // namespace

zUICustomJoinScreenManager::zUICustomJoinScreenManager(zUI* ui) : zUICustom(ui)
{
    textJoinDrop = NULL;
    textReturn = NULL;
    textPause = NULL;
}

void zUICustomJoinScreenManager::SetDropJoinText(S32 trccount)
{
    if (trccount == 2)
        textJoinDrop->SetText(xTextFindString(xStrHash("UI TXT JOIN DROP TXT"), NULL));
    else
        textJoinDrop->SetText(xTextFindString(xStrHash("UI TXT JOIN GAME TXT"), NULL));
}

void zUICustomJoinScreenManager::SetLockPauseText(bool value)
{
    textPause->locked = value;
    if (value)
        textPause->current.color.a = 55;
    else
        textPause->current.color.a = 255;
}

bool zUICustomJoinScreenManager::HandleEvent(xBase* from, U32 toEvent, const F32*, xBase*, U32)
{
    switch (toEvent) {
    case eEventUIFocusOn:
    {
        xASSERT_L(81, zUIGetPadPortInitiatedPause() != -1);
        if (zUIGetActivePlayersAtInitiatedPause() == 2)
            textJoinDrop->SetText(xTextFindString(xStrHash("UI TXT JOIN DROP TXT"), NULL));
        else
            textJoinDrop->SetText(xTextFindString(xStrHash("UI TXT JOIN GAME TXT"), NULL));
        bJoinDropActionConfirmed = false;
        S32 port = zUIGetPadPortInitiatedPause();
        if (xTRCPadGetStatePort(port) == TRC_PadInserted && xTRCPadGetPadStatePort(port) == TRC_PadStatePresent &&
            zUIIncrediblesGetPauseType() == PAUSE_JOIN) {
            xTRCPadSetStatePadPort(port, TRC_PadActive);
            xTRCPadReOrder();
        }
        break;
    }
    case eEventUIJoinDropGame:
    {
        pSysMessageManager->Signal(ui, eEventUISysMessageWaitResponse);
        F32 toParams[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
        xASSERT_L(107, zUIGetPadPortInitiatedPause() != -1);
        if (zUIGetActivePlayersAtInitiatedPause() == 2) {
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
                zSystemEventCB, eEventUISysMessageWaitResponse, eEventSys_Nope, toParams, from, NULL, 0,
                xTextFindString(xStrHash("UI TXT JOIN DROP MSG"), NULL));
        } else {
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
                zSystemEventCB, eEventUISysMessageWaitResponse, eEventSys_Nope, toParams, from, NULL, 0,
                xTextFindString(xStrHash("UI TXT JOIN GAME MSG"), NULL));
        }
        ((zUI*)from)->Signal(ui, eEventUIFocusOff_Unselect);
        break;
    }
    case eEventUISysMessageActionDeclined:
        return false;
    case eEventUISysMessageActionConfirmed:
        bJoinDropActionConfirmed = true;
        zEntEvent(textReturn, eEventPadPressX);
        break;
    case eEventUIFocusOff:
        if (!bJoinDropActionConfirmed && zUIGetActivePlayersAtInitiatedPause() == 1 &&
            zUIIncrediblesGetPauseType() == PAUSE_JOIN) {
            xTRCPadSetStatePadPort(zUIGetPadPortInitiatedPause(), TRC_PadInserted);
            xTRCPadReOrder();
        }
        break;
    default:
        break;
    }

    return true;
}

void zUICustomJoinScreenManager::Init()
{
    textJoinDrop = (zUIText*)zSceneFindObject(xStrHash("UI JOIN GAME TEXT"));
    xASSERT_FMT_L(166, (textJoinDrop), "'UI JOIN GAME TEXT' UI Text not found ! Ask a programmer ");

    textReturn = (zUIText*)zSceneFindObject(xStrHash("UI JOIN QUIT TEXT"));
    xASSERT_FMT_L(169, (textReturn), "'UI JOIN QUIT TEXT' UI Text not found ! Ask a programmer ");

    textPause = (zUIText*)zSceneFindObject(xStrHash("UI JOIN PAUSE TEXT"));
    xASSERT_FMT_L(172, (textPause), "'UI JOIN PAUSE TEXT' UI Text not found ! Ask a programmer ");

    ui->restoreFocus = false;
}
