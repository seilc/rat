#include "xCommon.h"
#include "zUICustomUpgrade.h"

#include "xEvent.h"
#include "xString.h"
#include "xTextAsset.h"
#include "zCommonPlayer.h"
#include "zDispatcher.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zPortal.h"
#include "zScene.h"
#include "zUIText.h"

#include "decomp.h"

#include <stdio.h>

namespace {
// UNUSED: needed to match (code from RotU GC)
zUICustomUpgrade* pCustomUpgradesColumns[2];
S32 playerCount;
}

zUICustomUpgrade* pUICustomUpgradeManager;

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("zUICustomUpgrade.cpp", "playerCount == 1 || playerCount == 2", "%s", "!(port[0] == port[1] )",
                   "zPlayerContainer.h", "index >= 0 && index < numPlayers");
#endif

zUICustomUpgrade::zUICustomUpgrade(zUI* ui) : zUICustom(ui)
{
}

void zUICustomUpgrade::Init()
{
    return;

    columnNum = ui->asset->customWidget;
    numItems = 0;
    firstTime = true;
    numItems = ezCommonPlayerUpgradeType_Count;
    upgrades = (zUIText**)zUIAllocStaticMemory(numItems * sizeof(zUIText*));
    counts = (zUIText**)zUIAllocStaticMemory(numItems * sizeof(zUIText*));
    pCustomUpgradesColumns[columnNum] = this;
}

void zUICustomUpgrade::Reset()
{
    zUICustom::Reset();
}

void zUICustomUpgrade::Setup()
{
    return;

    zUIText* menuitem;
    S32 i;
    const char* leftOrRight;

    zUICustom::Setup();

    leftOrRight = columnNum ? "RIGHT" : "LEFT";
    for (i = 0; i < numItems; i++) {
        sprintf(temp, "UI UPGRADE %s STAT %d", leftOrRight, i + 1);
        menuitem = (zUIText*)zSceneFindObject(xStrHash(temp));
        upgrades[i] = menuitem;
        sprintf(temp, "UI UPGRADE %s COUNT %d", leftOrRight, i + 1);
        menuitem = (zUIText*)zSceneFindObject(xStrHash(temp));
        counts[i] = menuitem;
    }

    soundFXDeny = zSceneFindObject(xStrHash("SFX_ERROR"));
    soundFXConfirm = zSceneFindObject(xStrHash("SFX_CONFIRM"));
    soundFXLeft = zSceneFindObject(xStrHash("SFX_LEFT"));
    soundFXRight = zSceneFindObject(xStrHash("SFX_RIGHT"));
    soundFXUp = zSceneFindObject(xStrHash("SFX_UP"));
    soundFXDown = zSceneFindObject(xStrHash("SFX_DOWN"));
}

void zUICustomUpgrade::Exit()
{
    zUICustom::Exit();
}

void zUICustomUpgrade::SwapSides(S16 active, S16 inactive)
{
    return;

    pCustomUpgradesColumns[active]->ui->padport = ui->padport;
    pCustomUpgradesColumns[inactive]->ui->padport = -2;
    pCustomUpgradesColumns[inactive]->DeselectActive();
    pCustomUpgradesColumns[active]->activeItem = activeItem;
    pCustomUpgradesColumns[inactive]->activeItem = -1;
    pCustomUpgradesColumns[active]->SelectActive();
}

void zUICustomUpgrade::SelectActive()
{
    return;

    if (activeItem >= 0) {
        upgrades[activeItem]->HandleEvent(ui, eEventUIFocusOn_Select, NULL, NULL, 0);
        upgrades[activeItem]->Signal(eEventUIFocusOn_Select);
        counts[activeItem]->HandleEvent(ui, eEventUIFocusOn_Select, NULL, NULL, 0);
        counts[activeItem]->Signal(eEventUIFocusOn_Select);
    }
}

void zUICustomUpgrade::DeselectActive()
{
    return;

    if (activeItem >= 0) {
        upgrades[activeItem]->HandleEvent(ui, eEventUIFocusOff_Unselect, NULL, NULL, 0);
        counts[activeItem]->HandleEvent(ui, eEventUIFocusOff_Unselect, NULL, NULL, 0);
        upgrades[activeItem]->Signal(eEventUIFocusOff_Unselect);
        counts[activeItem]->Signal(eEventUIFocusOff_Unselect);
    }
}

void zUICustomUpgrade::PreUpdate(F32 dt)
{
    return;

    zUICustom::PreUpdate(dt);

    for (S32 i = 0; i < numItems; i++) {
        zCommonPlayerUpgradeLevel meleeUpgrade = ((zCommonPlayer*)pPlayer)->GetUpgradeLevel((zCommonPlayerUpgradeType)i);
        sprintf(temp, "UPGRADE ICON %d", meleeUpgrade);
        counts[i]->SetText(xTextFindString(xStrHash(temp), NULL));
    }
}

bool zUICustomUpgrade::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    zUIText* ui_text = (zUIText*)ui;
    S32 currentval = 0;
    U32 toUpgrade;

    switch (toEvent) {
    case eEventPadPressX:
        break;

    case eEventUpgradePowerUp:
        toUpgrade = *toParam;
        currentval = ((zCommonPlayer*)pPlayer)->GetUpgradeLevel((zCommonPlayerUpgradeType)toUpgrade);
        ((zCommonPlayer*)pPlayer)->SetUpgrade((zCommonPlayerUpgradeType)toUpgrade, (zCommonPlayerUpgradeLevel)(currentval + 1));
        break;

    case eEventPadPressTriangle:
        if (globals.sceneCur->pendingPortal && globals.sceneCur->pendingPortal->passet)
            ZDSP_elcb_event(from, from, eEventDispatcher_SetGameState_SceneSwitch, toParam, toParamWidget, toParamWidgetID);
        else {
            zUI* controller = (zUI*)zSceneFindObject(xStrHash("UI UPGRADE SCREEN"));
            controller->Signal(eEventPadPressTriangle);
        }
        break;

    case eEventPadPressRight:
        if (playerCount == 1) {
            SwapSides(1, 0);
            zEntEvent(soundFXRight, eEventPlay);
        }
        break;

    case eEventPadPressLeft:
        if (playerCount == 1) {
            SwapSides(0, 1);
            zEntEvent(soundFXLeft, eEventPlay);
        }
        break;

    case eEventPadPressUp:
        if (activeItem > 0) {
            DeselectActive();
            activeItem--;
            SelectActive();
            upgrades[activeItem]->Signal(eEventSuccess);
            zEntEvent(soundFXUp, eEventPlay);
        } else {
            upgrades[activeItem]->Signal(eEventFailure);
            zEntEvent(soundFXDeny, eEventPlay);
        }
        break;

    case eEventPadPressDown:
        if (activeItem < numItems - 1) {
            DeselectActive();
            activeItem++;
            SelectActive();
            upgrades[activeItem]->Signal(eEventSuccess);
            zEntEvent(soundFXDown, eEventPlay);
        } else {
            upgrades[activeItem]->Signal(eEventFailure);
            zEntEvent(soundFXDeny, eEventPlay);
        }
        break;

    case eEventInvisible:
        firstTime = true;
        break;
    }

    return zUICustom::HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}
