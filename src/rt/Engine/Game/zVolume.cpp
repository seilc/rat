#include "xCommon.h"
#include "zVolume.h"

#include "containers.h"
#include "xCam.h"
#include "xFont.h"
#include "xQuickCull.h"
#include "xstransvc.h"
#include "xVolumeAsset.h"
#include "zBase.h"
#include "zEvent.h"
#include "zPlayerContainer.h"

#define MAX_OCCLUDERS 10

static zVolume* vols;
static U16 nvols;

static void zVolumeInit(zVolume* vol, xVolumeAsset* asset)
{
    vol->Init(asset);

    vol->eventFunc = zVolumeEventCB;
}

void zVolumeInit()
{
    nvols = xSTAssetCountByType('VOLU');

    if (nvols) {
        vols = (zVolume*)xMEMALLOC_L(nvols * sizeof(zVolume), 0, eMemMgrTag_EnvSpatial, 'VOLU', 55);
        xASSERT_L(56, vols);

        for (U16 i = 0; i < nvols; i++) {
            U32 size;
            xVolumeAsset* asset = (xVolumeAsset*)xSTFindAssetByType('VOLU', i, &size);
            xASSERT_L(61, asset);

            zVolumeInit(&vols[i], asset);
        }
    } else {
        vols = NULL;
    }
}

void zVolumeSetup()
{
    for (U32 i = 0; i < nvols; i++) {
        xASSERT_L(81, vols);
        xQuickCullForEverything(&vols[i].asset->bound.qcd);
    }
}

zVolume* zVolumeGetVolume(U16 n)
{
    xASSERT_L(94, nvols);
    xASSERT_L(95, vols);
    xASSERT_L(96, n < nvols);

    return &vols[n];
}

struct PreCalcOcclude
{
    xVec4 DepthVec;
    xVec4 FrustVec[4];
};

S32 gOccludeCount;
zVolume* gOccludeList[MAX_OCCLUDERS];
S32 gOccludeCalcCount;
PreCalcOcclude gOccludeCalc[MAX_OCCLUDERS];

void zVolume_OccludePrecalc(xVec3* camPos)
{
    S32 i, j;
    xVec3 corner[5];

    gOccludeCalcCount = 0;

    for (i = 0; i < gOccludeCount; i++) {
        zVolume* vol = gOccludeList[i];
        xVolumeAsset* a = vol->asset;
        F32 c = icos(a->rot);
        F32 s = isin(a->rot);

        PreCalcOcclude* calc = &gOccludeCalc[gOccludeCalcCount];

        corner[0].x = c * (a->bound.box.box.lower.x - a->xpivot) + a->xpivot;
        corner[0].y = a->bound.box.box.lower.y;
        corner[0].z = s * (a->bound.box.box.lower.x - a->xpivot) + a->bound.box.box.lower.z;

        corner[1].x = c * (a->bound.box.box.upper.x - a->xpivot) + a->xpivot;
        corner[1].y = a->bound.box.box.lower.y;
        corner[1].z = s * (a->bound.box.box.upper.x - a->xpivot) + a->bound.box.box.lower.z;

        corner[2].x = c * (a->bound.box.box.upper.x - a->xpivot) + a->xpivot;
        corner[2].y = a->bound.box.box.upper.y;
        corner[2].z = s * (a->bound.box.box.upper.x - a->xpivot) + a->bound.box.box.lower.z;

        corner[3].x = c * (a->bound.box.box.lower.x - a->xpivot) + a->xpivot;
        corner[3].y = a->bound.box.box.upper.y;
        corner[3].z = s * (a->bound.box.box.lower.x - a->xpivot) + a->bound.box.box.lower.z;

        corner[4] = corner[0];

        xVec3 d1, d2;
        xVec4 locFrustVec[4];

        xVec3Sub(&d1, &corner[1], &corner[0]);
        xVec3Sub(&d2, &corner[2], &corner[0]);
        xVec3Cross((xVec3*)&calc->DepthVec, &d1, &d2);
        xVec3Normalize((xVec3*)&calc->DepthVec, (xVec3*)&calc->DepthVec);
        F32 depthdot = xVec3Dot((xVec3*)&calc->DepthVec, &corner[0]);
        F32 camdot = xVec3Dot((xVec3*)&calc->DepthVec, camPos);

        if (camdot > depthdot) {
            xVec3Inv((xVec3*)&calc->DepthVec, (xVec3*)&calc->DepthVec);
            camdot = -camdot;
            depthdot = -depthdot;
        }

        if (xabs(camdot - depthdot) < 1.0f)
            continue;

        calc->DepthVec.w = depthdot;

        for (j = 0; j < 4; j++) {
            xVec3Sub(&d1, &corner[j], camPos);
            xVec3Sub(&d2, &corner[j + 1], camPos);
            xVec3Cross((xVec3*)&locFrustVec[j], &d1, &d2);
            xVec3Normalize((xVec3*)&locFrustVec[j], (xVec3*)&locFrustVec[j]);
        }

        F32 testdot1 = xVec3Dot((xVec3*)&locFrustVec[0], &corner[0]);
        F32 testdot2 = xVec3Dot((xVec3*)&locFrustVec[0], &corner[2]);
        if (testdot1 > testdot2) {
            for (j = 0; j < 4; j++) {
                xVec3Inv((xVec3*)&locFrustVec[j], (xVec3*)&locFrustVec[j]);
            }
        }

        for (j = 0; j < 4; j++) {
            locFrustVec[j].w = xVec3Dot((xVec3*)&locFrustVec[j], &corner[j]);
        }

        calc->FrustVec[0].x = locFrustVec[0].w;
        calc->FrustVec[0].y = locFrustVec[1].w;
        calc->FrustVec[0].z = locFrustVec[2].w;
        calc->FrustVec[0].w = locFrustVec[3].w;

        calc->FrustVec[1].x = locFrustVec[0].x;
        calc->FrustVec[1].y = locFrustVec[1].x;
        calc->FrustVec[1].z = locFrustVec[2].x;
        calc->FrustVec[1].w = locFrustVec[3].x;

        calc->FrustVec[2].x = locFrustVec[0].y;
        calc->FrustVec[2].y = locFrustVec[1].y;
        calc->FrustVec[2].z = locFrustVec[2].y;
        calc->FrustVec[2].w = locFrustVec[3].y;

        calc->FrustVec[3].x = locFrustVec[0].z;
        calc->FrustVec[3].y = locFrustVec[1].z;
        calc->FrustVec[3].z = locFrustVec[2].z;
        calc->FrustVec[3].w = locFrustVec[3].z;

        gOccludeCalcCount++;
    }
}

void zVolumeEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    zVolume* vol = (zVolume*)to;
    S32 i;

    xASSERT_L(359, vol);
    xASSERT_L(360, vol->baseType == eBaseTypeVolume);
    xASSERT_L(361, vol->asset);

    switch (toEvent) {
    case eEventReset:
    case eEventDebugReset:
        vol->Reset();
        break;
    case eEventOccludeOn:
        if (gOccludeCount == MAX_OCCLUDERS)
            break;

        for (i = 0; i < gOccludeCount; i++)
            if (gOccludeList[i] == vol) {
                return;
            }

        gOccludeList[gOccludeCount] = vol;
        gOccludeCount++;
        break;
    case eEventOccludeOff:
        for (i = 0; i < gOccludeCount; i++)
            if (gOccludeList[i] == vol) {
                // BUG: writes slot gOccludeCount, not slot i
                gOccludeList[gOccludeCount] = gOccludeList[gOccludeCount - 1];
                gOccludeCount--;
                return;
            }
        break;
    }
}
