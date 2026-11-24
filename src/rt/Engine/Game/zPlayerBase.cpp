#include "xCommon.h"
#include "zPlayerBase.h"

#include "containers.h"
#include "xCam.h"
#include "xCutscene.h"
#include "xDebugTweak.h"
#include "xGlobals.h"
#include "xPtankPool.h"
#include "xSndMgr.h"
#include "xstransvc.h"
#include "xutil.h"
#include "zConfiguration.h"
#include "zCutsceneMgr.h"
#include "zGlobals.h"
#include "zPlayer.h"
#include "zScene.h"

#include "decomp.h"

static S32 sMemDepthJustHIPStartPlayer = -1;

void zPlayerUpdate(xEnt* ent, xScene* sc, F32 dt)
{
    static xTWEAKABLE F32 time_scale = 1.0f;
#if DEBUG || RELEASE
    xTEMPTWEAKS_BEGIN(prefix)
        {
            const char* prefix = "Player|";
            xAUTOTWEAK(prefix, "time_scale", &time_scale, 0.00001f, 100.0f, NULL, NULL, 0, true);
        }
    xTEMPTWEAKS_END
#endif

    if (time_scale != 1.0f) {
        dt *= time_scale;
    }

    zPlayer* player = (zPlayer*)ent;
    if (globals.cmgr && globals.cmgr->csn->Ready) {
        if (player->Get_cutsceneReady()) {
            return;
        }

        player->Set_cutsceneReady();
    } else {
        player->Clear_cutsceneReady();
    }

    player->sc = sc;
    player->BeginUpdate(dt);
    player->Update(dt);
    player->EndUpdate(dt);
}

void zPlayerRender(zEnt* ent)
{
    zPlayer* player = (zPlayer*)ent;
    if (!player->Get_cutsceneReady())
        player->Render();
}

void zPlayerReset(zEnt* ent)
{
    zPlayer* player = (zPlayer*)ent;
    player->Reset();
}

void zPlayerMove(xEnt* ent, xScene* s, F32 dt, xEntFrame* frame)
{
    zPlayer* player = (zPlayer*)ent;
    player->Move(s, dt, frame);
}

void zPlayerLoadHIP(U32 gameSceneID)
{
    S32 playerCount;
    U32 playersToLoad, tagPlayer;
    playerCount = zConfig_getPlayers(gameSceneID, &playersToLoad);

    globals.checkPoint.jsp_active = NULL;
    globals.firstPlayerBrainID = eBrainType_Unknown;
    sMemDepthJustHIPStartPlayer = xMemPushBase();

    bool playerHipLoaded[PLAYER_COUNT];
    for (S32 j = 0; j < PLAYER_COUNT; j++)
        playerHipLoaded[j] = false;

    for (S32 i = 0; i < zConfig_getPlayerCount(gameSceneID); i++) {
        tagPlayer = zConfig_getPlayerTag(gameSceneID, i);

        globals.playerTag[i] = tagPlayer;

        if (!playerHipLoaded[zConfig_getPlayerEnumIdx(tagPlayer)]) {
            bool result = false;
            result = zSceneLoadPackFile(tagPlayer, XST_OPTS_HIP, false, NULL);
            if (result) {
                playerHipLoaded[zConfig_getPlayerEnumIdx(tagPlayer)] = true;
            }
        }
    }
    globals.playerLoaded = true;
}

void zPlayerUnloadHIP()
{
    for (S32 i = 0; i < CURPLAYERS.size(); i++)
        CURPLAYERS[i]->Exit();

    for (S32 i = 0; i < PLAYER_COUNT; i++) {
        if (globals.playerTag[i]) {
            iprintf("Unloading Player File %s...\n", xUtil_idtag2string(globals.playerTag[i], 0));
            xSTUnLoadSceneMany(globals.playerTag[i], 0x1 | 0x4);
        }
    }

    xMemPopBase(sMemDepthJustHIPStartPlayer);
    sMemDepthJustHIPStartPlayer = -1;

    CURPLAYERS.empty();
    globals.playerLoaded = false;
}

void zPlayer_StoreCheckPoint(const zPlayerCheckPoint* checkPoints, U32 initialCameraID)
{
    globals.checkPoint.initCamID = initialCameraID;
    if (!globals.checkPoint.jsp_active) {
        globals.checkPoint.jsp_active = (U8*)xMEMALLOC_L(globals.sceneCur->env->geom->jsp_count, 0, eMemMgrTag_Player, 0, 195);
    }

    globals.checkPoint.currentEffect = xSndMgrGetEffect();

    for (S32 i = 0; i < CURPLAYERS.size(); i++) {
        CURPLAYERS[i]->StoreCheckPoint(checkPoints[i]);
    }
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE(
    "xEnt.h",
    "ent->model",
    "xModel.h",
    "model",
    "model->Mat"
)
#endif

void zPlayerSceneInit(U32 sceneID)
{
    if (zConfig_getPlayerEnum(sceneID, 0) == REMY) {
        globals.firstPlayerBrainID = eBrainType_Player_Remy;
    }
    else if (zConfig_getPlayerEnum(sceneID, 0) == HUMAN_VEHICLE) {
        globals.firstPlayerBrainID = eBrainType_Player_HumanVehicle;
    }

    for (S32 i = 0; i < CURPLAYERS.size(); i++) {
        globals.playerTag[i] = zConfig_getPlayerTag(sceneID, i);
    }
}

void zPlayerSceneExit()
{
    for (S32 i = 0; i < CURPLAYERS.size(); i++)
        CURPLAYERS[i]->Exit();
}
