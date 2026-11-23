#ifndef ZCAMERACURVE_H
#define ZCAMERACURVE_H

#include "xBase.h"
#include "xVec3.h"
#include "zCameraCurveAsset.h"

struct xCam;
struct xCamTransitionParams;
class xNurbs;
class zCamPoolBase;

class zCameraCurve : public xBase
{
public:
    struct BeadInfo
    {
        S32 mBeadIndex;
        F32 mBeadIndexFrac;
        F32 mCurveU[2];
        xVec3 mCurvePos[2];
        F32 mDistanceScale;
        F32 mPitchOffset;
        F32 mTargetRadius;
        F32 mTargetMarginAngle;
        F32 mLeadOffset;
        F32 mYOffset;
        F32 mNearWallScale;
        F32 mFarWallScale;
    };

    class Follower
    {
    public:
        void Init(const zCameraCurve& curveInfo, const xVec3& playerPos);
        void UpdatePosition(const xVec3& playerPos);
        const xVec3& CameraPathPos() const { return mInfo.mCurvePos[eCamCurve_Camera]; }
        const BeadInfo& InterpBeadInfo(bool wantPlayerPathPosition);
        void DebugRender();

    private:
#if DEBUG || RELEASE
        static BeadInfo sTweakedBead;
        static bool sTweakedBeadActive;
#endif

        bool mValidPlayerPathPos;
        const zCameraCurve* mCurves;
        BeadInfo mInfo;
    };

    void Start(xCam&);
    void Stop(xCam&);
    bool Activate(const xCamTransitionParams* transition, bool forceRestart);

private:
    static xCam* sLastAddedCam;

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
