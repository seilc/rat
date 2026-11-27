#include "xCommon.h"
#include "zUICustomOptionsManager.h"

#include "containers.h"
#include "xEvent.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xPad.h"
#include "xSnd.h"
#include "xString.h"
#include "xTextAsset.h"
#include "xTRC.h"
#include "xutil.h"
#include "zDispatcher.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zUI.h"
#include "zUIBox.h"
#include "zUICustomSysMessage.h"
#include "zUICustomVolume.h"
#include "zUIText.h"

bool inOverlayScreen;
F32 inOverlayScreenExitTimer;

namespace {
S32 sysMsgType;
bool screenAdjusted;
bool screenIsInSystemMessage;
} // namespace

bool zUICustomOptionsScreenIsInSystemMessage()
{
    return screenIsInSystemMessage;
}

void zUICustomOptionsScreenAdjust(bool value)
{
    screenAdjusted = value;
}

zUICustomOptionsManager::zUICustomOptionsManager(zUI* ui) : zUICustom(ui)
{
    musicVolume = 0.0f;
    sfxVolume = 0.0f;
    vibrationOnP1 = false;
    vibrationOnP2 = false;
    screenAdjusted = false;
    doupdate = true;
    soundMode = -1;

    optionVibP1Text = NULL;
    optionVibP2Text = NULL;
    optionVibP1Box = NULL;
    optionVibP2Box = NULL;
}

void zUICustomOptionsManager::Init()
{
    optionVibP1Text = (zUIText*)zSceneFindObject(xStrHash("UI OPTIONS VIBRATION TEXT"));
    optionVibP2Text = (zUIText*)zSceneFindObject(xStrHash("UI OPTIONS VIBRATION P2 TEXT"));
    optionVibP1Box = (zUIBox*)zSceneFindObject(xStrHash("UI OPTIONS VIBRATION BOX"));
    optionVibP2Box = (zUIBox*)zSceneFindObject(xStrHash("UI OPTIONS VIBRATION P2 BOX"));

    xASSERT_FMT_L(60, (optionVibP1Text), "'UI Text' widget 'UI OPTIONS VIBRATION TEXT' not found, ask a programmer.");
    xASSERT_FMT_L(61, (optionVibP2Text), "'UI Text' widget 'UI OPTIONS VIBRATION P2 TEXT' not found, ask a programmer.");
    xASSERT_FMT_L(62, (optionVibP1Box), "'UI Text' widget 'UI OPTIONS VIBRATION BOX' not found, ask a programmer.");
    xASSERT_FMT_L(63, (optionVibP2Box), "'UI Text' widget 'UI OPTIONS VIBRATION P2 BOX' not found, ask a programmer.");
}

bool zUICustomOptionsManager::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                                          U32 toParamWidgetID)
{
    switch (toEvent) {
    case eEventUISignalDeactivateScreen:
    case eEventUISignalDeactivatedScreen:
        ui->focus = false;
        ui->visible = false;
        ui->selected = false;
        screenAdjusted = false;
        screenIsInSystemMessage = false;
        break;

    case eEventUISignalActivateScreen:
        ui->focus = true;
        ui->visible = true;
        ui->selected = true;
        screenAdjusted = false;
        zScene_ScreenAdjustMode_SetMenuOffset();

        musicVolume = pUICustomMusicVolumeManager->GetVolume();
        sfxVolume = pUICustomSFXVolumeManager->GetVolume();
        vibrationOnP1 = globals.option_vibration_p1 ? true : false;
        vibrationOnP2 = globals.option_vibration_p2 ? true : false;
        soundMode = xSndGetMode();
        globals.option_vibration_p1_menu = vibrationOnP1;
        globals.option_vibration_p2_menu = vibrationOnP2;

        sysMsgType = 0;
        doupdate = true;
        break;

    case eEventUISignalActivatedScreen:
    case eEventUISysMessageActionCanceled:
        screenIsInSystemMessage = false;
        switch (sysMsgType) {
        case 1:
            AdjustScreenOnOff();
            sysMsgType = 0;
            if (!zScene_ScreenAdjustMode_WasAdjusted())
                screenAdjusted = false;
            break;
        }

        zSendUIEvent("UI PREV SCREEN HANDLER", eEventUIFocusOff_Unselect);
        zSendUIEvent("UI SCREEN MANAGER FADE HELPER", eEventUIFocusOff_Unselect);
        break;

    case eEventPadPressTriangle:
        if (!doupdate)
            return false;
        if (inOverlayScreen)
            return false;
        if (inOverlayScreenExitTimer)
            return false;
        {
            bool isBreak = false;
            xASSERT_L(129, sysMsgType != -1);
            switch (sysMsgType) {
            case 0:
            {
                F32 curMusicVolume = pUICustomMusicVolumeManager->GetVolume();
                F32 curSfxVolume = pUICustomSFXVolumeManager->GetVolume();
                S32 curSoundMode = xSndGetMode();
                if (musicVolume != curMusicVolume || sfxVolume != curSfxVolume || soundMode != curSoundMode ||
                    vibrationOnP1 != (globals.option_vibration_p1 ? true : false) ||
                    vibrationOnP2 != (globals.option_vibration_p2 ? true : false) || screenAdjusted) {
                    screenIsInSystemMessage = true;
                    pSysMessageManager->Signal(ui, eEventUISysMessageWaitResponse);
                    ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->enablePrevScreenHandler = false;
                    ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
                        NULL, eEventUISysMessageWaitResponse, 0, NULL, from, NULL, 0,
                        xTextFindString(xStrHash("UI TXT OPTIONS CONFIRM EO MSG"), NULL));
                    isBreak = true;
                }
                break;
            }
            case 1:
            case 2:
                break;
            default:
                xASSERT_L(157, 0);
                break;
            }
            if (isBreak)
                break;
        }

    case eEventUISysMessageActionDeclined:
    {
        bool isBreak = false;
        xASSERT_L(167, sysMsgType != -1);
        switch (sysMsgType) {
        case 0:
        {
            pUICustomMusicVolumeManager->SetVolume(musicVolume);
            pUICustomSFXVolumeManager->SetVolume(sfxVolume);
            xSndSetMode((xSndMode)soundMode);
            xBase* d = zSceneFindObject(xStrHash("UI PAUSE DISP"));
            zSceneResetScreenAdj();

            if (vibrationOnP1 != (globals.option_vibration_p1 ? true : false)) {
                if (vibrationOnP1)
                    ZDSP_elcb_event(ui, d, eEventDispatcher_PadVibrateOn_P1, NULL, NULL, 0);
                else
                    ZDSP_elcb_event(ui, d, eEventDispatcher_PadVibrateOff_P1, NULL, NULL, 0);
            }
            if (vibrationOnP2 != (globals.option_vibration_p2 ? true : false)) {
                if (vibrationOnP2)
                    ZDSP_elcb_event(ui, d, eEventDispatcher_PadVibrateOn_P2, NULL, NULL, 0);
                else
                    ZDSP_elcb_event(ui, d, eEventDispatcher_PadVibrateOff_P2, NULL, NULL, 0);
            }

            screenIsInSystemMessage = false;
            break;
        }
        case 1:
            break;
        case 2:
        {
            if (toEvent != eEventUISysMessageActionDeclined) {
                sysMsgType = 0;
                isBreak = true;
                break;
            }

            pUICustomMusicVolumeManager->SetVolume(musicVolume);
            pUICustomSFXVolumeManager->SetVolume(sfxVolume);
            xSndSetMode((xSndMode)soundMode);
            xBase* d = zSceneFindObject(xStrHash("UI PAUSE DISP"));
            zSceneResetScreenAdj();

            if (vibrationOnP1 != (globals.option_vibration_p1 ? true : false)) {
                if (vibrationOnP1)
                    ZDSP_elcb_event(ui, d, eEventDispatcher_PadVibrateOn_P1, NULL, NULL, 0);
                else
                    ZDSP_elcb_event(ui, d, eEventDispatcher_PadVibrateOff_P1, NULL, NULL, 0);
            }
            if (vibrationOnP2 != (globals.option_vibration_p2 ? true : false)) {
                if (vibrationOnP2)
                    ZDSP_elcb_event(ui, d, eEventDispatcher_PadVibrateOn_P2, NULL, NULL, 0);
                else
                    ZDSP_elcb_event(ui, d, eEventDispatcher_PadVibrateOff_P2, NULL, NULL, 0);
            }

            screenIsInSystemMessage = false;
            break;
        }
        default:
            xASSERT_L(236, 0);
            break;
        }
        if (isBreak)
            break;
    }

    case eEventUISysMessageActionConfirmed:
        screenIsInSystemMessage = false;
        xASSERT_L(247, sysMsgType != -1);
        switch (sysMsgType) {
        case 2:
            zSendUIEvent("UI OPTIONS CONTROLS TEXT", eEventUIEnterControlsScreen);
            doupdate = false;
            inOverlayScreenExitTimer = 0.0f;
            if (globals.sceneCur->sceneID == IDTAG('M','N','U','S'))
                globals.option_vibration_p2 = globals.option_vibration_p1;
            break;
        case 0:
            zSendUIEvent("UI PREV SCREEN HANDLER", eEventUIFocusOn_Select);
            zSendUIEvent("UI SCREEN MANAGER FADE HELPER", eEventUIFocusOn_Select);
            zSendUIEvent("UI PREV SCREEN HANDLER", eEventPadPressTriangle);
            doupdate = false;
            inOverlayScreenExitTimer = 0.0f;
            if (globals.sceneCur->sceneID == IDTAG('M','N','U','S'))
                globals.option_vibration_p2 = globals.option_vibration_p1;
            break;
        case 1:
            AdjustScreenOnOff();
            sysMsgType = 0;
            break;
        default:
            xASSERT_L(283, 0);
            break;
        }
        break;

    case eEventPadPressX:
        if (!IsPS2Demo()) {
            zUIText* adjustText = (zUIText*)zSceneFindObject(xStrHash("UI OPTIONS SCREEN ADJUST TEXT"));
            xASSERT_L(293, adjustText);
            if (adjustText->focus) {
                AdjustScreenOnOff();
                sysMsgType = 1;
                screenIsInSystemMessage = true;
                pSysMessageManager->Signal(ui, eEventUISysMessageWaitConfirm);
                ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->enablePrevScreenHandler = false;
                ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
                    NULL, eEventUISysMessageWaitConfirm, 0, NULL, from, NULL, 0,
                    xTextFindString(xStrHash("UI TXT OPTIONS SCREEN ADJUST INFO TXT"), NULL));
                break;
            }

            zUIText* controlsText = (zUIText*)zSceneFindObject(xStrHash("UI OPTIONS CONTROLS TEXT"));
            xASSERT_L(306, controlsText);
            if (controlsText->focus) {
                sysMsgType = 2;
                F32 curMusicVolume = pUICustomMusicVolumeManager->GetVolume();
                F32 curSfxVolume = pUICustomSFXVolumeManager->GetVolume();
                S32 curSoundMode = xSndGetMode();
                if (musicVolume != curMusicVolume || sfxVolume != curSfxVolume || soundMode != curSoundMode ||
                    vibrationOnP1 != (globals.option_vibration_p1 ? true : false) ||
                    vibrationOnP2 != (globals.option_vibration_p2 ? true : false) || screenAdjusted) {
                    screenIsInSystemMessage = true;
                    pSysMessageManager->Signal(ui, eEventUISysMessageWaitResponse);
                    ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->enablePrevScreenHandler = false;
                    ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
                        NULL, eEventUISysMessageWaitResponse, 0, NULL, from, NULL, 0,
                        xTextFindString(xStrHash("UI TXT OPTIONS CONFIRM CC MSG"), NULL));
                    break;
                }

                zSendUIEvent("UI OPTIONS CONTROLS TEXT", eEventUIEnterControlsScreen);
                doupdate = false;
                inOverlayScreenExitTimer = 0.0f;
            }
        }
        break;
    }

    return zUICustom::HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}

void zUICustomOptionsManager::PreUpdate(F32 dt)
{
    if (inOverlayScreenExitTimer) {
        inOverlayScreenExitTimer -= dt;
        if (inOverlayScreenExitTimer < 0.0f)
            inOverlayScreenExitTimer = 0.0f;
    }

    if (xTRCPadCountWereActiveTotal() == 1) {
        optionVibP2Text->locked = true;
        zEntEvent(optionVibP2Text, eEventInvisible);
        zEntEvent(optionVibP2Box, eEventInvisible);
        optionVibP1Text->SetText(xTextFindString(xStrHash("UI TXT OPTIONS VIBRATION"), NULL));
    } else {
        optionVibP2Text->locked = false;
        zEntEvent(optionVibP2Text, eEventVisible);
        zEntEvent(optionVibP2Box, eEventVisible);
        optionVibP1Text->SetText(xTextFindString(xStrHash("UI TXT OPTIONS VIBRATION P1"), NULL));
        optionVibP2Text->SetText(xTextFindString(xStrHash("UI TXT OPTIONS VIBRATION P2"), NULL));
    }
}

void zUICustomOptionsManager::AdjustScreenOnOff()
{
    zUIText* textField = (zUIText*)zSceneFindObject(xStrHash("UI OPTIONS SCREEN ADJUST TEXT"));
    if (textField->focus) {
        if (zScene_ScreenAdjustMode())
            ZDSP_elcb_event(ui, ui, eEventDispatcherScreenAdjustOFF, NULL, NULL, 0);
        else
            ZDSP_elcb_event(ui, ui, eEventDispatcherScreenAdjustON, NULL, NULL, 0);
    }
}
