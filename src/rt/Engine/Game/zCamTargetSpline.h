#ifndef ZCAMTARGETSPLINE_H
#define ZCAMTARGETSPLINE_H

#include "zCam2Player.h"
#include "zCamSplineCommonMix.h"

struct zCamTargetSpline : zCam2Player, zCamSplineCommonMix
{
    virtual zCamSplineCommonMix* get_common_mix() { return this; }
    virtual void create();
    virtual void stop();
    virtual void start();
    virtual void pre_update(xScene&);
    virtual void post_update(xScene&);
    void ApplyBeadValues(bool lazy);
    virtual void update(xScene& scene, F32 dt);
#if DEBUG || RELEASE
    virtual void debug_mode_draw();
    virtual void add_tweaks(const char* prefix);
#endif

    xCamSpatialInfo oldSpatial;
    xMat4x3 oldMat;
    xSpringyF32 mYOffset;
};

#endif
