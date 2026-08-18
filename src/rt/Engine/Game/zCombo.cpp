#include "zCombo.h"

#include "xHudText.h"

#include "decomp.h"

static xhud::text_widget::widget_chunk* comboHUD = NULL;
static bool sComboIsPaused = false;

void zCombo_Add(F32 points, zComboType type)
{
    // Fakematch?
    S32 rewardLevel;
    rewardLevel = rewardLevel;
}

DECOMP_FORCEACTIVE(
    "NumCombos",
    "ComboTimer",
    "ComboDisplayTime",
    "ComboFadeDir",
    "leftright",
    "updown",
    DEBUG ? "zCombo.cpp" : 0,
    "Combo%02d",
    "HUD_TEXT_COMBOMESSAGE"
)

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

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE(
    "zCombo.cpp",
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
    "rewardLevel > -1"
)
#endif
