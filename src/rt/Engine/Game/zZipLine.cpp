#include "xCommon.h"
#include "zZipLine.h"

#include "xGlobals.h"
#include "xScene.h"
#include "xstransvc.h"
#include "zEvent.h"
#include "zScene.h"

#include "decomp.h"

void zZipLineInit(void* plat, void* asset)
{
    zZipLineInit((zZipLine*)plat, (zZipLineAsset*)asset);
}

void zZipLineInit(zZipLine* zipline, zZipLineAsset* asset)
{
    xBaseInit(zipline, asset);

    zipline->asset = asset;
    zipline->eventFunc = zZipLineEventCB;

    zipline->link = (xLinkAsset*)(asset + 1);

    zipline->enabled = true;
    zipline->gotDismount = false;
}

void zZipLineSetup(zZipLine* zipline, zScene*)
{
    if (zipline->asset->boundEntityID) {
        U32 id = zipline->asset->boundEntityID;
        zipline->asset->boundEntity = (xEnt*)zSceneFindObject(id);
        xASSERT_FMT_L(52, zipline->asset->boundEntity, "Can't find zipline's bound Entity with id 0x%08x", id);
    }

    xASSERT_FMT_L(54, zipline->asset->startSplineID, "Zipline %s exists without a spline ID", xSceneID2Name(xglobals->sceneCur, zipline->id));
    U32 id = zipline->asset->startSplineID;
    zipline->asset->spline = (xNurbs*)xSTFindAsset(id, NULL);
    xASSERT_FMT_L(57, zipline->asset->spline, "Can't find zipline's spline with id 0x%08x", id);
}

void zZipLineEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    zZipLine* zipline = (zZipLine*)to;

    switch (toEvent) {
    case eEventOn:
        zipline->enabled = true;
        break;

    case eEventOff:
        zipline->enabled = false;
        break;

    case eEventZipLineDismount:
        zipline->gotDismount = true;
        break;
    }
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("speed_turn")
#endif
