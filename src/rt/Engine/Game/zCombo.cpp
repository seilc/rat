#include "zCombo.h"

#include "xHudText.h"

static xhud::text_widget::widget_chunk* comboHUD = NULL;
static bool sComboIsPaused = false;

void zCombo_Add(F32 points, zComboType type)
{
    S32 rewardLevel;
    rewardLevel = rewardLevel;
}

#ifndef NON_MATCHING
static const char* __unused0[] = {
    "NumCombos",
    "ComboTimer",
    "ComboDisplayTime",
    "ComboFadeDir",
    "leftright",
    "updown",
#ifdef DEBUG
    "zCombo.cpp",
#endif
    "Combo%02d",
    "HUD_TEXT_COMBOMESSAGE",
};
#endif

void zCombo_Paused()
{
    if (sComboIsPaused) {
        return;
    }

    sComboIsPaused = true;

    if (comboHUD) {
        comboHUD->w.set_text("");
        comboHUD->w.clear_motives();
    }
}

#ifdef DEBUGRELEASE
#ifndef NON_MATCHING
static const char* __unused1[] = {
#ifdef RELEASE
    "zCombo.cpp",
#endif
    "comboReward[i].textAsset",
    "You need to get latest MNUI and in.ini",
    "Player|Combo|\2combo add points",
    "Player|Combo|\2comboInputMult",
    "Player|Combo|\2comboMaxTime",
    "Player|Combo|\3combo add type",
    "Player|Combo|Add Combo Point",
    "Go!",
    "Player|Combo|comboTimer",
    "Player|Combo|comboCounter",
    "Player|Combo|comboLevel",
    "Player|Combo|comboHitsNextLevel",
    "Player|Combo|\1comboCounterGraph|",
    "rewardLevel < comboNumRewards",
    "%s",
    "rewardLevel > -1",
};
#endif
#endif
