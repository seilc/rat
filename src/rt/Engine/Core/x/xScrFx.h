#ifndef XSCRFX_H
#define XSCRFX_H

#include "xMath3.h"

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
};

void xScrFxDrawBox(F32 x1, F32 y1, F32 x2, F32 y2, U8 red, U8 green, U8 blue, U8 alpha, bool clear_z_to_far);
cameraFX* xCameraFXShake(F32 maxTime, F32 magnitude, F32 cycleMax, F32 rotate_magnitude, F32 radius, const xVec3* epicenter, const xVec3* player, bool shakeY);
cameraFX* xCameraFXShakeForever(F32 magnitude, F32 cycleMax, F32 rotate_magnitude, F32 radius, const xVec3* epicenter, const xVec3* player, bool shakeY);
void xCameraFXShakeEnd(cameraFX* fx, F32 fadeOutTime);

// RESTORED
inline cameraFX* xCameraFXShake(const xCameraFXShakeParams& shakeParams, const xVec3* epicenter, const xVec3* player)
{
    return xCameraFXShake(shakeParams.maxTime, shakeParams.magnitude, shakeParams.cycleMax, shakeParams.rotateMagnitude, shakeParams.radius, epicenter, player, shakeParams.shakeY);
}
// END RESTORED

#endif
