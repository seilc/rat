#include "xCommon.h"
#include "zMiniGamePlayer.h"

#include "containers.h"
#include "xCam.h"
#include "xPTankPool.h"
#include "xTRC.h"
#include "zGlobals.h"
#include "zMiniGameManager.h"
#include "zScene.h"

namespace
{
    S32 sFirstPlayerPadPort;
}

bool zMiniGamePlayer::IsActionButtonPressed()
{
    return GetPlayerInput().pressed.jump;
}

bool zMiniGamePlayer::IsActionButtonReleased()
{
    return GetPlayerInput().released.jump;
}

void zMiniGamePlayer::Init(xEntAsset* asset)
{
    zPlayer::Init(asset);

    activePickupTimer = 0.0f;
    activePickupType = -1;

    S32 playerCount = globals.mgManager->GetUserSetPlayerCount();
    isAI = index >= playerCount;

    if (!isAI && pad) {
        sFirstPlayerPadPort = pad->GetPadPort();
    }

    if (!isAI && !pad) {
        pad = zPad::GetUnusedPad(false);

        if (!pad) {
            for (S32 i = 0; i < k_MAX_TRC_PADS; i++) {
                if (xTRCPadGetPadStatePort(i) == TRC_PadStatePresent && xTRCPadGetStatePort(i) != TRC_PadActive) {
                    xTRCPadSetStatePadPort(i, TRC_PadActive);
                    pad = zPad::GetUnusedPad(false);
                    break;
                }
            }
        }

        if (pad)
            playerInput.SetPad(pad);
        else
            isAI = true;
    }
}

void zMiniGamePlayer::SceneSetup()
{
    zPlayer::SceneSetup();
}

void zMiniGamePlayer::Reset()
{
    zPlayer::Reset();

    activePickupTimer = 0.0f;
    activePickupType = -1;
}

void zMiniGamePlayer::BeginUpdate(F32 dt)
{
    zPlayer::BeginUpdate(dt);
}

void zMiniGamePlayer::Update(F32 dt)
{
    if (activePickupTimer > 0.0f) {
        activePickupTimer -= dt;
        if (activePickupTimer < 0.0f) {
            activePickupTimer = 0.0f;
            activePickupType = -1;
        }
    }
}

void zMiniGamePlayer::EndUpdate(F32 dt)
{
    zPlayer::EndUpdate(dt);
}

void zMiniGamePlayer::SceneExit(zScene* scene)
{
    S32 port = -1;
    if (pad) {
        port = pad->GetPadPort();
    }

    if (port != -1 && port != sFirstPlayerPadPort) {
        if (xTRCPadGetPadStatePort(port) == TRC_PadStatePresent && xTRCPadGetStatePort(port) == TRC_PadActive) {
            xTRCPadSetStatePadPort(port, TRC_PadInserted);
        }
    }

    zPlayer::SceneExit(scene);
}
