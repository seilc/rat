#ifndef ZCAMERAFLY_H
#define ZCAMERAFLY_H

#include "xBase.h"
#include "xCam.h"
#include "zCameraFlyAsset.h"

struct zCameraFly : xBase
{
    CameraFly_asset* casset;
};

class zCamFlyThrough : public xCam
{
public:
    struct zFlyKey
    {
        S32 frame;
        F32 matrix[12];
        F32 aperture[2];
        F32 focal;
    };

    static F32 TranSpeed(zFlyKey* keys);
    static F32 MatrixSpeed(zFlyKey* keys);
    void update(xScene&, F32 dt);
    void create();
    void start();
    void stop();
    xCam* get_next();
    void SetFlyData(void* data, U32 size, zCameraFly* fly, xCam* previous);
    static void StartFly(zCameraFly* fly);
    static void StopFly(zCameraFly* fly); // UNUSED

    static const F32 FLY_FRAMERATE;
    static F32 skipTimeFlythrough;

private:
    xCam* mPreviousCam;
    zFlyKey* mKeyFrames;
    S32 mNumKeys;
    F32 mFlyTime;
    F32 mFlyRate;
    bool mStopFly;
    zCameraFly* mCamFly;
};

void zCameraFly_Init(xBase& data, xDynAsset& asset, size_t asset_size);
void zCameraFly_Init(zCameraFly* fly, CameraFly_asset* asset);
void zCameraFly_Setup(zCameraFly* fly);
void zCameraFly_Update(xBase* to, xScene* sc, F32);
void zCameraFly_Reset(zCameraFly* fly); // UNUSED
void zCameraFly_Save(zCameraFly* fly, xSerial* s);
void zCameraFly_Load(zCameraFly* fly, xSerial* s);
void zCameraFlyEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);

#endif
