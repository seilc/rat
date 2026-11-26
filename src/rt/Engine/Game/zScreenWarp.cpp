#include "xCommon.h"
#include "zScreenWarp.h"

#include "containers.h"
#include "xCam.h"
#include "xDebugTweak.h"
#include "xDraw.h"
#include "xDynAsset.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xScene.h"
#include "xScreenWarp.h"
#include "zBase.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zScene.h"

#include <stdio.h>

struct zScreenWarpAsset : xDynAsset
{
    enum { VERSION = 1 };

    U32 effectType;
    U32 flags;
    xVec3 pos;
    F32 radius;
    F32 duration;
    F32 repeatDelay;
    F32 intensity;
    F32 frequency;
    F32 triggerDistance;

    static const char* type_name() { return "Effect:Screen Warp"; }
};

namespace {

zScreenWarp* sWarpList;

#if DEBUG || RELEASE
const char* sWarpTypeLabels[xScreenWarp::MAX_TYPE] = {
    "WARBLE",
    "PULSING LENS",
    "RIPPLE",
    "SHOCKWAVE",
    "HEAT SHIMMER",
    "STATIC LENS"
};
#endif

const U32 sWarpTypeValues[xScreenWarp::MAX_TYPE] = {
    xScreenWarp::TYPE_WARBLE,
    xScreenWarp::TYPE_LENS,
    xScreenWarp::TYPE_RIPPLE,
    xScreenWarp::TYPE_SHOCKWAVE,
    xScreenWarp::TYPE_HEAT_SHIMMER,
    xScreenWarp::TYPE_STATIC_LENS
};

} // namespace

#if DEBUG || RELEASE
void zScreenWarp::addDebugTweaks()
{
    char prefix[128];
    sprintf(prefix, "Widgets|Screen Warp|%s|", xSceneID2Name(xglobals->sceneCur, asset->id));

    xAUTOTWEAK_SELECT(prefix, "asset->effectType", &asset->effectType, sWarpTypeLabels, sWarpTypeValues, xScreenWarp::MAX_TYPE, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "asset->pos", &asset->pos, 0, 0, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "asset->radius", &asset->radius, 0.0f, 1000000000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "asset->duration", &asset->duration, 0.0f, 1000000000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "asset->repeatDelay", &asset->repeatDelay, 0.0f, 1000000000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "asset->intensity", &asset->intensity, 0.0f, 1000000000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "asset->frequency", &asset->frequency, 0.0f, 1000000000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "asset->triggerDistance", &asset->triggerDistance, 0.0f, 1000000000.0f, NULL, NULL, 0, true);
    xAUTOTWEAK_BOOL(prefix, "renderDebugSphere", &renderDebugSphere, NULL, NULL, 0, true);

    static const tweak_callback cb_enable = { on_enable };
    xAUTOTWEAK_FLAG(prefix, "Enabled", &flags, SW_FLAG_ENABLED, &cb_enable, this, 0);
    xAUTOTWEAK_FLAG(prefix, "Running", &flags, SW_FLAG_RUNNING, NULL, NULL, 1);
}

void zScreenWarp::on_enable(const tweak_info& info)
{
    zScreenWarp* sw = (zScreenWarp*)info.context;
    sw->Enable((sw->flags & SW_FLAG_ENABLED) ? true : false);
}
#endif

void zScreenWarp::SceneEnter()
{
    sWarpList = NULL;
}

void zScreenWarp::ResetAll()
{
    zScreenWarp* sw = sWarpList;
    while (sw) {
        sw->Reset();
        sw = sw->next;
    }
}

void zScreenWarp::UpdateAll(F32 dt)
{
    zScreenWarp* sw = sWarpList;
    while (sw) {
        sw->Update(dt);
        sw = sw->next;
    }
}

#if DEBUG || RELEASE
void zScreenWarp::RenderAll()
{
    zScreenWarp* sw = sWarpList;
    while (sw) {
        sw->Render();
        sw = sw->next;
    }
}
#endif

void zScreenWarp::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(141, asset, asset_size, zScreenWarpAsset, zScreenWarpAsset::type_name());
    xBaseInit(&data, &asset);

    zScreenWarp* sw = (zScreenWarp*)&data;
    sw->asset = (zScreenWarpAsset*)&asset;
    sw->eventFunc = EventCB;
    sw->Reset();
    sw->addDebugTweaks();

    sw->next = sWarpList;
    sWarpList = sw;
}

void zScreenWarp::Reset()
{
    timer = 0.0f;
    flags = 0;
    Enable((asset->flags & SW_FLAG_ENABLED) ? true : false);
}

void zScreenWarp::Enable(bool bEnable)
{
    if (bEnable)
        flags |= SW_FLAG_ENABLED;
    else
        flags &= ~(SW_FLAG_ENABLED | SW_FLAG_RUNNING);
}

void zScreenWarp::Update(F32 dt)
{
    UpdateState();

    if (!(flags & SW_FLAG_RUNNING)) {
        return;
    }

    timer -= dt;
    if (timer > 0.0f) {
        return;
    }

    timer = asset->repeatDelay;
    xScreenWarp::emit(asset->pos, asset->radius, asset->duration, (xScreenWarp::type_enum)asset->effectType, asset->intensity, asset->frequency, 0);
}

void zScreenWarp::UpdateState()
{
    if (!(flags & SW_FLAG_ENABLED)) {
        return;
    }

    const xVec3* camPos = &globals.cam->mat.pos;
    F32 dist = xVec3DistFast(camPos, &asset->pos);

    bool bRunning = (flags & SW_FLAG_RUNNING) ? true : false;
    if (bRunning && dist > asset->triggerDistance) {
        flags &= ~SW_FLAG_RUNNING;
    }
    else if (!bRunning && dist < asset->triggerDistance) {
        flags |= SW_FLAG_RUNNING;
        timer = 0.0f;
    }
}

#if DEBUG || RELEASE
void zScreenWarp::Render()
{
    if (renderDebugSphere)
        xDrawSphere(&asset->pos, asset->radius, 0xC0006);
}
#endif

void zScreenWarp::EventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    xASSERT_L(218, to);
    xASSERT_L(219, to->baseType == eBaseTypeScreenWarp);

    zScreenWarp* sw = (zScreenWarp*)to;
    switch (toEvent) {
    case eEventOn:
        sw->Enable(true);
        break;
    case eEventOff:
        sw->Enable(false);
        break;
    case eEventReset:
    case eEventDebugReset:
        sw->Reset();
        break;
    }
}
