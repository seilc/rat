#include "xCommon.h"
#include "zCamSplineCommonMix.h"

#include "xCam.h"
#include "xEnt.h"
#include "xGlobals.h"
#include "zAutoTweak.h"
#include "zGlobals.h"
#include "zPlayer.h"

const F32 zCamSplineCommonMix::cDefaultCameraSpring = 10.0f;
#if DEBUG || RELEASE
S32 zCamSplineCommonMix::mDebugDraw;
#endif

zCamSplineCommonMix::zCamSplineCommonMix()
{
#if DEBUG || RELEASE
    mSpringResponse = cDefaultCameraSpring;
#endif
}

void zCamSplineCommonMix::SetCurves(zCameraCurve* curveInfo)
{
    mCurveInfo = curveInfo;
    InitPaths();
}

void zCamSplineCommonMix::InitPaths()
{
    mCurveFollower.Init(*mCurveInfo, globals.cameraPlayersMidpoint);
    mCameraPathPosSprung.SetGoal(mCurveFollower.CameraPathPos());
    mCameraPathPosSprung.Reset();
}

void zCamSplineCommonMix::UpdatePaths(F32)
{
#if DEBUG || RELEASE
    mCameraPathPosSprung.SetResponsiveness(mSpringResponse);
#endif

    mCurveFollower.UpdatePosition(globals.cameraPlayersMidpoint);

#if DEBUG || RELEASE
    DebugRender();
#endif
}

void zCamSplineCommonMix::DebugRender()
{
#if DEBUG || RELEASE
    if (mDebugDraw)
        mCurveFollower.DebugRender();
#endif
}

void zCamSplineCommonMix::DM_Draw()
{
#if DEBUG || RELEASE
    if (mCurveInfo) {
        F32 playersDistanceApart = 0.0f;
        if (CURPLAYERS.size() == 2) {
            xVec3 v;
            v.Sub(xEntGetFrame(CURPLAYERS[1])->pos, xEntGetFrame(CURPLAYERS[0])->pos);
            playersDistanceApart = v.length();
        }

        const zCameraCurve::BeadInfo& curBeadInfo = mCurveFollower.InterpBeadInfo(false);

        xprintf("Bead Index:%.3f  Player U:%.4f\n", curBeadInfo.mBeadIndex + curBeadInfo.mBeadIndexFrac, curBeadInfo.mCurveU[eCamCurve_Player]);
        xprintf("Players Distance:%.2f\n", playersDistanceApart);
    }
#endif
}

#if DEBUG || RELEASE
void zCamSplineCommonMix::AddTweaks(const char* prefix)
{
    zAUTOTWEAK(prefix, "Debug Lines On", &mDebugDraw, 0, 1, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "Spring Response", &mSpringResponse, 0.1f, 500.0f, NULL, NULL, 2);
}
#endif
