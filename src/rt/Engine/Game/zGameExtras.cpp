#include "xCommon.h"
#include "zGameExtras.h"

#include "iSystem.h"
#include "containers.h"
#include "xFont.h"
#include "xPad.h"
#include "xserializer.h"
#include "xTRC.h"
#include "xutil.h"
#include "zGame.h"
#include "zGameState.h"
#include "zGlobals.h"
#include "zHashes.h"
#include "zMain.h"
#include "zScene.h"

#include "decomp.h"

#include <string.h>

#define CHEAT_COUNT 16

struct EGGItem;

struct EGGMisc
{
    S32 other;
};

struct EGGData
{
    union
    {
        S32 placeholder;
        EGGMisc misc;
    };
};

struct EGGItem
{
    S32 (*fun_check)(EGGItem*);
    struct EGGItemFuncs* funcs;
    S32 enabled;
    EGGData eggdata;
};

struct EGGItemFuncs
{
    void (*fun_update)(F32, EGGItem*);
    void (*fun_init)(EGGItem*);
    void (*fun_reset)(EGGItem*);
    void (*fun_done)(EGGItem*);
};

static S32 g_enableGameExtras;
static S32 g_currDay;
static S32 g_currMonth;
static S32 g_gameExtraFlags;

static S32 EGG_check_ExtrasFlags(EGGItem*);

static EGGItemFuncs EGGEmpty = { NULL, NULL, NULL, NULL };
static EGGItem g_eggBasket[] = {
    { EGG_check_ExtrasFlags, &EGGEmpty, 0, 0 },
    { NULL, NULL, 0, 0 }
};

void zGameExtras_SceneInit()
{
    xASSERT_L(242, globals.sceneCur);

    g_enableGameExtras = FALSE;

    g_currDay = iGetDay();
    g_currMonth = iGetMonth();

    S32 somethingIsEnabled = 0;
    EGGItem* egg_next = g_eggBasket;
    while (egg_next->fun_check) {
        EGGItem* egg = egg_next;
        egg_next++;
        egg->enabled = egg->fun_check(egg);
        if (!egg->enabled) {
            continue;
        }
        somethingIsEnabled++;
        if (egg->funcs->fun_init) {
            egg->funcs->fun_init(egg);
        }
    }

    if (somethingIsEnabled) g_enableGameExtras = TRUE;
}

void zGameExtras_SceneReset()
{
    if (!g_enableGameExtras) {
        return;
    }

    EGGItem* egg_next = g_eggBasket;
    while (egg_next->fun_check) {
        EGGItem* egg = egg_next;
        egg_next++;
        if (!egg->enabled) {
            continue;
        }
        if (egg->funcs->fun_reset) {
            egg->funcs->fun_reset(egg);
        }
    }
}

void zGameExtras_SceneExit()
{
    if (!g_enableGameExtras) return;
    if (globals.cmgr) return;
    if (zGameIsPaused()) return;

    EGGItem* egg_next = g_eggBasket;
    while (egg_next->fun_check) {
        EGGItem* egg = egg_next;
        egg_next++;
        if (!egg->enabled) continue;
        egg->enabled = FALSE;
        if (egg->funcs->fun_done) {
            egg->funcs->fun_done(egg);
        }
    }

    g_enableGameExtras = FALSE;
    g_currDay = 0;
    g_currMonth = 0;
}

void zGameExtras_SceneUpdate(F32 dt)
{
    zGameCheatsUpdate(dt);

    if (!g_enableGameExtras) return;
    if (globals.cmgr) return;
    if (zGameIsPaused()) return;

    EGGItem* egg_next = g_eggBasket;
    while (egg_next->fun_check) {
        EGGItem* egg = egg_next;
        egg_next++;
        if (!egg->enabled) continue;
        if (egg->funcs->fun_update) {
            egg->funcs->fun_update(dt, egg);
        }
    }
}

static S32 GE_IsStandardScene(); // UNUSED
static S32 GE_SceneIsOk(const char* nam_scene); // UNUSED
static S32 GE_SceneIsOk(U32 tag_scene); // UNUSED
static S32 EGG_check_Standard(EGGItem*); // UNUSED

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("HB01");
#endif

// UNUSED: needed to match (speculative code)
static S32 EGG_check_TEST(EGGItem* egg)
{
    iprintf("EGGTEST: Check enable called (0x%08X)\n", egg);
    return TRUE;
}

// UNUSED: needed to match (code from TSSM GC)
static void EGG_update_TEST(F32 dt, EGGItem* egg)
{
    static S32 doOnce = TRUE;
    if (doOnce) {
        iprintf("EGGTEST: Update called (0x%08X %.3f)\n", egg, dt);
        doOnce = FALSE;
    }
}

// UNUSED: needed to match (code from TSSM GC)
static void EGG_init_TEST(EGGItem* egg)
{
    iprintf("EGGTEST: Init called (0x%08X)\n", egg);
}

// UNUSED: needed to match (code from TSSM GC)
static void EGG_reset_TEST(EGGItem* egg)
{
    iprintf("EGGTEST: Reset called (0x%08X)\n", egg);
}

// UNUSED: needed to match (code from TSSM GC)
static void EGG_done_TEST(EGGItem* egg)
{
    iprintf("EGGTEST: Done called (0x%08X)\n", egg);
}

static S32 EGG_check_BDayAll(EGGItem*); // UNUSED
static void EGG_update_BDay(F32, EGGItem*); // UNUSED
static void EGG_init_BDay(EGGItem*); // UNUSED
static void EGG_done_BDay(EGGItem*); // UNUSED
static void EGG_reset_BDay(EGGItem*); // UNUSED
static S32 EGG_check_DecSnow(EGGItem*); // UNUSED
static void EGG_update_Snow(F32, EGGItem*); // UNUSED
static void EGG_init_Snow(EGGItem*); // UNUSED
static void EGG_reset_Snow(EGGItem*); // UNUSED
static void EGG_done_Snow(EGGItem*); // UNUSED
static S32 EGG_check_AprilRain(EGGItem*); // UNUSED
static void EGG_update_Rain(F32, EGGItem*); // UNUSED
static void EGG_init_Rain(EGGItem*); // UNUSED
static void EGG_reset_Rain(EGGItem*); // UNUSED
static void EGG_done_Rain(EGGItem*); // UNUSED

static S32 EGG_check_ExtrasFlags(EGGItem*)
{
    switch (g_currMonth) {
    case 1:
        if (g_currDay == 1) g_gameExtraFlags |= 0x1;
        break;
    case 2:
        if (g_currDay == 2) g_gameExtraFlags |= 0x100;
        break;
    case 3:
        if (g_currDay == 15) g_gameExtraFlags |= 0x8;
        if (g_currDay == 17) g_gameExtraFlags |= 0x4;
        if (g_currDay == 18) g_gameExtraFlags |= 0x100;
        if (g_currDay == 21) g_gameExtraFlags |= 0x100;
        if (g_currDay == 22) g_gameExtraFlags |= 0x100;
        break;
    case 4:
        if (g_currDay == 1) g_gameExtraFlags |= 0x80;
        break;
    case 5:
        break;
    case 6:
        if (g_currDay == 6) g_gameExtraFlags |= 0x40;
        break;
    case 7:
        if (g_currDay == 4) g_gameExtraFlags |= 0x2;
        break;
    case 8:
        if (g_currDay == 8) g_gameExtraFlags |= 0x100;
        break;
    case 9:
        if (g_currDay == 8) g_gameExtraFlags |= 0x2;
        break;
    case 10:
        if (g_currDay == 5) g_gameExtraFlags |= 0x100;
        if (g_currDay == 14) g_gameExtraFlags |= 0x100;
        if (g_currDay == 22) g_gameExtraFlags |= 0x100;
        if (g_currDay == 31) g_gameExtraFlags |= 0x20;
        break;
    case 11:
        if (g_currDay == 5) g_gameExtraFlags |= 0x100;
        break;
    case 12:
        break;
    default:
        xFAIL_MSG_L(687, "What month is it?");
        break;
    }

    return FALSE;
}

static S32 g_flg_chEnabled;

inline void GEC_CheatFlagAdd(S32 flg) { g_flg_chEnabled |= flg; }
inline void GEC_CheatFlagToggle(S32 flg) { g_flg_chEnabled ^= flg; }

void zGameExtras_NewGameReset()
{
    g_flg_chEnabled = 0;
}

void zGameExtras_Save(xSerial* xser)
{
    S32 keepers;

    keepers = g_flg_chEnabled & 0xFFFF;
    xser->Write(keepers);
}

void zGameExtras_Load(xSerial* xser)
{
    S32 keepers;

    keepers = 0;
    xser->Read(&keepers);
    g_flg_chEnabled |= keepers;
}

static xTWEAKABLE F32 k_CHEATTIMER_START = 0.3f;
static F32 sCheatTimer;
static S32 sCheatInputCount;

U32 sCheatTestCheats[CHEAT_COUNT] = {
    0x40000, 0x20000, 0x40000, 0x20000, 0x20000, 0x40000, 0x20000, 0x20000,
    0x20000, 0x40000, 0x40000, 0x40000, 0x40000, 0x20000, 0x20000, 0x40000
};
U32 sCheatSwapCCLR[CHEAT_COUNT] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0x40000, 0x40000, 0x20000, 0x20000, 0x20000, 0x20000, 0x40000, 0x40000
};
U32 sCheatSwapCCUD[CHEAT_COUNT] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0x40000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x40000
};

struct GECheat
{
    U32* key_code;
    void (*fun_cheat)();
    S32 flg_keep;
    S32 flg_mode;
};

static void GEC_cb_TestCheats();
static void GEC_cb_SwapCCLR();
static void GEC_cb_SwapCCUD();

static GECheat cheatList[] = {
    { sCheatTestCheats, GEC_cb_TestCheats, 0, 1 },
    { sCheatSwapCCLR, GEC_cb_SwapCCLR, 0x200, 1 },
    { sCheatSwapCCUD, GEC_cb_SwapCCUD, 0x400, 1 },
    { NULL, NULL, 0, 0 }
};

U32 sCheatPressed[CHEAT_COUNT] = { 0 };

static S32 TestCheat(U32* cheat_info)
{
    S32 i = CHEAT_COUNT - 1, enable_cheat = TRUE;

    xASSERT_L(1013, cheat_info[i] != 0);
    if (cheat_info[i] == 0) return FALSE;
    while (i >= 0) {
        if (cheat_info[i] != sCheatPressed[i]) return FALSE;
        i--;
    }
    return enable_cheat;
}

static void AddToCheatPressed(U32 button)
{
    for (S32 i = 1; i < CHEAT_COUNT; i++) {
        sCheatPressed[i - 1] = sCheatPressed[i];
    }
    sCheatPressed[CHEAT_COUNT - 1] = button;
}

void zGameCheatsUpdate(F32 dt)
{
    if (GCN_DEMO_PARAM) {
        return;
    }

    if (zGameModeGet() != eGameMode_Pause) {
        return;
    }

#if DEBUG || RELEASE
    static S32 displayPresses = FALSE;
    if (displayPresses) {
        xprintf("\n\n\n\n");
        for (S32 i = 0; i < CHEAT_COUNT; i++) {
            switch (sCheatPressed[i]) {
            case 0x20000: xprintf("R"); break;
            case 0x40000: xprintf("L"); break;
            default: xprintf("-"); break;
            }
        }
        xprintf("       %.3f   %3d\n", sCheatTimer, sCheatInputCount);
    }
#endif

    S32 startover = FALSE, i = -1; xTRCPadGetPadPort(i, TRC_PadActive);
    if ((mPad[i].pressed & ~0x61100) || (mPad[i].on & ~0x61100))
        startover = TRUE;
    else if ((mPad[i].on & 0x1100) != 0x1100)
        startover = TRUE;
    else if (!(mPad[i].pressed & 0x60000)) {
        sCheatTimer -= dt;
        if (sCheatTimer <= 0.0f) startover = TRUE;
        else return;
    }

    if (!startover) {
        if (sCheatInputCount > 32) startover = TRUE;
        sCheatInputCount++;
    }

    if (startover) {
        sCheatTimer = k_CHEATTIMER_START;
        sCheatInputCount = 0;
        memset(sCheatPressed, 0, sizeof(sCheatPressed));
        return;
    }

    {
        S32 i = -1; xTRCPadGetPadPort(i, TRC_PadActive);
        AddToCheatPressed(mPad[i].pressed & 0x60000);
        sCheatTimer = k_CHEATTIMER_START;
    }

    S32 match = 0;
    GECheat* rec_next = cheatList;
    while (rec_next->key_code) {
        GECheat* rec_curr = rec_next;
        rec_next++;
        S32 rc = TestCheat(rec_curr->key_code);
        if (!rc) {
            continue;
        }
        match++;
        if (rec_curr->flg_mode & 1)
            GEC_CheatFlagToggle(rec_curr->flg_keep);
        else
            GEC_CheatFlagAdd(rec_curr->flg_keep);
        if (rec_curr->fun_cheat) rec_curr->fun_cheat();
    }

    if (match) {
        sCheatTimer = -1.0f;
        memset(sCheatPressed, 0, sizeof(sCheatPressed));
    }
}

static void GEC_dfltSound()
{
    iprintf("---> Cheat Triggered!\n");

    static U32 aid_sndList[] = {
        HASH_SBG01030,
        HASH_SBG01030,
        HASH_SBG01030,
        HASH_SBG01017_a,
        HASH_SBG01017_b,
        HASH_SBG01018,
        HASH_SBG01016
    };

    xUtil_choose(aid_sndList, ARRAY_LENGTH(aid_sndList), NULL);
}

static void GEC_cb_TestCheats()
{
    iprintf("Test Cheat Triggered!\n");
    GEC_dfltSound();
}

static void GEC_cb_SwapCCLR()
{
    iprintf("Camera Control Left/Right Cheat Triggered!\n");
    GEC_dfltSound();
}

static void GEC_cb_SwapCCUD()
{
    iprintf("Camera Control Up/Down Cheat Triggered!\n");
    GEC_dfltSound();
}
