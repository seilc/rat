#include "xCommon.h"
#include "zUICustomMissionCompleteManager.h"

#include "xString.h"
#include "xTextAsset.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zSaveLoad.h"
#include "zScene.h"
#include "zUI.h"
#include "zUICustomStatsScreen.h"
#include "zUICustomSysMessage.h"
#include "zUIIncredibles.h"
#include "zUIMgr.h"
#include "zUIText.h"

namespace {
bool lastSceneWasAlreadyCompleted;
bool screenEnd = true;
bool allowPause;
}

bool zUICustomMissionComplete_IsAllowedPaused()
{
    return allowPause;
}

zUICustomMissionCompleteManager::zUICustomMissionCompleteManager(zUI* ui) : zUICustom(ui)
{
    saved = true;
    preupdatetimer = 0.0f;
}

void zUICustomMissionCompleteManager::Init()
{
}

bool zUICustomMissionCompleteManager::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                                                  U32 toParamWidgetID)
{
    switch (toEvent) {
    case eEventUISignalActivateScreen:
        lastSceneWasAlreadyCompleted = zUIIncrediblesAdvanceStage();
        pUICustomParrBestScreen->SetCompletedGoalsFlags();
        gPassAnyActveController = true;
        ui->forcepreupdate = true;
        if (globals.autoSaveFeature)
            preupdatetimer = 3.0f;
        else
            preupdatetimer = 1.0f;
        start_autosave();
        screenEnd = false;
        break;
    case eEventUISysMessageActionDeclined:
        break;
    case eEventUISysMessageActionConfirmed:
    case eEventUISysMessageActionCanceled:
    {
        zUIText* acceptText = (zUIText*)zSceneFindObject(xStrHash("UI STATS CONTINUE TEXT"));
        xASSERT_L(71, acceptText);
        zEntEvent(acceptText, eEventInvisible);
        zEntEvent(ui, eEventPadPressX);
        gPassAnyActveController = false;
        screenEnd = true;
        break;
    }
    default:
        break;
    }

    return zUICustom::HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}

void zUICustomMissionCompleteManager::start_autosave()
{
    if (globals.autoSaveFeature) {
        saved = false;
        pSysMessageManager->Signal(ui, eEventUISysMessageWaitInfo);
        ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->SetInfoTimer(preupdatetimer);
        ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
            NULL, eEventUISysMessageWaitInfo, 0, NULL, NULL, NULL, 0,
            xTextFindString(xStrHash("AUTO SAVE TXT"), NULL));
        zSaveLoadPreAutoSave(true);
    } else {
        saved = true;
    }
}

void zUICustomMissionCompleteManager::end_autosave()
{
    if (globals.autoSaveFeature) {
        saved = true;
        pSysMessageManager->Signal(ui, eEventUISysMessageWaitInfo);
        ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
            NULL, eEventUISysMessageWaitInfo, 0, NULL, NULL, NULL, 0,
            xTextFindString(xStrHash("UI TXT SAVE COMPLETED"), NULL));
        zSaveLoad_TriggerAutoSave();
    }
}

namespace {
void zSystemMessageCB(xBase*, xBase*, U32 toEvent, const F32*, xBase*, U32)
{
    switch (toEvent) {
    case eEventSys_Nope:
        zUISetTimer(1.0f);
        return;
    default:
        break;
    }

    xASSERT_L(129, 0);
}
}

void zUICustomMissionCompleteManager::PreUpdate(F32 dt)
{
    if (screenEnd)
        return;

    if (preupdatetimer) {
        preupdatetimer -= dt;
        if (preupdatetimer <= 0.0f) {
            preupdatetimer = 0.0f;
            if (!saved)
                end_autosave();
        }
        return;
    }

    if (!gSysMessageManager_active) {
        ui->forcepreupdate = false;
        allowPause = true;
        U32 strHash;
        if (lastSceneWasAlreadyCompleted) {
            strHash = xStrHash("UI TXT MISSCOMP MESSAGE NO BONUS");
            HandleEvent(NULL, eEventUISysMessageActionConfirmed, NULL, NULL, 0);
            return;
        }
        zUIText* acceptText = (zUIText*)zSceneFindObject(xStrHash("UI STATS CONTINUE TEXT"));
        xASSERT_L(163, acceptText);
        zEntEvent(acceptText, eEventVisible);
        strHash = xStrHash("UI TXT MISSCOMP MESSAGE");
        xASSERT_L(167, strHash);
        pSysMessageManager->Signal(ui, eEventUISysMessageWaitConfirm);
        ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->infoNoPressTriangle = 0;
        ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
            zSystemMessageCB, eEventUISysMessageWaitConfirm, eEventSys_Nope, NULL, NULL, NULL, 0,
            xTextFindString(strHash, NULL));
    }
}
