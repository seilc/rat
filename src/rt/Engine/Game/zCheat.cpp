#include "xCommon.h"
#include "zCheat.h"

#include "xCam.h"
#include "xEntMotion.h"
#include "xFXHighDynamicRange.h"
#include "xHud.h"
#include "xRand.h"
#include "xScrFx.h"
#include "xSpacePartitionTree.h"
#include "xutil.h"
#include "zBomb.h"
#include "zCommonPlayer.h"
#include "zFMV.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zUIImage.h"

#include <string.h>

zCheats gCheats;
bool gCheatBouncyHenchmen;
bool gCheatExtraShrapnel;

namespace {

F32 cheatTimer;
F32 headScale = 1.0f;
F32 targetHead = 1.0f;
F32 discoTimer;
const char* cheatName;

void zCheatRemoveTimed()
{
    gCheats.halfDamage = false;
    gCheats.incredipower = false;
    gCheats.chronos = false;
    gCheats.noFriction = false;
    cheatTimer = 0.0f;
}

zCheatStatus Cheat_DanielTheFlash(); // UNUSED
bool MrsI(); // UNUSED
bool MrI(); // UNUSED
zCheatStatus Cheat_HalfDamage(); // UNUSED

bool IncrediBall()
{
    return false;
}

zCheatStatus Cheat_Kronos(); // UNUSED

// UNUSED: needed to match (code from Incredibles GC)
zCheatStatus Cheat_Smartbomb()
{
    if (MrI() || MrsI()) {
        if (globals.sceneCur->sceneID != IDTAG('O', 'M', '0', '4') &&
            globals.sceneCur->sceneID != IDTAG('N', 'J', '0', '3') &&
            globals.sceneCur->sceneID != IDTAG('N', 'I', '0', '3') &&
            globals.sceneCur->sceneID != IDTAG('H', 'S', '0', '1')) {
            sphere_damage(
                CURPLAYERS[0], CURPLAYERS[0]->frame->mat.pos, 25.0f, 0, 1000, 5.0f, 1.0f, zHS_EXPLOSION,
                zHT_GENERAL);
        } else {
            sphere_damage(
                CURPLAYERS[0], CURPLAYERS[0]->frame->mat.pos, 25.0f, 0, 1, 5.0f, 1.0f, zHS_EXPLOSION,
                zHT_GENERAL);
        }
        return zCheatStatus_Ok;
    }
    return zCheatStatus_Bad;
}

zCheatStatus Cheat_DashLikes(); // UNUSED

// UNUSED: needed to match (speculative code)
bool Dash()
{
    return false;
}

zCheatStatus Cheat_DoneA(); // UNUSED
zCheatStatus Cheat_DoneB(); // UNUSED
zCheatStatus Cheat_ShowMe(); // UNUSED
zCheatStatus Cheat_LevelLocksmith(); // UNUSED
zCheatStatus Cheat_LevelLocksmithLL(); // UNUSED
zCheatStatus Cheat_UnlockScenes(); // UNUSED
zCheatStatus Cheat_IncrediPower(); // UNUSED
zCheatStatus Cheat_Showtime(); // UNUSED
zCheatStatus Cheat_Flexible(); // UNUSED
zCheatStatus Cheat_TonyLoaf(); // UNUSED

// UNUSED: needed to match (speculative code)
bool Violet()
{
    return false;
}

zCheatStatus Cheat_BoaPlace(); // UNUSED
zCheatStatus Cheat_InvertTurret(); // UNUSED

// UNUSED: needed to match (code from Incredibles GC)
zCheatStatus Cheat_KonamiCode()
{
    if (!Violet() && !Dash()) {
        if (CURPLAYERS[0]->GetCombat()->currentHitPoints > 0) {
            CURPLAYERS[0]->GetCombat()->currentHitPoints = MIN(
                CURPLAYERS[0]->GetCombat()->currentHitPoints + 30,
                CURPLAYERS[0]->GetCombat()->maximumHitPoints);
        }
        return zCheatStatus_Ok;
    }
    return zCheatStatus_Bad;
}

zCheatStatus Cheat_BouncyHenchmen(); // UNUSED
zCheatStatus Cheat_ExtraShrapnel(); // UNUSED

// UNUSED: needed to match (code from Incredibles GC)
zCheatStatus Cheat_SlowMo()
{
    if (globals.timeMultiplierTarget == 0.5f) {
        globals.timeMultiplierTarget = 1.0f;
    } else {
        globals.timeMultiplierTarget = 0.5f;
    }
    return zCheatStatus_Ok;
}

zCheatStatus Cheat_FlamingPlants(); // UNUSED
zCheatStatus Cheat_DiscoMode(); // UNUSED

void newDiscoColor()
{
    F32 val = xrand_RandomFloatRange(1.0f, 2.0f);

    switch (xrand_RandomChoice(4)) {
    case 0:
        xFXHighDynamicRangeSetColorize(val, 0.0f, 0.0f, 1.0f);
        break;
    case 1:
        xFXHighDynamicRangeSetColorize(0.0f, 0.0f, val, 1.0f);
        break;
    case 2:
        xFXHighDynamicRangeSetColorize(0.0f, val, 0.0f, 1.0f);
        break;
    case 3:
        xFXHighDynamicRangeSetColorize(val, val, 0.0f, 1.0f);
        break;
    }
    discoTimer = xrand_RandomFloatRange(0.5f, 3.0f);
}

zCheatStatus Cheat_FlamingFeet(); // UNUSED
zCheatStatus Cheat_MakeHarder(); // UNUSED
zCheatStatus Cheat_MakeEasier(); // UNUSED
zCheatStatus Cheat_Blurry(); // UNUSED
zCheatStatus Cheat_Ethereal(); // UNUSED
zCheatStatus Cheat_GlowingSwimmer(); // UNUSED

zCheatStatus Cheat_HeadRescale(F32 scale)
{
    if (!IncrediBall()) {
        gCheats.scaleHead = true;
        if (targetHead != scale) {
            targetHead = scale;
        } else {
            targetHead = 1.0f;
        }
        return zCheatStatus_Ok;
    }
    return zCheatStatus_Bad;
}

zCheatStatus Cheat_BigHeadMode()
{
    return Cheat_HeadRescale(3.0f);
}

zCheatStatus Cheat_SmallHeadMode()
{
    return Cheat_HeadRescale(0.5f);
}

zCheatStatus Cheat_BigMouth()
{
    gCheats.bigMouth = !gCheats.bigMouth;
    return zCheatStatus_Ok;
}

zCheatStatus Cheat_BigCritters()
{
    gCheats.bigCritters = !gCheats.bigCritters;
    return zCheatStatus_Ok;
}

zCheatStatus Cheat_HUD()
{
    if (!globals.bAllowMasterCheats) {
        return zCheatStatus_Bad;
    }

    globals.enableHUD = !globals.enableHUD;
    return zCheatStatus_Ok;
}

zCheatStatus Cheat_FastMo()
{
    if (globals.timeMultiplierTarget == 2.0f) {
        globals.timeMultiplierTarget = 1.0f;
    } else {
        globals.timeMultiplierTarget = 2.0f;
    }
    return zCheatStatus_Ok;
}

zCheatStatus Cheat_HILogo()
{
    return zCheatStatus_HILogo;
}

zCheatStatus Cheat_CreditCode()
{
    return zCheatStatus_CreditCode;
}

zCheatStatus Cheat_Nukes2(); // UNUSED

zCheatStatus Cheat_Nukes()
{
    if (!globals.bAllowMasterCheats) {
        return zCheatStatus_Bad;
    }

    globals.UnlimitedNukes = !globals.UnlimitedNukes;
    return zCheatStatus_Ok;
}

zCheatStatus Cheat_Invincible()
{
    if (!globals.bAllowMasterCheats) {
        return zCheatStatus_Bad;
    }

    globals.settings.TakeDamage = !globals.settings.TakeDamage;
    return zCheatStatus_Ok;
}

void Cheat_GiveXP(S32); // UNUSED
zCheatStatus Cheat_GiveXP1(); // UNUSED
zCheatStatus Cheat_GiveXP10(); // UNUSED
zCheatStatus Cheat_GiveXP100(); // UNUSED
zCheatStatus Cheat_GiveXP1000(); // UNUSED
zCheatStatus Cheat_GiveXP10000(); // UNUSED
zCheatStatus Cheat_MrIXP(); // UNUSED
zCheatStatus Cheat_FrozoneXP(); // UNUSED
zCheatStatus Cheat_MrIGiveNuke(); // UNUSED
zCheatStatus Cheat_FrozoneGiveNuke(); // UNUSED
zCheatStatus Cheat_Checkpoint(); // UNUSED
void UpgradePlayer(zCommonPlayer* player); // UNUSED
zCheatStatus Cheat_MrIUpgrades(); // UNUSED
zCheatStatus Cheat_FroUpgrades(); // UNUSED

zCheatStatus Cheat_DoubleXP()
{
    if (globals.XPMultiplier != 2.0f)
        globals.XPMultiplier = 2.0f;
    else
        globals.XPMultiplier = 1.0f;

    return zCheatStatus_Ok;
}

struct Cheat
{
    const char* name;
    zCheatStatus (*callback)();
};

Cheat cheats[] = {
    { "ChaChing", Cheat_DoubleXP },
    { "EgoProblem", Cheat_BigHeadMode },
    { "FunkyChicken", Cheat_BigCritters },
    { "Headroom", Cheat_SmallHeadMode },
    { "HUDBeGone", Cheat_HUD },
    { "ICantHearYou", Cheat_BigMouth },
    { "InAHurry", Cheat_FastMo },
    { "Nuke", Cheat_Nukes },
    { "RollCall", Cheat_CreditCode },
    { "Shamless", Cheat_HILogo },
    { "Unstoppable", Cheat_Invincible },
};

#define NUM_CHEATS (sizeof(cheats) / sizeof(cheats[0]))

} // namespace

void zCheatInit()
{
    U32 UNLOCK_INDEX = 0;
    while (UNLOCK_INDEX < NUM_CHEATS) {
        UNLOCK_INDEX++;
    }

    if (UNLOCK_INDEX < NUM_CHEATS) {
        static char unlock_clear[32] = "";
        char* uncrypt = unlock_clear;
        const char* crypt = "x\024\024\035\031\025\031\037\tf";

        char last = 0;
        do {
            *uncrypt = *crypt ^ (last ^ 0x11);
            last = *uncrypt;
            crypt++;
            uncrypt++;
            *uncrypt = '\0';
        } while (last);

        cheats[UNLOCK_INDEX].name = unlock_clear;
    }

    zCheatRemoveAll();
}

void zCheatRemoveAll()
{
    cheatTimer = 0.0f;
    memset(&gCheats, 0, sizeof(gCheats));
    headScale = 1.0f;
    targetHead = 1.0f;
    gCheatExtraShrapnel = false;
    gCheatBouncyHenchmen = false;
}

void zCheatReset()
{
    if (globals.sceneCur->sceneID == IDTAG('M', 'N', 'U', 'S')) {
        zCheatRemoveAll();
    } else {
        zCheatRemoveTimed();
        if (globals.sceneCur->sceneID == IDTAG('C', 'I', '0', '3') ||
            globals.sceneCur->sceneID == IDTAG('F', 'T', '0', '1') ||
            globals.sceneCur->sceneID == IDTAG('L', 'D', '0', '4')) {
            gCheats.flamingFeet = false;
        }
    }
}

void zCheatUpdate(F32 dt)
{
    if (cheatTimer > 0.5f && cheatTimer - dt <= 0.5f) {
        xColor color = { 0xFF, 0xFF, 0xFF, 0x80 };
        xScrFxFade(&color, &color, 0.5f, 0);
    }

    if (gCheats.scaleHead && targetHead != headScale) {
        if (headScale > targetHead) {
            headScale -= dt;
            if (headScale <= targetHead) {
                headScale = targetHead;
            }
        } else {
            headScale += dt;
            if (headScale >= targetHead) {
                headScale = targetHead;
            }
        }

        if (headScale == 1.0f && targetHead == 1.0f) {
            gCheats.scaleHead = false;
        }
    }

    if (gCheats.discoMode) {
        discoTimer -= dt;
        if (discoTimer <= 0.0f) {
            newDiscoColor();
        }
    }

    cheatTimer -= dt;
    if (cheatTimer < 0.0f) {
        zCheatRemoveTimed();
    }
}

F32 zCheatGetHeadScale()
{
    return headScale;
}

zCheatStatus zCheatProcess(const char* name)
{
    cheatName = name;

    S32 low = 0, high = NUM_CHEATS;
    while (low < high) {
        S32 mid = (low + high) / 2;
        S32 compare = stricmp(name, cheats[mid].name);
        if (compare == 0)
            return cheats[mid].callback();
        else if (compare < 0)
            high = mid;
        else
            low = mid + 1;
    }

    return zCheatStatus_Bad;
}
