#ifndef XSCRFX_H
#define XSCRFX_H

#include "xColor.h"
#include "xMath3.h"
#include "xScrFxLensFlareAsset.h"

struct cameraFXShake
{
    F32 magnitude;
    xVec3 dir;
    F32 cycleTime;
    F32 cycleMax;
    F32 dampen;
    F32 dampenRate;
    F32 rotate_magnitude;
    F32 radius;
    const xVec3* epicenterP;
    xVec3 epicenter;
    const xVec3* player;
};

struct cameraFXZoom
{
    F32 holdTime;
    F32 vel;
    F32 accel;
    F32 distance;
    U32 mode;
    F32 velCur;
    F32 distanceCur;
    F32 holdTimeCur;
};

struct cameraFX
{
    S32 type;
    S32 flags;
    F32 elapsedTime;
    F32 maxTime;
    union
    {
        cameraFXShake shake;
        cameraFXZoom zoom;
    };
};

struct xCameraFXShakeParams
{
    F32 maxTime;
    F32 magnitude;
    F32 cycleMax;
    F32 rotateMagnitude;
    F32 radius;
    bool shakeY;

#if DEBUG || RELEASE
    void AddTweaks(const char* prefix); // UNUSED
#endif
};

extern RwTexture* gxFlareTexture;
extern RwFrame* gxFlareFrame;
#if DEBUG || RELEASE
extern S32 gxLensFlareDebug;
#endif

void xScrFxInit();
void xScrFxReset();
void xScrFxUpdate(RwCamera*, F32 dt);
void xScrFxRender(RwCamera*);
void xScrFxCruiseDistortSetFactor(F32 factor); // UNUSED
void xScrFxDrawScreenSizeRectangle();
void xScrFxDrawBox(F32 x1, F32 y1, F32 x2, F32 y2, U8 red, U8 green, U8 blue, U8 alpha, bool clear_z_to_far);
void xScrFxFadeInit();
void xScrFxFade(const xColor* base, const xColor* dest, F32 seconds, S32 hold);
void xScrFxStopFade();
bool xScrFxIsFading(); // UNUSED
F32 xScrFxFadeAmount(); // UNUSED
void xScrFxShake(F32); // UNUSED
void xScrFxLetterBoxInit(); // UNUSED
void xScrFxLetterboxReset();
void xScrFxLetterBoxSetSize(F32 size);
F32 xScrFxLetterBoxGetSizeInScreenPercent();
void xScrFxLetterBoxSetAlpha(U8 alpha);
void xScrFxLetterbox(S32 enable);
void xScrFxRenderLetterBox(RwCamera*);
S32 xScrFxIsLetterbox(); // UNUSED
void xScrFxDrawSafeArea();
void xScrFXGlareInit();
void xScrFXGlareReset();
void xScrFXGlareDisable(S32 id);
S32 xScrFXGlareAdd(const xVec3* pos, F32 life, F32 intensity, F32 size, F32 r, F32 g, F32 b, F32 a, RwRaster* raster); // UNUSED
void xScrFXGlareChange(S32 id, const xVec3* pos, F32 life, F32 intensity);
bool xScrFXGlareActive(S32 id); // UNUSED
void xScrFXGlareUpdate(F32 dt);
void xScrFXFullScreenGlareEnable(S32 enable); // UNUSED
void xScrFXFullScreenGlareSetup(xVec3* dir, F32 intensity, U8 r, U8 g, U8 b, U8 a, U32 texID); // UNUSED
void xScrFXFullScreenGlareRender();
void xScrFXGlareRender(const xMat4x3& mat);
void xScrFxDrawBox(F32 x1, F32 y1, F32 x2, F32 y2, U8 red, U8 green, U8 blue, U8 alpha, F32 ushift, F32 vshift);
void xCameraFXBegin(const xMat4x3& mat);
bool xCameraFXAnyActive(); // UNUSED
void xCameraFXKill(S32 type); // UNUSED
void xCameraFXKill();
cameraFX* xCameraFXAlloc();
void xCameraFXZoomUpdate(cameraFX* f, F32 dt, const xMat4x3*, xMat4x3* m);
void xCameraFXZoom(F32 zoomVel, F32 zoomAccel, F32 zoomDistance, F32 zoomHoldTime); // UNUSED
cameraFX* xCameraFXShakeForever(F32 magnitude, F32 cycleMax, F32 rotate_magnitude, F32 radius, const xVec3* epicenter, const xVec3* player, bool shakeY);
void xCameraFXShakeEnd(cameraFX* fx, F32 fadeOutTime);
cameraFX* xCameraFXShake(F32 maxTime, F32 magnitude, F32 cycleMax, F32 rotate_magnitude, F32 radius, const xVec3* epicenter, const xVec3* player, bool shakeY);
void xCameraFXShakeUpdate(cameraFX* f, F32 dt, const xMat4x3*, xMat4x3* m);
void xCameraFXUpdate(xMat4x3& mat, RwCamera* icam, F32 dt);
void xCameraFXEnd(xMat4x3& mat, RwCamera* icam);
void xScrFxLensFlare_SceneEnter();
void xScrFxLensFlare_SceneExit();
void xScrFxLensFlare_Add(const xVec3* pos, const xVec3* at, F32 intensity, U32 sourceAssetID); // UNUSED
void xScrFxLensFlare_Reset();
void xScrFxLensFlare_Render(RwCamera* cam);

// UNUSED: needed to match (code from RotU GC)
inline cameraFX* xCameraFXShake(const xCameraFXShakeParams& shakeParams, const xVec3* epicenter, const xVec3* player)
{
    return xCameraFXShake(shakeParams.maxTime, shakeParams.magnitude, shakeParams.cycleMax, shakeParams.rotateMagnitude, shakeParams.radius, epicenter, player, shakeParams.shakeY);
}

#endif
