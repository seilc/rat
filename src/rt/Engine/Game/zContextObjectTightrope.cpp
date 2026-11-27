#include "xCommon.h"
#include "zContextObjectTightrope.h"

#include "xColor.h"
#include "xDebugTweak.h"
#include "xDraw.h"
#include "xEntMotion.h"
#include "xFX.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xScene.h"
#include "xSplineAsset.h"
#include "xstransvc.h"
#include "zCommonPlayer.h"
#include "zCutsceneMgr.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zShrapnel.h"
#include "zSurface.h"

namespace
{
    xTWEAKABLE F32 MAGNETIZE_DIST = 10.0f;
    xTWEAKABLE F32 LOOK_AHEAD_TIME = 0.5f;
    xTWEAKABLE U32 LOOK_AHEAD_POINTS = 10;
    xTWEAKABLE F32 PREDICT_DIST = 1.5f;
    xTWEAKABLE F32 PREDICT_Y_SNAP = 0.5f;
    xTWEAKABLE F32 START_CHECK_TIME = 0.5f;
    xTWEAKABLE bool drawDebugChecks = false;
} // namespace

void zContextObjectTightrope::Draw() const
{
    if (asset->NoRender()) {
        return;
    }

    xASSERT_L(46, curve);

    xBox box;
    curve->getBoundBox(box);

    RwCamera* cam = RwCameraGetCurrentCamera();
    RwMatrix* rwmatrix = RwFrameGetMatrix(RwCameraGetFrame(cam));

    xVec3 camPos;
    xVec3Copy(&camPos, (const xVec3*)&rwmatrix->pos);
    F32 d1 = camPos.Distance2(box.upper);
    F32 d2 = camPos.Distance2(box.lower);

    F32 lowest = d1;
    if (d2 < lowest)
        lowest = d2;
    F32 lodDistance = xsqrt(lowest);

    RwReal farClip = RwCameraGetFarClipPlane(cam);

    F32 t = lodDistance / farClip;
    if (t > 1.0f)
        t = 1.0f;
    t = 1.0f - t;

    S32 increments = 50.0f * t;
    if (increments > 50)
        increments = 50;
    else if (increments < 5)
        increments = 5;

    curve->DrawCrossSection(increments, 0.2f * t);
}

#if DEBUG || RELEASE
void zContextObjectTightrope::AddStaticTweaks()
{
    zContextObject::AddStaticTweaks();
    const char* prefix = "Player|Context Objects|Tightrope|";
    xAUTOTWEAK(prefix, "MAGNETIZE_DIST", &MAGNETIZE_DIST, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "LOOK_AHEAD_TIME", &LOOK_AHEAD_TIME, 0.0f, 3.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "LOOK_AHEAD_POINTS", &LOOK_AHEAD_POINTS, 0, 20, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "PREDICT_DIST", &PREDICT_DIST, 0.0f, 1000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK_BOOL(prefix, "drawDebugChecks", &drawDebugChecks, NULL, NULL, 0, true);
}
#endif

zContextObjectTightrope::zContextObjectTightrope(zContextObjectTightropeAsset* theAsset)
{
    asset = theAsset;
    xBaseInit(this, asset);
    curve = (const xNurbs*)xSTFindAsset(asset->curveID, NULL);
    xASSERT_FMT_L(111, curve, "Tightrope object %s cannot find its curve %s (0x%08x)", xSceneID2Name(xglobals->sceneCur, id), xSceneID2Name(xglobals->sceneCur, asset->curveID), asset->curveID);

#if DEBUG || RELEASE
    if (!curve) return;
#endif

    curve->getBoundBox(bound);

    bound.lower.x -= MAGNETIZE_DIST;
    bound.lower.y -= MAGNETIZE_DIST;
    bound.lower.z -= MAGNETIZE_DIST;
    bound.upper.x += MAGNETIZE_DIST;
    bound.upper.y += MAGNETIZE_DIST;
    bound.upper.z += MAGNETIZE_DIST;
}

void zContextObjectTightrope::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(128, asset, asset_size, zContextObjectTightropeAsset, "Context Object:Tightrope");
    new (&data) zContextObjectTightrope((zContextObjectTightropeAsset*)&asset);
}

void zContextObjectTightrope::Setup()
{
    if (asset->objectID) {
        object = (xEnt*)zSceneFindObject(asset->objectID);
        xASSERT_FMT_L(137, object, "Object id 0x%08x specified, but not found in scene", asset->objectID);
    } else {
        object = NULL;
    }
}

bool zContextObjectTightrope::Advance(F32 dist, F32& u, bool forward) const
{
    return !curve->advance_u(u, dist, forward, u);
}

bool zContextObjectTightrope::CanMount(zCommonPlayer* player, MountData& mountData, F32& priority) const
{
    const xVec3& dpos = player->frame->dpos;
    const xVec3& pos = player->frame->mat.pos;
    if (dpos.y > 0.0f || player->Get_gravityDisabled()) {
        return false;
    }

    xIsect intersectData;
    iBoxIsectVec(&bound, &pos, &intersectData);
    if (intersectData.penned != -1.0f) {
        return false;
    }

    F32 dposLength2 = dpos.length2();
    if (xfeq0(dposLength2)) {
        return false;
    }

    F32 curvePosU;
    xVec3 curvePos = curve->project_point(pos, &curvePosU, NULL, HUGE);
    xDrawSetColor(g_RED);
    xDrawSphere(&curvePos, 0.3f, 0xC0006);
    if (pos.y < curvePos.y) {
        return false;
    }

    F32 magnetizeDistSqr = xsqr(MAGNETIZE_DIST);
    F32 currDistance2 = pos.Distance2(curvePos);
    if (currDistance2 > magnetizeDistSqr) {
        return false;
    }

    F32 currPriority = GetPriority(currDistance2);
    if (currDistance2 < xsqr(PREDICT_DIST) && currPriority < priority &&
        player->frame->mat.pos.y - curvePos.y < PREDICT_Y_SNAP) {
        mountData.tightrope.u = curvePosU;
        priority = currPriority;
        return true;
    }

    F32 xzVel = player->GetPredictedVelocityMag();
    F32 a = 0.5f * -globals.sceneCur->gravity;
    F32 b = xsqrt(xsqr(xzVel) + xsqr(player->frame->vel.y));

    bool validEstimate = false;
    F32 timeAbove = 0.0f;
    F32 timeBelow = -1.0f;
    F32 checkTime = START_CHECK_TIME;

    for (U32 i = 0; i < LOOK_AHEAD_POINTS; i++) {
        xVec3 predictPos;
        player->Predict(&predictPos, checkTime, 1.0f, true, true);

        curvePos = curve->project_point(predictPos, &curvePosU, NULL, HUGE);

        F32 currDistance2 = curvePos.Distance2(predictPos);
        currPriority = GetPriority(currDistance2);
        if (currDistance2 <= xsqr(PREDICT_DIST) && currPriority <= priority &&
            xeq(predictPos.y, curvePos.y, PREDICT_Y_SNAP)) {
            validEstimate = true;
            mountData.tightrope.u = curvePosU;
            priority = currPriority;
        }

        if (predictPos.y > curvePos.y) {
            if (timeBelow < 0.0f) {
                checkTime *= 2.0f;
            } else {
                timeAbove = checkTime;
                checkTime = (timeAbove + timeBelow) / 2.0f;
            }
        }
        else if (predictPos.y < curvePos.y) {
            timeBelow = checkTime;
            checkTime = (timeAbove + timeBelow) / 2.0f;
        } else {
            break;
        }

#if DEBUG || RELEASE
        if (drawDebugChecks) {
            static xColor colors[] = { g_NEON_GREEN, g_NEON_BLUE, g_PEACH, g_FUSCHIA, g_MAROON,
                                       g_MIDNIGHTBLUE, g_SEAGREEN, g_FORESTGREEN, g_KHAKI, g_LAVENDER };

            xDrawSetColor(colors[i % ARRAY_LENGTH(colors)]);
            xDrawMarker(&curvePos, 0.5f, 1);
            xDrawSphere(&predictPos, 0.5f, 0xC0006);
        }
#endif
    }

    xASSERT_FMT_L(281, timeBelow > 0.0f, "Crap! we never guessed below the tightrope in our estimation");

    return validEstimate;
}

bool zContextObjectTightrope::Mount(zCommonPlayer* player, MountData& mountData, F32)
{
    xVec3 pos;
    curve->evaluate(mountData.tightrope.u, pos);
    return MoveTowardsMountPoint(player->frame, pos);
}

U8 zContextObjectTightrope::ClearChkby() const
{
    if (object) {
        U8 prevVal = object->chkby;
        object->chkby = 0;
        return prevVal;
    }

    return 0;
}

void zContextObjectTightrope::RestoreChkby(U8 prevValue) const
{
    if (object) {
        object->chkby = prevValue;
    }
}
