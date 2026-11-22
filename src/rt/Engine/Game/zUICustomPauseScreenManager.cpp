#include "xCommon.h"
#include "zUICustomPauseScreenManager.h"

#include "xString.h"
#include "xTextAsset.h"
#include "xTextureManager.h"
#include "xTRC.h"
#include "zDispatcher.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zMain.h"
#include "zUI.h"
#include "zUICustomLevelManager.h"
#include "zUICustomSysMessage.h"

enum enPAUSE_TYPE
{
    PAUSE_QUIT,
    PAUSE_SWITCH_LEVEL,
    PAUSE_JOIN
};

namespace {
enPAUSE_TYPE pauseSysType_cur = PAUSE_SWITCH_LEVEL;
enPAUSE_TYPE pauseSysType_old = PAUSE_SWITCH_LEVEL;
}

zUICustomPauseScreenManager::zUICustomPauseScreenManager(zUI* ui) : zUICustom(ui)
{
}

void zUICustomPauseScreenManager::Init()
{
}

bool zUICustomPauseScreenManager::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                                              U32 toParamWidgetID)
{
    zUICustom::HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
    switch (toEvent) {
    case eEventUIQuitGame:
    {
        U32 strHash;
        if (GCN_DEMO_PARAM)
            strHash = xStrHash("UI TXT QUITC OXM MESSAGE");
        else
            strHash = xStrHash("UI TXT QUITC MESSAGE");
        F32 toParams[4] = { 0 };
        pSysMessageManager->Signal(ui, eEventUISysMessageWaitResponse);
        ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
            ZDSP_elcb_event, eEventUISysMessageWaitResponse, eEventWarpGo, toParams, from, NULL, 0,
            xTextFindString(strHash, NULL));
        ((zUI*)from)->Signal(ui, eEventUIFocusOff_Unselect);
        pauseSysType_old = pauseSysType_cur;
        pauseSysType_cur = PAUSE_QUIT;
        break;
    }
    case eEventUISysMessageActionConfirmed:
        if (pauseSysType_cur == PAUSE_QUIT) {
            for (S32 i = 0; i < k_MAX_TRC_PADS; i++) {
                if (xTRCPadIsPort(i, TRC_PadActive))
                    xTRCPadSetStatePadPort(i, TRC_PadInserted);
                else if (!xTRCPadIsPort(i, TRC_PadMissing)) {
                    if (xTRCPadGetPadStatePort(i) == TRC_PadStateMissingDuringGameplay) {
                        xTRCPadSetStatePadPort(i, TRC_Unknown);
                        xTRCPadSetPadStatePort(i, TRC_PadStateUnknown);
                        xTRCPadRemove(i);
                    } else
                        xTRCPadSetStatePadPort(i, TRC_PadInserted);
                } else {
                    xTRCPadSetStatePadPort(i, TRC_Unknown);
                    xTRCPadSetPadStatePort(i, TRC_PadStateUnknown);
                    xTRCPadRemove(i);
                }
            }
            xTRCPadReOrder();
            globals.autoSaveFeature = false;
            globals.UnlimitedNukes = false;
            S32 portInitiatedSysMsg =
                ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->GetPortInititatedSystemMessage();
            zUIIncrediblesSetSceneCurrent(1);
            if (globals.demoType == zDT_OPM)
                ZDSP_elcb_event(ui, ui, eEventUIXboxDemoExitToLauncher, NULL, NULL, 0);
            if (globals.demoType == zDT_PUBLICITY) {
                xTextureManager::Get().FreeMemory();
                globals.firstStartPressed = false;
            } else
                xTRCPadSetStatePadPort(portInitiatedSysMsg, TRC_PadActive);
            xTRCPadReOrder();
        }
    case eEventUISysMessageActionDeclined:
    case eEventUISysMessageActionCanceled:
        pauseSysType_cur = pauseSysType_old;
    }

    return true;
}
