#include "xCommon.h"
#include "zUICustomSaveLoad.h"

#include "xEvent.h"
#include "xGroup.h"
#include "xPad.h"
#include "xPtankPool.h"
#include "xString.h"
#include "xTextAsset.h"
#include "xTRC.h"
#include "xutil.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zSaveLoad.h"
#include "zScene.h"
#include "zUI.h"
#include "zUICustomSysMessage.h"
#include "zUIText.h"

zUICustomSaveLoadManager* pUICustomSaveLoadManager;

// UNUSED: needed to match (code from RotU GC)
void zUICustomNewGameManager::Init()
{
    if (globals.sceneCur->sceneID == IDTAG('M','N','U','S')) {
        backText = (zUIText*)zSceneFindObject(xStrHash("UIS MAINM BACK TEXT"));
        xASSERT(backText);
    }
}

// UNUSED: needed to match (code from RotU GC)
bool zUICustomNewGameManager::HandleEvent(xBase* from, U32 toEvent, const F32*, xBase*, U32)
{
    switch (toEvent) {
    case eEventUI_StartNewGame:
        if (xTRCPadCountWereActiveTotal() == 1) {
            zEntEvent(backText, eEventVisible);
        }
        pSysMessageManager->Signal(ui, eEventUISysMessageWaitResponse);
        ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
            NULL, eEventUISysMessageWaitResponse, 0, NULL, from, NULL, 0,
            xTextFindString(xStrHash("UI TXT CREATESAVE"), NULL));
        break;
    case eEventUISysMessageActionCanceled:
        if (xTRCPadCountWereActiveTotal() == 1) {
            zEntEvent(backText, eEventInvisible);
        }
        break;
    case eEventUISysMessageActionDeclined:
        zSceneSetLastSceneID(globals.sceneCur->sceneID);
        zEntEvent(ui, eEventUI_StartNewGameNoCreateGame);
        break;
    case eEventUISysMessageActionConfirmed:
        zEntEvent(ui, eEventUI_StartNewGameYesCreateGame);
        break;
    }

    return true;
}

namespace {
S32 sysMsgType = -1;
} // namespace

zUICustomSaveLoadManager::zUICustomSaveLoadManager(zUI* ui) : zUICustom(ui)
{
    backText = NULL;
    selectText = NULL;
}

void zUICustomSaveLoadManager::Init()
{
    backText = (zUIText*)zSceneFindObject(xStrHash("UI SAVE BACK TEXT"));
    selectText = (zUIText*)zSceneFindObject(xStrHash("UI SAVE SELECT TEXT"));
    xASSERT_L(88, backText);
    xASSERT_L(89, selectText);
}

bool zUICustomSaveLoadManager::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase*, U32)
{
    switch (toEvent) {
    case eEventVisible:
        xASSERT_L(98, toParam[0]);
        sysMsgType = toParam[1];
        switch (sysMsgType) {
        case 7:
            pSysMessageManager->Signal(ui, eEventUISysMessageWaitConfirm);
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->enablePrevScreenHandler = 0;
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->infoNoPressTriangle = 0;
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
                NULL, eEventUISysMessageWaitConfirm, 0, NULL, from, NULL, 0,
                xTextFindString(*(const U32*)&toParam[0], NULL));
            break;
        case 1:
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->SetInvisible(backText);
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->SetInvisible(selectText);
            pSysMessageManager->Signal(ui, eEventUISysMessageWaitInfo);
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->enablePrevScreenHandler = 0;
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
                NULL, eEventUISysMessageWaitInfo, 0, NULL, from, NULL, 0,
                xTextFindString(*(const U32*)&toParam[0], NULL));
            break;
        case 2:
            pSysMessageManager->Signal(ui, eEventUISysMessageWaitConfirm);
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->enablePrevScreenHandler = 0;
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
                NULL, eEventUISysMessageWaitConfirm, 0, NULL, from, NULL, 0,
                xTextFindString(*(const U32*)&toParam[0], NULL));
            break;
        case 3:
            pSysMessageManager->Signal(ui, eEventUISysMessageWaitResponse);
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->itemYes->SetText(
                xTextFindString(xStrHash("UI TXT SYS MESSAGE CONTINUE"), NULL));
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->itemNo->SetText(
                xTextFindString(xStrHash("UI TXT SYS MESSAGE RETRY"), NULL));
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->enablePrevScreenHandler = 0;
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
                NULL, eEventUISysMessageWaitResponse, 0, NULL, from, NULL, 0,
                xTextFindString(*(const U32*)&toParam[0], NULL));
            break;
        case 4:
            pSysMessageManager->Signal(ui, eEventUISysMessageWaitResponse);
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->itemYes->SetText(
                xTextFindString(xStrHash("UI TXT SYS MESSAGE CONTINUE WITHOUT SAVING"), NULL));
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->itemNo->SetText(
                xTextFindString(xStrHash("UI TXT SYS MESSAGE RETRY"), NULL));
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->enablePrevScreenHandler = 0;
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
                NULL, eEventUISysMessageWaitResponse, 0, NULL, from, NULL, 0,
                xTextFindString(*(const U32*)&toParam[0], NULL));
            break;
        case 5:
            pSysMessageManager->Signal(ui, eEventUISysMessageWaitResponseExtra);
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->itemYesExtra->SetText(
                xTextFindString(xStrHash("UI TXT SYS MESSAGE CONTINUE"), NULL));
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->itemOkExtra->SetText(
                xTextFindString(xStrHash("UI TXT FORMAT CARD GC"), NULL));
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->itemNoExtra->SetText(
                xTextFindString(xStrHash("UI TXT SYS MESSAGE RETRY"), NULL));
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->enablePrevScreenHandler = 0;
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
                NULL, eEventUISysMessageWaitResponseExtra, 0, NULL, from, NULL, 0,
                xTextFindString(*(const U32*)&toParam[0], NULL));
            break;
        case 6:
            pSysMessageManager->Signal(ui, eEventUISysMessageWaitResponseExtra);
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->itemYesExtra->SetText(
                xTextFindString(xStrHash("UI TXT SYS MESSAGE CONTINUE WITHOUT SAVING"), NULL));
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->itemOkExtra->SetText(
                xTextFindString(xStrHash("UI TXT FORMAT CARD GC"), NULL));
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->itemNoExtra->SetText(
                xTextFindString(xStrHash("UI TXT SYS MESSAGE RETRY"), NULL));
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->enablePrevScreenHandler = 0;
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
                NULL, eEventUISysMessageWaitResponseExtra, 0, NULL, from, NULL, 0,
                xTextFindString(*(const U32*)&toParam[0], NULL));
            break;
        default:
            pSysMessageManager->Signal(ui, eEventUISysMessageWaitResponse);
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->enablePrevScreenHandler = 0;
            ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
                NULL, eEventUISysMessageWaitResponse, 0, NULL, from, NULL, 0,
                xTextFindString(*(const U32*)&toParam[0], NULL));
            break;
        }
        break;
    case eEventUISysMessageActionCanceled:
        xASSERT_L(179, sysMsgType != -1);
        switch (sysMsgType) {
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        default:
            zSaveLoad_DispatchCB(eEventDispatcher_SLBack, NULL);
            break;
        }
        break;
    case eEventUISysMessageActionDeclined:
        xASSERT_L(194, sysMsgType != -1);
        switch (sysMsgType) {
        case 3:
        case 4:
        case 5:
        case 6:
            zSaveLoad_DispatchCB(eEventDispatcher_SLRetry, NULL);
            break;
        default:
            zSaveLoad_DispatchCB(eEventDispatcher_SLBack, NULL);
            break;
        }
        break;
    case eEventUISysMessageActionConfirmed:
        xASSERT_L(211, sysMsgType != -1);
        switch (sysMsgType) {
        case 3:
        case 4:
        case 5:
        case 6:
            if (((zUICustomSysMessageManager*)pSysMessageManager_Custom)->itemOkExtra->focus)
                zSaveLoad_DispatchCB(eEventDispatcher_SLOkay, NULL);
            else
                zSaveLoad_DispatchCB(eEventDispatcher_SLContinue, NULL);
            break;
        default:
            zSaveLoad_DispatchCB(eEventDispatcher_SLOkay, NULL);
            break;
        }
        break;
    case eEventUIFocusOn_Select:
    {
        xGroup* slotGroup = (xGroup*)zSceneFindObject(xStrHash("UI SAVE GAMESLOT GROUP"));
        xASSERT_L(234, slotGroup);
        zEntEvent(slotGroup, eEventInvisible);
        break;
    }
    default:
        break;
    }

    return true;
}
