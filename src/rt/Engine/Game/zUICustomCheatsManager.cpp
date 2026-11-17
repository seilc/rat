#include "xCommon.h"
#include "zUICustomCheatsManager.h"

#include "xString.h"
#include "xTextAsset.h"
#include "zEvent.h"
#include "zScene.h"
#include "zUI.h"
#include "zUICustomSysMessage.h"
#include "zUIUserString.h"

zUICustomCheatsManager* pUICustomCheatsManager;

namespace {
bool gCreditCode;
bool gClearUserString;
}

zUICustomCheatsManager::zUICustomCheatsManager(zUI* ui) : zUICustom(ui)
{
}

void zUICustomCheatsManager::Init()
{
}

bool zUICustomCheatsManager::HandleEvent(xBase* from, U32 toEvent, const F32*, xBase*, U32)
{
    switch (toEvent) {
    case eEventUISignalCreditCode:
        gCreditCode = true;

    case eEventUICheatOK:
        pSysMessageManager->Signal(ui, eEventUISysMessageWaitConfirm);
        ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(NULL, eEventUISysMessageWaitConfirm, 0, NULL, from,
                                                                                    NULL, 0, xTextFindString(xStrHash("UI TXT CHEAT GOOD"), NULL));

    case eEventUISignalHILogo:
        gClearUserString = true;
        break;

    case eEventUICheatBad:
        pSysMessageManager->Signal(ui, eEventUISysMessageWaitConfirm);
        ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(NULL, eEventUISysMessageWaitConfirm, 0, NULL, from,
                                                                                    NULL, 0, xTextFindString(xStrHash("UI TXT CHEAT BAD"), NULL));
        break;

    case eEventUISysMessageActionConfirmed:
    case eEventUISysMessageActionCanceled:
    {
        zUIUserString* userString = (zUIUserString*)zSceneFindObject(xStrHash("UI SECRETS DISPLAY STRING"));
        xASSERT_L(58, userString);
        if (gCreditCode) {
            zEntEvent(userString, eEventUISignalCreditCodeStart);
        }
        if (gClearUserString) {
            userString->DoReset();
            userString->visible = true;
        }
        gCreditCode = false;
        gClearUserString = false;
    }
    }

    return true;
}
