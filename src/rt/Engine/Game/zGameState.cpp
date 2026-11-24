#include "xCommon.h"
#include "zGameState.h"

#include "containers.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xPad.h"
#include "xserializer.h"
#include "xSndMgr.h"
#include "xTextureManager.h"
#include "xTRC.h"
#include "zBase.h"
#include "zEnt.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zMain.h"
#include "zScene.h"

S32 gGameState = eGameState_Dead;
eGameMode gGameMode;
eGameOstrich gGameOstrich;

static U32 sGameMode_DispatchTable[eGameMode_Count] = { 0 };

static U32 sGameState_DispatchTable[eGameState_Count] = {
    eEventDispatcher_GameState_FirstTime,
    eEventDispatcher_GameState_Play,
    eEventDispatcher_GameState_LoseChance,
    eEventDispatcher_GameState_GameOver,
    eEventDispatcher_GameState_SceneSwitch,
    eEventDispatcher_GameState_Dead
};

static U32 sPauseState_DispatchTable[ePauseState_Count] = {
    eEventDispatcher_PauseState_Pause,
    eEventDispatcher_PauseState_Options
};

static U32 sSaveState_DispatchTable[eSaveState_Count] = {
    eEventDispatcher_SaveState_SelectMemCard,
    eEventDispatcher_SaveState_SelectSlot,
    eEventDispatcher_SaveState_Saving
};

static U32 sOptionsState_DispatchTable[eOptionsState_Count] = {
    eEventDispatcher_OptionsState_Options
};

static U32 sLoadState_DispatchTable[eLoadState_Count] = {
    eEventDispatcher_LoadState_SelectMemCard,
    eEventDispatcher_LoadState_SelectSlot,
    eEventDispatcher_LoadState_Loading
};

static U32 sTitleState_DispatchTable[eTitleState_Count] = {
    eEventDispatcher_TitleState_Start,
    eEventDispatcher_TitleState_Attract
};

static U32 sIntroState_DispatchTable[eIntroState_Count] = {
    eEventDispatcher_IntroState_Sony,
    eEventDispatcher_IntroState_Publisher,
    eEventDispatcher_IntroState_Developer,
    eEventDispatcher_IntroState_License
};

static U32 sGameState_DoDispatchTable[eGameState_Count] = {
    eEventDispatcher_SetGameState_FirstTime,
    eEventDispatcher_SetGameState_Play,
    eEventDispatcher_SetGameState_LoseChance,
    eEventDispatcher_SetGameState_GameOver,
    eEventDispatcher_SetGameState_GameStats,
    eEventDispatcher_SetGameState_SceneSwitch,
    eEventDispatcher_SetGameState_Dead,
    eEventDispatcher_SetGameState_Exit
};

static U32 sPauseState_DoDispatchTable[ePauseState_Count] = {
    eEventDispatcher_SetPauseState_Pause,
    eEventDispatcher_SetPauseState_Options
};

static U32 sSaveState_DoDispatchTable[eSaveState_Count] = {
    eEventDispatcher_SetSaveState_SelectMemCard,
    eEventDispatcher_SetSaveState_SelectSlot,
    eEventDispatcher_SetSaveState_Saving
};

static U32 sOptionsState_DoDispatchTable[eOptionsState_Count] = {
    eEventDispatcher_SetOptionsState_Options
};

static U32 sLoadState_DoDispatchTable[eLoadState_Count] = {
    eEventDispatcher_SetLoadState_SelectMemCard,
    eEventDispatcher_SetLoadState_SelectSlot,
    eEventDispatcher_SetLoadState_Loading
};

static U32 sTitleState_DoDispatchTable[eTitleState_Count] = {
    eEventDispatcher_SetTitleState_Start,
    eEventDispatcher_SetTitleState_Attract
};

static U32 sIntroState_DoDispatchTable[eIntroState_Count] = {
    eEventDispatcher_SetIntroState_Sony,
    eEventDispatcher_SetIntroState_Publisher,
    eEventDispatcher_SetIntroState_Developer,
    eEventDispatcher_SetIntroState_License
};

S32 zGameStateGet()
{
    return gGameState;
}

S32 zGameModeGet()
{
    return gGameMode;
}

eGameOstrich zGameGetOstrich()
{
    return gGameOstrich;
}

void zGameSetOstrich(eGameOstrich o)
{
    gGameOstrich = o;
}

S32 zGameStateFindEvent(U32* array, S32 size, S32 mode, S32 event, S32* ret_mode, S32* ret_state)
{
    xASSERT_L(200, array);
    xASSERT_L(201, size >= 1);
    xASSERT_L(202, ret_mode);
    xASSERT_L(203, ret_state);

    for (S32 i = 0; i < size; i++) {
        if (event == array[i]) {
            *ret_mode = mode;
            *ret_state = i;
            return TRUE;
        }
    }

    return FALSE;
}

void zGameStateSwitchEvent(S32 event)
{
    S32 cur_mode = zGameModeGet();
    S32 cur_state = zGameStateGet();
    S32 new_mode = -1;
    S32 new_state = -1;

    if (!zGameStateFindEvent(sGameState_DoDispatchTable, eGameState_Count, eGameMode_Game, event, &new_mode, &new_state)) {
        if (!zGameStateFindEvent(sPauseState_DoDispatchTable, ePauseState_Count, eGameMode_Pause, event, &new_mode, &new_state)) {
            if (!zGameStateFindEvent(sSaveState_DoDispatchTable, eSaveState_Count, eGameMode_Save, event, &new_mode, &new_state)) {
                if (!zGameStateFindEvent(sOptionsState_DoDispatchTable, eOptionsState_Count, eGameMode_Options, event, &new_mode, &new_state)) {
                    if (!zGameStateFindEvent(sLoadState_DoDispatchTable, eLoadState_Count, eGameMode_Load, event, &new_mode, &new_state)) {
                        if (!zGameStateFindEvent(sTitleState_DoDispatchTable, eTitleState_Count, eGameMode_Title, event, &new_mode, &new_state)) {
                            zGameStateFindEvent(sIntroState_DoDispatchTable, eIntroState_Count, eGameMode_Intro, event, &new_mode, &new_state);
                        }
                    }
                }
            }
        }
    }

    if (new_mode != cur_mode) {
        zGameModeSwitch((eGameMode)new_mode);
    }

    if (new_mode != cur_mode || new_state != cur_state) {
        zGameStateSwitch(new_state);
        if (new_state == eGameState_Exit) {
            xSerialWipeMainBuffer();
            if (IsPS2Demo()) {
                xglobals->PS2demo->quit = 1;
            }
        }

        S32 startidx = 0;
        while (xTRCPadFindFirstPadState(startidx, TRC_PadStatePresent)) {
            xPad* pad;
            S32 port = xTRCPadGetPadPort(startidx);
            TRCState state = xTRCPadGetStateIdx(startidx);
            pad = &mPad[port];
            pad->pressed = 0;
            startidx++;
        }
    }
}

void zGameStateSwitch(S32 theNewState)
{
    S32 stateOld;
    U32 theEvent = 0;

    stateOld = gGameState;
    gGameState = theNewState;

    if (theNewState == eGameState_Play && stateOld == eGameState_FirstTime && zGameModeGet() != eGameMode_Title) {
        xTextureManager::Get().FreeMemory();
    }

    switch (gGameMode) {
    case eGameMode_Intro:
        theEvent = sIntroState_DispatchTable[theNewState];
        break;
    case eGameMode_Title:
        theEvent = sTitleState_DispatchTable[theNewState];
        break;
    case eGameMode_Load:
        theEvent = sLoadState_DispatchTable[theNewState];
        break;
    case eGameMode_Options:
        theEvent = sOptionsState_DispatchTable[theNewState];
        break;
    case eGameMode_Save:
        theEvent = sSaveState_DispatchTable[theNewState];
        break;
    case eGameMode_Pause:
        theEvent = sPauseState_DispatchTable[theNewState];
        break;
    case eGameMode_Game:
        theEvent = sGameState_DispatchTable[theNewState];
        break;
    case eGameMode_Boot:
    case eGameMode_Start:
    case eGameMode_WorldMap:
    case eGameMode_MonsterGallery:
    case eGameMode_ConceptArtGallery:
        break;
    }

    if (theEvent)
        zEntEventAllOfType(theEvent, eBaseTypeDispatcher);
}

void zGameModeSwitch(eGameMode modeNew)
{
    xASSERT_L(340, modeNew > eGameMode_Boot);
    xASSERT_L(341, modeNew < eGameMode_Count);

    eGameMode modeOld = gGameMode;

    xASSERT_L(346, modeOld != modeNew);

    if ((modeOld == eGameMode_Game && modeNew == eGameMode_Pause) ||
        (modeOld == eGameMode_Game && modeNew == eGameMode_Save)) {
        xSndMgrPauseSounds(xSndCat_CUTSCENE, true, true);
        xSndMgrPauseSounds(xSndCat_DIALOG, true, false);
        xSndMgrPauseSounds(xSndCat_DIALOG_3D, true, true);
        xSndMgrPauseSounds(xSndCat_GAME_2D, true, true);
        xSndMgrPauseSounds(xSndCat_GAME_3D, true, true);
        xSndMgrPauseSounds(xSndCat_MUSIC, true, false);
    }
    else if ((modeOld == eGameMode_Pause || modeOld == eGameMode_Save || modeOld == eGameMode_Load) &&
             modeNew == eGameMode_Game) {
        if (globals.sceneCur->sceneID != SCENE_ID_MNU_START) {
            xSndMgrStopSounds(xSndCat_DIALOG, true, false);
        }

        if (globals.sceneCur->pendingPortal == NULL) {
            xSndMgrPauseSounds(xSndCat_CUTSCENE, false, true);
            xSndMgrPauseSounds(xSndCat_DIALOG, false, true);
            xSndMgrPauseSounds(xSndCat_DIALOG_3D, false, true);
            xSndMgrPauseSounds(xSndCat_GAME_2D, false, true);
            xSndMgrPauseSounds(xSndCat_GAME_3D, false, true);
            xSndMgrPauseSounds(xSndCat_MUSIC, false, true);
        }
    }

    switch (modeOld) {
    case eGameMode_Boot:
        break;
    case eGameMode_Intro:
        break;
    case eGameMode_Title:
        break;
    case eGameMode_Start:
        break;
    case eGameMode_Load:
        break;
    case eGameMode_Options:
        break;
    case eGameMode_Save:
        break;
    case eGameMode_Pause:
        break;
    case eGameMode_WorldMap:
        break;
    case eGameMode_MonsterGallery:
        break;
    case eGameMode_ConceptArtGallery:
        break;
    case eGameMode_Game:
        break;
    }

    gGameMode = modeNew;

    switch (modeNew) {
    case eGameMode_Boot:
        break;
    case eGameMode_Intro:
        break;
    case eGameMode_Title:
        break;
    case eGameMode_Start:
        break;
    case eGameMode_Load:
        break;
    case eGameMode_Options:
        break;
    case eGameMode_Save:
        break;
    case eGameMode_Pause:
        break;
    case eGameMode_WorldMap:
        break;
    case eGameMode_MonsterGallery:
        break;
    case eGameMode_ConceptArtGallery:
        break;
    case eGameMode_Game:
        break;
    }

    zEntEventAllOfType(sGameMode_DispatchTable[modeNew], eBaseTypeDispatcher);
}
