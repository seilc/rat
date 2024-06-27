#ifndef ZGLOBALS_H
#define ZGLOBALS_H

#include "xGlobals.h"
#include "xSndMgr.h"
#include "zBrainTypes.h"
#include "zMiniGameManager.h"

struct zGlobalSettings
{
    U16 AnalogMin;
    U16 AnalogMax;
    U32 TakeDamage;
    F32 DamageInvincibility;
    F32 Gravity;
    U8 AttractModeDuringGameplay;
#ifdef DEBUGRELEASE
    U32 AccelScripts;
#endif
    F32 CameraFOV;
};

struct zCheckPoint
{
    U32 initCamID;
    U8* jsp_active;
    xSndEffect currentEffect;
};

enum zGlobalDemoType
{
    zDT_NONE,
    zDT_E3,
    zDT_PUBLICITY,
    zDT_OPM,
    zDT_LAST
};

struct zAssetPickupTable;
struct zCutsceneMgr;
class zEconomics;

struct zGlobals : xGlobals
{
    zGlobalSettings settings;
    zCheckPoint checkPoint;
    U32 playerTag[9];
    bool playerLoaded;
    bool invertJoystick;
    F32 timeMultiplier;
    F32 timeMultiplierTarget;
    F32 XPMultiplier;
    zAssetPickupTable* pickupTable;
    zCutsceneMgr* cmgr;
    char startDebugMode[32];
    U32 noMovies;
    U32 boundUpdateTime;
    bool draw_player_after_fx;
    bool bAllowMasterCheats;
    bool enableFriendlyFly;
    bool stopCurrentConversation;
    zGlobalDemoType demoType;
    zCutsceneMgr* DisabledCutsceneDoneMgr;
    xVec3 cameraPlayersMidpoint;
    xVec3 cameraLookAt;
    S32 cameraAIFocus;
    eBrainType firstPlayerBrainID;
    zEconomics* economics;
    zMiniGameManager* mgManager;
    bool fmvJustFinished;
#ifdef DEBUGRELEASE
    U32 initialRemyPowerUp[5];
#endif
};

extern zGlobals globals;

#endif
