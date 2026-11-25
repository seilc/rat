#include "xCommon.h"
#include "zUICustom.h"

#include "xEvent.h"
#include "xString.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zHashes.h"
#include "zPlayer.h"
#include "zScene.h"
#include "zUI.h"
#include "zUICustomArtworkManager.h"
#include "zUICustomCheatsManager.h"
#include "zUICustomCheckpoints.h"
#include "zUICustomConditionallyDisabled.h"
#include "zUICustomControllerManager.h"
#include "zUICustomControlOverlay.h"
#include "zUICustomImageSlideshow.h"
#include "zUICustomJoinScreenManager.h"
#include "zUICustomMissionCompleteManager.h"
#include "zUICustomMissionSelect.h"
#include "zUICustomOptionsManager.h"
#include "zUICustomPauseScreenManager.h"
#include "zUICustomPowerEffect.h"
#include "zUICustomSaveLoad.h"
#include "zUICustomSceneImage.h"
#include "zUICustomScrollers.h"
#include "zUICustomSecretsScreenControl.h"
#include "zUICustomSoundMode.h"
#include "zUICustomStatic.h"
#include "zUICustomStatsScreen.h"
#include "zUICustomSysMessage.h"
#include "zUICustomUpgrade.h"
#include "zUICustomVolume.h"
#include "zUICustomWireframe.h"

void zUICustom::Init()
{
}

void zUICustom::Setup()
{
}

void zUICustom::Reset()
{
}

void zUICustom::ResetMotion()
{
}

bool zUICustom::HandleEvent(xBase*, U32, const F32*, xBase*, U32)
{
    return true;
}

void zUICustom::PreUpdate(F32)
{
}

void zUICustom::PostUpdate(F32)
{
}

bool zUICustom::PreRender()
{
    return true;
}

bool zUICustom::PreAutoMenu()
{
    return true;
}

void zUICustom::PostRender()
{
}

void zUICustom::Exit()
{
}

#if DEBUG || RELEASE
void zUICustom::AddTweaks(const char*)
{
}
#endif

void zUICustom::PreInitMotion()
{
}

void zUICustom::PostInitMotion()
{
}

void zUICustom::PreApplyMotionFrame(zUIMotionFrame*)
{
}

void zUICustom::PostApplyMotionFrame(zUIMotionFrame*)
{
}

void* zUICustom::operator new(size_t size)
{
    return zUIAllocStaticMemory(size);
}

zUICustom* zUICustomAllocate(U32 hashID, zUI* ui)
{
    switch (hashID) {
    case HASH_NumberScroller:
        return new zUICustomNumberScroller(ui);
    case HASH_Volume_Music_:
        return pUICustomMusicVolumeManager = new zUICustomVolume(ui, xSndMixerFader_MUSIC_GROUP);
    case HASH_Wireframe:
        return globals.InitializingLoadingScreen ? new zUICustomWireframe(ui) : NULL;
    case HASH_DescriptionScroller:
        return new zUICustomDescriptionScroller(ui);
    case HASH_SoundMode:
        return pUICustomSoundMode = new zUICustomSoundMode(ui);
    case HASH_SceneImage:
        return new zUICustomSceneImage(ui);
    case HASH_Static:
        return new zUICustomStatic(ui);
    case HASH_ControlOverlay:
        return new zUICustomControlOverlay(ui);
    case HASH_Checkpoints:
        return new zUICustomCheckpoints(ui);
    case HASH_MapScroller:
        return new zUICustomMapScroller(ui);
    case HASH_PauseScreenManager:
        return new zUICustomPauseScreenManager(ui);
    case HASH_SysMsgManager:
        return new zUICustomSysMessageManager(ui);
    case HASH_PowerEffect:
        return new zUICustomPowerEffect(ui);
    case HASH_ArtworkManager:
        return pUICustomArtworkManager = new zUICustomArtworkManager(ui);
    case HASH_Volume_SFX_:
        return pUICustomSFXVolumeManager = new zUICustomVolume(ui, xSndMixerFader_SFX_GROUP);
    case HASH_SecretsScreenControl:
        return new zUICustomSecretsScreenControl(ui);
    case HASH_SlideshowImage:
        return new zUICustomImageSlideshow(ui);
    case HASH_ConditionallyDisabled:
        return new zUICustomConditionallyDisabled(ui);
    case HASH_Upgrade:
        return pUICustomUpgradeManager = new zUICustomUpgrade(ui);
    case HASH_MissionSelect:
        return new zUICustomMissionSelect(ui);
    case HASH_StatsManager:
        return new zUICustomStatsScreen(ui);
    case HASH_ParrBest_Manager:
        return pUICustomParrBestScreen = new zUICustomParrBestScreen(ui);
    case HASH_JoinScreenManager:
        return pUICustomDropJoinSceneManager = new zUICustomJoinScreenManager(ui);
    case HASH_UI_SaveLoadManager:
        return pUICustomSaveLoadManager = new zUICustomSaveLoadManager(ui);
    case HASH_UI_ControllerManager:
        return pUICustomControllerManager = new zUICustomControllerManager(ui);
    case HASH_UI_OptionsManager:
        return new zUICustomOptionsManager(ui);
    case HASH_UI_MissionCompleteManager:
        return new zUICustomMissionCompleteManager(ui);
    case HASH_UI_CheatManager:
        return pUICustomCheatsManager = new zUICustomCheatsManager(ui);
    }

    return NULL;
}

namespace {

bool zUIIsMNUS(); // UNUSED

} // namespace

void zSendUIEvent(const char* widgetName, U32 event)
{
    xASSERT_L(266, widgetName);
    U32 id = xStrHash(widgetName);
    xASSERT_L(268, id);
    xBase* sendTo = zSceneFindObject(id);
    xASSERT_L(270, sendTo);
    zEntEvent(sendTo, event);
}

void zUIAutoScreenManagerFocusOnSelect()
{
    zSendUIEvent("UI PREV SCREEN HANDLER", eEventUIFocusOn_Select);
    zSendUIEvent("UI SCREEN MANAGER", eEventUIFocusOn_Select);
    zSendUIEvent("UI SCREEN MANAGER FADE HELPER", eEventUIFocusOn_Select);
}

void zUIAutoScreenManagerFocusOffUnselect()
{
    zSendUIEvent("UI PREV SCREEN HANDLER", eEventUIFocusOff_Unselect);
    zSendUIEvent("UI SCREEN MANAGER", eEventUIFocusOff_Unselect);
    zSendUIEvent("UI SCREEN MANAGER FADE HELPER", eEventUIFocusOff_Unselect);
}
