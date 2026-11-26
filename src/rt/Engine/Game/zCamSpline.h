#ifndef ZCAMSPLINE_H
#define ZCAMSPLINE_H

#include "xCam.h"
#include "xSpringy.h"
#include "zCamSplineCommonMix.h"

struct zCamSpline : xCam, zCamSplineCommonMix
{
    virtual zCamSplineCommonMix* get_common_mix() { return this; }
    virtual void create();
    virtual void stop();
    virtual void start();
    virtual void pre_update(xScene&);
    virtual void post_update(xScene&);
    void UpdateFloorMount(F32 dt);
    void UpdateTarget(F32); // UNUSED
    void CalcPitchGoal(const xVec3& fromPos);
    void CalcOrientGoals(const xVec3& fromPos);
    void CalcXOffsetGoal(F32 rawXOffset);
    void ApplyBeadSettings(const zCameraCurve::BeadInfo& beadInfo);
    virtual void update(xScene&, F32 dt);
#if DEBUG || RELEASE
    virtual void debug_mode_draw();
    virtual void add_tweaks(const char* prefix);
#endif

    F32 mTargetyOffset;
    xSpringyVec3 mSprungTargetPos;
    xSpringyF32 mMountY;
    xSpringyF32 mXOffset;
    xSpringyF32 mYOffset;
    xSpringyAngle mYaw;
    xSpringyAngle mPitch;
};

#endif
