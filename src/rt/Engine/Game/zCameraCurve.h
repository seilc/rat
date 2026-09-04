#ifndef ZCAMERACURVE_H
#define ZCAMERACURVE_H

#include "xBase.h"
#include "zCameraCurveAsset.h"

struct xCam;
struct xCamTransitionParams;
class xNurbs;
class zCamPoolBase;

class zCameraCurve : public xBase
{
public:
    bool Activate(const xCamTransitionParams* transition, bool forceRestart);

private:
    zCameraCurveAsset* mAsset;
    xNurbs* mCurve[2];
    F32 mCurveLength[2];
    F32 mRenderIncU[2];
    zCamPoolBase* mCamPool;
    const xCam* mCamera;

#if DEBUG || RELEASE
    S32 mUsageLevel;
#endif
};

#endif
