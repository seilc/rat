#ifndef ZCAMSPLINECOMMONMIX_H
#define ZCAMSPLINECOMMONMIX_H

#include "xSpringy.h"
#include "zCameraCurve.h"

class zCamSplineCommonMix
{
public:
    zCamSplineCommonMix();

    virtual void SetCurves(zCameraCurve* curveInfo);

    void InitPaths();
    void UpdatePaths(F32);
    void DebugRender();
    void DM_Draw();
#if DEBUG || RELEASE
    void AddTweaks(const char* prefix);
#endif

    static const F32 cDefaultCameraSpring;
#if DEBUG || RELEASE
    static S32 mDebugDraw;
#endif

protected:
#if DEBUG || RELEASE
    F32 mSpringResponse;
#endif
    zCameraCurve* mCurveInfo;
    zCameraCurve::Follower mCurveFollower;
    xSpringyVec3 mCameraPathPosSprung;
};

#endif
