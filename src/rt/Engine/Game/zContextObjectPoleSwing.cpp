#include "xCommon.h"
#include "zContextObjectPoleSwing.h"

#include "xColor.h"
#include "xDebugTweak.h"
#include "xDraw.h"
#include "xEntMotion.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xScene.h"
#include "zCommonPlayer.h"
#include "zScene.h"

namespace
{
    xTWEAKABLE F32 REMY_MOUNT_HEIGHT = 2.3f;
    xTWEAKABLE F32 DIR_COS = 0.5f;
    xTWEAKABLE F32 END_POINT_DIST = 2.0f;
    xTWEAKABLE F32 POLE_DIST = 2.0f;
    xTWEAKABLE F32 HAND_WIDTH = 0.75f;
    xTWEAKABLE bool debugDraw = false;
} // namespace

#if DEBUG || RELEASE
void zContextObjectPoleSwing::AddStaticTweaks()
{
    zContextObject::AddStaticTweaks();
    const char* prefix = "Player|Context Objects|Pole Swing|";
    xAUTOTWEAK(prefix, "REMY_MOUNT_HEIGHT", &REMY_MOUNT_HEIGHT, 0.0f, 10000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "Dir angle", &DIR_COS, 0.0f, 90.0f, &cb_tweak_convert_cos, NULL, 0, false);
    xAUTOTWEAK(prefix, "END_POINT_DIST", &END_POINT_DIST, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "POLE_DIST", &POLE_DIST, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "HAND_WIDTH", &HAND_WIDTH, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK_BOOL(prefix, "debugDraw", &debugDraw, NULL, NULL, 0, true);
}
#endif

zContextObjectPoleSwing::zContextObjectPoleSwing(zContextObjectPoleSwingAsset* theAsset)
{
    asset = theAsset;
    xBaseInit(this, asset);
}

void zContextObjectPoleSwing::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(60, asset, asset_size, zContextObjectPoleSwingAsset, "Context Object:Pole Swing");
    new (&data) zContextObjectPoleSwing((zContextObjectPoleSwingAsset*)&asset);
}

void zContextObjectPoleSwing::Setup()
{
    pole = (xEnt*)zSceneFindObject(asset->poleID);
    xASSERT_FMT_L(67, pole, "Pole missing for pole swing %s. About to crash.", xSceneID2Name(xglobals->sceneCur, id));

    if (asset->length > 0.0f) {
        length = asset->length;
    } else {
        xASSERT_FMT_L(75, pole->bound.type == k_XBOUNDTYPE_OBB,
                      "Pole object %s should have an object oriented bounding box for its bound",
                      xSceneID2Name(xglobals->sceneCur, pole->id));

        xBox& box = pole->bound.box.box;
        length = box.upper.z - box.lower.z;
    }
}

bool zContextObjectPoleSwing::CanMount(zCommonPlayer* player, MountData& mountData, F32& priority) const
{
    const xMat4x3& poleMat = pole->frame->mat;
    const xMat4x3& playerMat = player->frame->mat;

    xVec3 handPos = playerMat.pos;
    handPos.y += REMY_MOUNT_HEIGHT;

#if DEBUG || RELEASE
    if (debugDraw) {
        xDrawSetColor(g_PINK);
        xDrawSphere(&handPos, 0.3f, 0xC0006);
    }
#endif

    F32 dirDot = poleMat.right.dot(playerMat.at);
    bool forward = dirDot > 0.0f;

    xVec3 dir;
    dir.Sub(handPos, poleMat.pos);

    F32 projection = dir.dot(poleMat.at), distance2;
    bool valid = false;
    xVec3 grabPoint;

    if (projection > length - HAND_WIDTH || projection < HAND_WIDTH) {
        grabPoint = poleMat.pos;

        F32 dist = HAND_WIDTH;
        if (projection > HAND_WIDTH) {
            dist = length - HAND_WIDTH;
        }

        grabPoint.AddScale(poleMat.at, dist);
        distance2 = grabPoint.Distance2(handPos);
        valid = distance2 <= xsqr(END_POINT_DIST);
    } else {
        grabPoint = poleMat.pos;
        grabPoint.AddScale(poleMat.at, projection);
        dir.AddScale(poleMat.at, -projection);
        distance2 = dir.length2();
        valid = distance2 <= xsqr(POLE_DIST);
    }

#if DEBUG || RELEASE
    if (debugDraw) {
        xDrawSetColor(g_CYAN);
        xDrawLine(&handPos, &grabPoint);
    }
#endif

    F32 currPriority = GetPriority(distance2);
    if (!valid || currPriority > priority) {
        return false;
    }

    xVec3 diff;
    diff.Sub(grabPoint, handPos);

    priority = currPriority;
    mountData.poleSwing.pos = grabPoint;
    mountData.poleSwing.pos.y -= REMY_MOUNT_HEIGHT;
    mountData.poleSwing.forward = forward;
    return true;
}

bool zContextObjectPoleSwing::Mount(zCommonPlayer* player, MountData& mountData, F32 dt)
{
    bool match = MatchOrientation(player->frame, pole->frame->mat.right, dt, mountData.poleSwing.forward);
    return MoveTowardsMountPoint(player->frame, mountData.poleSwing.pos) && match;
}
