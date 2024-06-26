#ifndef XGLOBALS_H
#define XGLOBALS_H

#include "xMath3.h"
#include "xUpdateCull.h"
#include "iFog.h"
#include "iTime.h"
#include "xPlaybackBuffers.h"
#include "zPlayerContainer.h"

#include <rpworld.h>

struct xCamGroup;
struct xCamScreen;
struct zScene;
struct xDebugLink;

enum sceDemoEndReason
{
    SCE_DEMO_ENDREASON_ATTRACT_INTERRUPTED,
    SCE_DEMO_ENDREASON_ATTRACT_COMPLETE,
    SCE_DEMO_ENDREASON_PLAYABLE_INACTIVITY_TIMEOUT,
    SCE_DEMO_ENDREASON_PLAYABLE_GAMEPLAY_TIMEOUT,
    SCE_DEMO_ENDREASON_PLAYABLE_COMPLETE,
    SCE_DEMO_ENDREASON_PLAYABLE_QUIT
};

struct PS2DemoGlobals
{
    U16 language;
    U16 aspect;
    U16 play_mode;
    U16 inactive_timeout;
    U16 gameplay_timeout;
    sceDemoEndReason exit_code;
    U32 FMV_playing : 1;
    U32 more_padding : 31;
    F32 bail_timer;
    S32 inactive_detect;
    F32 inactive_timer;
    F32 gameplay_timer;
    char subdir[16];
    U16 quit;
    U16 vmode;
};

struct xGlobals
{
    xCamGroup* cam;
    xCamScreen* screen;
    xVec4 frustplane[12];
    S32 profile;
    char profFunc[6][128];
    xUpdateCullMgr* updateMgr;
    S32 sceneFirst;
    char sceneStart[32];
    RpWorld* currWorld;
    iFogParams fog;
    iFogParams fogA;
    iFogParams fogB;
    iTime fog_t0;
    iTime fog_t1;
    S32 option_vibration_p1;
    S32 option_vibration_p2;
    S32 option_vibration_p1_menu;
    S32 option_vibration_p2_menu;
    S32 option_subtitles;
    U32 slowdown;
    F32 update_dt;
    U32 dumpCutscene;
#ifdef DEBUGRELEASE
    U32 PlaybackMode;
    PlaybackBuffers PlaybackFrames[4];
    char PlaybackFile[128];
    U32 PlaybackStartFrame;
    U32 PlaybackEndFrame;
    U32 PlaybackResolution;
    S32 MemTrackingLvl;
    char MemTrackLogFileName[128];
#endif
    S16 ForceCutscene;
    S32 useHIPHOP;
    bool NoMusic;
    bool NoCutscenes;
    bool NoPadCheck;
    bool firstStartPressed;
    bool fromLauncher;
    bool skipAssertWithController;
    bool enableHelperAI;
    bool enableHelperAIAttack;
    bool enableDebugControls;
    bool showReleaseMemInfo;
#ifdef DEBUGRELEASE
    bool PlaybackFlashDemo;
#endif
    bool FlashWIP;
    bool inLoadingScreen;
    bool LoadingScene;
    bool InitializingLoadingScreen;
    bool ForceMono;
    bool UnlimitedNukes;
    U32 minVSyncCnt;
    bool dontShowPadMessageDuringLoadingOrCutScene;
    bool autoSaveFeature;
    bool skipAsserts;
    bool beforeFirstFrame;
    S32 asyncLoadingScreen;
    S32 asyncLoadingFlags;
    char fromLauncherUser[32];
    zPlayerContainer players;
    zScene* sceneCur;
    zScene* scenePreload;
    PS2DemoGlobals* PS2demo;
    char watermark[127];
    U8 watermarkAlpha;
    F32 watermarkSize;
#ifdef DEBUGRELEASE
    xDebugLink* debugLink;
    bool enableRealTimeUpdate;
#endif
    bool showMenuOnBoot;
    bool enableHUD;
    bool skipAnimViewer;
};

extern xGlobals* xglobals;

#endif
