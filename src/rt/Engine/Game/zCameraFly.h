#ifndef ZCAMERAFLY_H
#define ZCAMERAFLY_H

#include "xCam.h"

struct zCameraFly;

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

    void update(xScene&, F32 dt);
    void create();
    void start();
    void stop();
    xCam* get_next();
    void SetFlyData(void* data, U32 size, zCameraFly* fly, xCam* previous);

    static F32 skipTimeFlythrough;
    static F32 FLY_FRAMERATE;

    xCam* mPreviousCam;
    zFlyKey* mKeyFrames;
    S32 mNumKeys;
    F32 mFlyTime;
    F32 mFlyRate;
    bool mStopFly;
    zCameraFly* mCamFly;
};

#endif
