#include "iCamera.h"

#include "iAsync.h"
#include "iScrFx.h"
#include "iMemMgr.h"
#include "zGameState.h"
#include "xGlobals.h"
#include "xShadow.h"
#include "xDebugTweak.h"

#include <string.h>

#undef RwCameraCreate
#undef RwCameraBeginUpdate

// Taken from baworobj.c
typedef struct RpWorldCameraExt RpWorldCameraExt;
struct RpWorldCameraExt
{
    RpWorldSector** frustumSectors;
    RwInt32 space;
    RwInt32 position;
    RpWorld* world;
    RwCameraBeginUpdateFunc oldBeginUpdate;
    RwCameraEndUpdateFunc oldEndUpdate;
    RwObjectHasFrameSyncFunction oldSync;
};
#define rwCAMERADISPLAYSECTORSINCREASE 50
#define WORLDCAMERAEXTFROMCAMERA(camera) ((RpWorldCameraExt *)(((RwUInt8 *)(camera)) + cameraExtOffset))

F32 sCameraNearClip = 0.2f;
F32 giCameraOONearClip = 5.0f;
F32 sCameraFarClip = 400.0f;
RwCamera* globalCamera = NULL;
static RwCamera* sMainGameCamera = NULL;

namespace {

RwFogType tweakFogType = rwFOGTYPELINEAR;
F32 tweakFogStart = 0.0f;
F32 tweakFogStop = 0.0f;
F32 tweakFogDensity = 0.0f;
F32 tweakFarClip = 0.0f;

inline F32 iCameraFogFarClip()
{
#ifdef DEBUGRELEASE
    return tweakFarClip > 0.0f ? tweakFarClip : xglobals->fog.stop;
#else
    return xglobals->fog.stop;
#endif
}

#ifndef NON_MATCHING
void AddTweaks()
{
    xTWEAK("FX|Fog|Type", (U32*)&tweakFogType, rwFOGTYPELINEAR, rwFOGTYPEEXPONENTIAL2, NULL, NULL, 0);
    xTWEAK("FX|Fog|Start", &tweakFogStart, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("FX|Fog|Stop", &tweakFogStop, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("FX|Fog|Density", &tweakFogDensity, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("FX|Fog|Far Clip", &tweakFarClip, 0.0f, 1.0f, NULL, NULL, 0);
}
#endif

}

void iCameraSetOrthoProjection(RwCamera* cam, bool ortho)
{
    RwCamera* camera = RwCameraGetCurrentCamera();

    if (cam == camera) {
        RwCameraEndUpdate(camera);
    }

    RwCameraSetProjection(cam, ortho ? rwPARALLEL : rwPERSPECTIVE);

    if (cam == camera) {
        RwCameraBeginUpdateWrapper(camera);
    }
}

RwCamera* iCameraCreate(S32 width, S32 height, S32 mainGameCamera)
{
    iAsyncLockRWSentry rw_sentry;
    RwV2d vw;
    RwCamera* camera;
    RwFrame* cameraFrame;
    RwRaster* cameraRaster, *zbuffer;

    camera = RwCameraCreate();
    xASSERT(150, camera);

    cameraFrame = RwFrameCreate();
    xASSERT(154, cameraFrame);
    RwCameraSetFrame(camera, cameraFrame);

    cameraRaster = RwRasterCreate(width, height, 0, rwRASTERTYPECAMERA);
    xASSERT(158, cameraRaster != NULL);
    RwCameraSetRaster(camera, cameraRaster);

    zbuffer = RwRasterCreate(width, height, 0, rwRASTERTYPEZBUFFER);
    xASSERT(163, zbuffer != NULL);
    RwCameraSetZRaster(camera, zbuffer);

    RwCameraSetFarClipPlane(camera, sCameraFarClip);
    RwCameraSetNearClipPlane(camera, sCameraNearClip);

    vw.x = 1.0f;
    vw.y = 1.0f;
    RwCameraSetViewWindow(camera, &vw);

    if (mainGameCamera) {
        xASSERT(181, sMainGameCamera == 0);
        iScrFxCameraCreated(camera);
        sMainGameCamera = camera;
    }

    return camera;
}

void iCameraDestroy(RwCamera* camera)
{
    xASSERT(197, camera);

    iAsyncLockRWSentry rw_sentry;
    RpWorld* pWorld;
    RwRaster* raster;
    RwFrame* frame;

    iMemMgrEnableRenderWareMemoryHack(true);
    _rwFrameSyncDirty();
    iMemMgrEnableRenderWareMemoryHack(false);

    pWorld = RwCameraGetWorld(camera);
    if (pWorld) {
        RpWorldRemoveCamera(pWorld, camera);
    }

    if (camera == sMainGameCamera) {
        iScrFxCameraDestroyed(camera);
        sMainGameCamera = 0;
    }

    if (camera) {
        frame = RwCameraGetFrame(camera);
        if (frame) {
            RwCameraSetFrame(camera, NULL);
            RwFrameDestroy(frame);
        }

        raster = RwCameraGetRaster(camera);
        if (raster) {
            RwRasterDestroy(raster);
            RwCameraSetRaster(camera, NULL);
        }

        raster = RwCameraGetZRaster(camera);
        if (raster) {
            RwRasterDestroy(raster);
            RwCameraSetZRaster(camera, NULL);
        }

        RwCameraDestroy(camera);
    }
}

#ifdef DEBUGRELEASE
F32 iCameraClearTime = 0.0f;
F32 iCameraClearTimeLast = 0.0f;
#endif

namespace {

RwRGBA hotPink = { 128, 0, 128, 255 };

}

void iCameraBegin(RwCamera* cam, S32 clear)
{
    xASSERT(277, cam);

#ifdef DEBUGRELEASE
    iTime timeA = iTimeGet();
#endif

#ifndef DEBUGRELEASE
    if (clear) {
        if (xglobals->fog.type != rwFOGTYPENAFOGTYPE) {
            RwCameraClear(cam, &xglobals->fog.bgcolor, rwCAMERACLEARIMAGE | rwCAMERACLEARZ | rwCAMERACLEARSTENCIL);
        } else {
            RwCameraClear(cam, NULL, rwCAMERACLEARZ);
        }
    }
#else
    if (zGameGetOstrich() != eGameOstrich_PlayingMovie && !xglobals->dumpCutscene) {
        RwCameraClear(cam, &hotPink, rwCAMERACLEARIMAGE | rwCAMERACLEARZ | rwCAMERACLEARSTENCIL);
    }
#endif

#ifdef DEBUGRELEASE
    iTime timeB = iTimeGet();

    iCameraClearTimeLast = iCameraClearTime;
    iCameraClearTime = iTimeDiffSec(timeA, timeB);
#endif

    RwCameraSetNearClipPlane(cam, sCameraNearClip);
    RwCameraBeginUpdateWrapper(cam);
}

void iCameraEnd(RwCamera* cam)
{
    xASSERT(336, cam);

    iScrFxCameraEndScene(cam);
    RwCameraEndUpdate(cam);
}

void iCameraShowRaster(RwCamera* cam)
{
    xASSERT(354, cam);

    RwCameraShowRaster(cam, NULL, 0);
}

void iCameraUpdatePos(RwCamera* cam, xMat4x3* pos)
{
    xASSERT(383, cam);
    xASSERT(384, pos);

    RwFrame* f;
    RwMatrix* m;

    f = RwCameraGetFrame(cam);
    xASSERT(387, f);

    m = RwFrameGetMatrix(f);
    xASSERT(389, m);
    xMat4x3Copy((xMat4x3*)m, pos);

    m = RwFrameGetLTM(f);
    xASSERT(394, m);
    xMat4x3Copy((xMat4x3*)m, pos);
    
    RwFrameOrthoNormalize(f);
    RwFrameUpdateObjects(f);
}

F32 iCameraGetAspectRatio()
{
    return 3/4.f;
}

void iCameraSetFOV(RwCamera* cam, F32 fov)
{
    F32 view_window;
    F32 fAspectRatio;
    RwV2d vw;

    view_window = itan(0.5f * fov * (PI/180.0f));
    fAspectRatio = iCameraGetAspectRatio();

    vw.x = view_window;
    vw.y = view_window * fAspectRatio;
    RwCameraSetViewWindow(cam, &vw);
}

#ifndef NON_MATCHING
F32 iCameraGetFOV(RwCamera* cam)
{
    xASSERT(0, cam != 0);

    const RwV2d* vw;

    vw = RwCameraGetViewWindow(cam);
    xASSERT(0, vw != 0);

    return 0;
}
#endif

void iCameraAssignEnv(RwCamera* camera, iEnv* env_geom)
{
    xASSERT(500, env_geom);
    xASSERT(501, env_geom->world);

    iAsyncLockRWSentry rw_sentry;

    globalCamera = camera;

    RpWorldAddCamera(env_geom->world, camera);
    xShadowSetWorld(env_geom->world);
}

void iCamGetViewMatrix(RwCamera* camera, xMat4x3* view_matrix)
{
    RwMatrix* rw_view;

    rw_view = RwCameraGetViewMatrix(camera);

    memset(view_matrix, 0, sizeof(xMat4x3));

    view_matrix->right.x = rw_view->right.x;
    view_matrix->right.y = rw_view->right.y;
    view_matrix->right.z = rw_view->right.z;
    view_matrix->up.x = rw_view->up.x;
    view_matrix->up.y = rw_view->up.y;
    view_matrix->up.z = rw_view->up.z;
    view_matrix->at.x = rw_view->at.x;
    view_matrix->at.y = rw_view->at.y;
    view_matrix->at.z = rw_view->at.z;
    view_matrix->pos.x = rw_view->pos.x;
    view_matrix->pos.y = rw_view->pos.y;
    view_matrix->pos.z = rw_view->pos.z;
}

void iCameraSetNearFarClip(F32 nearPlane, F32 farPlane)
{
    sCameraNearClip = (nearPlane <= 0.0f) ? 0.2f : nearPlane;
    sCameraFarClip = (farPlane <= 0.0f) ? 400.0f : farPlane;
}

void iCameraGetNearFarClip(F32& nearPlane, F32& farPlane)
{
    nearPlane = sCameraNearClip;
    farPlane = sCameraFarClip;
}

void iCameraSetFogParams(iFogParams* fp, F32 time)
{
    if (!fp || fp->type == rwFOGTYPENAFOGTYPE) {
        xglobals->fog.type = rwFOGTYPENAFOGTYPE;
        xglobals->fogA.type = rwFOGTYPENAFOGTYPE;
    } else if (time == 0.0f || fp->type != xglobals->fogA.type) {
        xglobals->fog = *fp;
        xglobals->fogA = *fp;
        xglobals->fog_t0 = 0;
    } else {
        xglobals->fogA = xglobals->fog;
        xglobals->fogB = *fp;
        xglobals->fog_t0 = iTimeGet();
        xglobals->fog_t1 = xglobals->fog_t0 + iTimeFromSec(time);
    }
}

void iCameraUpdateFog(RwCamera*, iTime t)
{
    iFogParams* fp = &xglobals->fog;
    iFogParams* fa = &xglobals->fogA;
    iFogParams* fb = &xglobals->fogB;
    F32 a;
    RwRGBA ca, cb, color;

// ???
#ifdef DEBUG
    if (xglobals->fog.type == rwFOGTYPENAFOGTYPE || xglobals->fog_t0 == 0) {
        return;
    }
#else
    if (xglobals->fog.type == rwFOGTYPENAFOGTYPE) return;
    if (xglobals->fog_t0 == 0) return;
#endif

    t = iTimeGet();

    a = iTimeDiffSec(xglobals->fog_t0, t) / iTimeDiffSec(xglobals->fog_t0, xglobals->fog_t1);
    a = xclamp(a, 0.0f, 1.0f);

    fp->type = xglobals->fogB.type;
    fp->table = xglobals->fogB.table;
    fp->start = fa->start + a * (fb->start - fa->start);
    fp->stop = fa->stop + a * (fb->stop - fa->stop);
    fp->density = fa->density + a * (fb->density - fa->density);

    ca = fa->fogcolor;
    cb = fb->fogcolor;
    color.red = ca.red + a * (cb.red - ca.red);
    color.green = ca.green + a * (cb.green - ca.green);
    color.blue = ca.blue + a * (cb.blue - ca.blue);
    color.alpha = ca.alpha + a * (cb.alpha - ca.alpha);
    fp->fogcolor = color;

    ca = fa->bgcolor;
    cb = fb->bgcolor;
    color.red = ca.red + a * (cb.red - ca.red);
    color.green = ca.green + a * (cb.green - ca.green);
    color.blue = ca.blue + a * (cb.blue - ca.blue);
    color.alpha = ca.alpha + a * (cb.alpha - ca.alpha);
    fp->bgcolor = color;

    if (a == 1.0f) {
        xglobals->fog_t0 = 0;
        xglobals->fogA = xglobals->fogB;
    }
}

static inline void iCameraFogOffWrapper()
{
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
}

void iCameraSetFogRenderStates()
{
    RwCamera* pCamera;
    iFogParams* pFogParams;
    RwUInt32 bite_me;
    
    pCamera = RwCameraGetCurrentCamera();
    if (!pCamera) return;

    pFogParams = &xglobals->fog;
    if (pFogParams->type == rwFOGTYPENAFOGTYPE || pFogParams->start >= pFogParams->stop) {
        if (pFogParams->type == rwFOGTYPENAFOGTYPE) {
            RwCameraSetFarClipPlane(pCamera, sCameraFarClip);
        } else {
            RwCameraSetFarClipPlane(pCamera, pFogParams->stop);
        }
        iCameraFogOffWrapper();
    } else {
        bite_me = RWRGBALONG(pFogParams->fogcolor.red,
                             pFogParams->fogcolor.green,
                             pFogParams->fogcolor.blue,
                             pFogParams->fogcolor.alpha);
                             
        RwCameraSetFarClipPlane(pCamera, iCameraFogFarClip());
        RwCameraSetFogDistance(pCamera, pFogParams->start);
        RwRenderStateSet(rwRENDERSTATEFOGCOLOR, (void*)bite_me);
        RwRenderStateSet(rwRENDERSTATEFOGDENSITY, (void*)&pFogParams->density);
        RwRenderStateSet(rwRENDERSTATEFOGTYPE, (void*)pFogParams->type);
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)TRUE);
    }
}

void iCameraSetFog(bool enable)
{
    iFogParams* pFogParams = &xglobals->fog;
    if (pFogParams->type == rwFOGTYPENAFOGTYPE || pFogParams->start >= pFogParams->stop) {
        return;
    }

    if (enable) {
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)TRUE);
    } else {
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    }
}

RwCamera* RwCameraBeginUpdateWrapper(RwCamera* camera)
{
    RwCamera* retval = RwCameraBeginUpdate(camera);
    return retval;
}

void UpdateGlobalFrustumPlanes(RwCamera*)
{
}

void iCameraPerformRenderWareHack(RwCamera* camera)
{
    RwUInt32 memSize;
    RpWorldSector** newFrustumSectors;
    RwInt32 cameraExtOffset;
    RpWorldCameraExt* cameraExt;

    memSize = rwCAMERADISPLAYSECTORSINCREASE * sizeof(RpWorldSector*);
    newFrustumSectors = (RpWorldSector**)RwMallocL(memSize, rwMEMHINTDUR_NADURATION, 1663);
    cameraExtOffset = RwCameraGetPluginOffset(rwID_WORLDOBJMODULE);
    cameraExt = WORLDCAMERAEXTFROMCAMERA(camera);

    cameraExt->frustumSectors = newFrustumSectors;
    cameraExt->space = rwCAMERADISPLAYSECTORSINCREASE;
}
