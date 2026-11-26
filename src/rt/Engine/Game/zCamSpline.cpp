#include "xCommon.h"
#include "zCamSpline.h"

#include "containers.h"
#include "xMovePoint.h"
#include "xPtankPool.h"
#include "zAutoTweak.h"
#include "zCam.h"
#include "zGlobals.h"
#include "zPlayerConstrainer.h"

#if DEBUG || RELEASE
static bool yawCorrection;
#endif

void zCamSpline::create()
{
    xCam::create();
    cfg_common.priority = 129;
}

void zCamSpline::stop()
{
    xCam::stop();
    mCurveInfo->Stop(*this);
}

void zCamSpline::start()
{
    xCam::start();

    coord_type = XCAM_COORD_CART;
    orient_type = XCAM_ORIENT_QUAT;

    flags &= ~0x2;
    flags |= 0x80;

    mSprungTargetPos.SetResponsiveness(2.0f * cDefaultCameraSpring);
    mSprungTargetPos.SetGoal(globals.cameraLookAt);
    mSprungTargetPos.Reset();

    cfg_common.priority = 129;
    mTargetyOffset = 2.0f;

    InitPaths();
    mCurveInfo->Start(*this);
    coll_mat.pos = mCurveFollower.CameraPathPos();
    UpdateFloorMount(0.0f);
    mMountY.Reset();
    CalcOrientGoals(coll_mat.pos);
    mPitch.Reset();
    mYaw.Reset();
    xMat3x3Euler(&coll_mat, mYaw.Goal(), mPitch.Goal(), 0.0f);
    const zCameraCurve::BeadInfo& bead = mCurveFollower.InterpBeadInfo(false);
    CalcXOffsetGoal(bead.mLeadOffset);
    mXOffset.Reset();
    mYOffset.SetGoal(bead.mYOffset);
    mYOffset.Reset();

    coll_spatial.coord.cart = mCameraPathPosSprung.Goal();
    coll_spatial.coord.cart.AddScale(coll_mat.right, mXOffset.Goal());
    coll_spatial.coord.cart.y += mYOffset.Goal();
    coll_mat.pos = coll_spatial.coord.cart;

    xMat3x3Euler(&coll_mat, mYaw.Goal(), mPitch.Goal(), 0.0f);
    xQuatFromMat(&coll_spatial.orient.quat, &coll_mat);

    spatial = coll_spatial;
    mat = coll_mat;

#if DEBUG || RELEASE
    zPlayerConstrainer::OOBExtent extent;
    U32 flags = zPlayerConstrainer::PlayersOOB(extent, *this, coll_mat.at, 0);
    xASSERT_FMT_L(81, !(flags & (zPlayerConstrainer::flag_OOBCameraFlags)), "DESIGNER WARNING: RAIL Camera Starting with player(s) out of bounds! (could be near wall)");
#endif
}

void zCamSpline::pre_update(xScene&)
{
}

void zCamSpline::post_update(xScene&)
{
}

void zCamSpline::UpdateFloorMount(F32 dt)
{
    mMountY.SetGoal(zCamFloorMountY());
    mMountY.Update(dt);
}

#if DEBUG || RELEASE
static xVec3 lastProjectedMidpoint;
#endif

static void GetPlayersProjectedMidpoint(xVec3& midpoint, const xVec3&)
{
    midpoint = zCamLookPlayer();
#if DEBUG || RELEASE
    lastProjectedMidpoint = midpoint;
#endif
}

static F32 CalcYawToPlayers(const xVec3& fromPos)
{
    const xVec3& target = zCamLookPlayer();
    return xatan2(target.x - fromPos.x, target.z - fromPos.z);
}

void zCamSpline::CalcPitchGoal(const xVec3& fromPos)
{
    xVec3 target;
    GetPlayersProjectedMidpoint(target, fromPos);
    target.y = (F32)mMountY + mTargetyOffset;

    xVec3 toTarget;
    toTarget.Sub(target, fromPos);
    F32 distanceToTarget = toTarget.length();
    mPitch = distanceToTarget < EPSILON ? 0.0f : -xasin(toTarget.y / distanceToTarget);
}

void zCamSpline::CalcOrientGoals(const xVec3& fromPos)
{
    mYaw = CalcYawToPlayers(fromPos);
    CalcPitchGoal(fromPos);
}

void zCamSpline::CalcXOffsetGoal(F32 rawXOffset)
{
    mXOffset = rawXOffset;
    if (zCamIsChangingFocus())
        return;

    zPlayerConstrainer::OOBExtent extent;
    zPlayerConstrainer::PlayersOOB(extent, *this, coll_mat.at, 2);

    if (mXOffset.Goal() < 0.0f) {
        if (-mXOffset.Goal() > extent.leftXMargin)
            mXOffset = -extent.leftXMargin;
    } else {
        if (mXOffset.Goal() > extent.rightXMargin)
            mXOffset = extent.rightXMargin;
    }
}

void zCamSpline::ApplyBeadSettings(const zCameraCurve::BeadInfo& beadInfo)
{
    F32 nearWall, farWall;
    zPlayerConstrainer::GetDefaults(nearWall, farWall);
    nearWall *= beadInfo.mNearWallScale;
    farWall *= beadInfo.mFarWallScale;
    zPlayerConstrainer::SetConstraints(nearWall, farWall);
}

void zCamSpline::update(xScene&, F32 dt)
{
    zCameraCurve::Follower lastGoodFollower = mCurveFollower;

    UpdateFloorMount(dt);
    UpdatePaths(dt);
    const zCameraCurve::BeadInfo* bead = &mCurveFollower.InterpBeadInfo(false);
    ApplyBeadSettings(*bead);
    coll_mat.pos = mCurveFollower.CameraPathPos();
    mCameraPathPosSprung.SetGoal(coll_mat.pos);
    CalcOrientGoals(coll_mat.pos);

    xMat3x3Euler(&coll_mat, mYaw.Goal(), mPitch.Goal(), 0.0f);

    zPlayerConstrainer::OOBExtent extent;
    U32 flags = zPlayerConstrainer::PlayersOOB(extent, *this, coll_mat.at, 0);
    if ((flags & zPlayerConstrainer::flag_OOBCameraFlags) && !zCamIsChangingFocus()) {
        F32 deltaU = mCurveFollower.PlayerU() - lastGoodFollower.PlayerU();
        F32 cIncU = deltaU / 10.0f;
        F32 cBaseU = lastGoodFollower.PlayerU();
        for (S32 i = 0; i < 10; i++) {
            mCurveFollower.UpdatePosition(cBaseU + cIncU * i);
            bead = &mCurveFollower.InterpBeadInfo(false);
            ApplyBeadSettings(*bead);
            coll_mat.pos = mCurveFollower.CameraPathPos();
            CalcOrientGoals(coll_mat.pos);

            xMat3x3Euler(&coll_mat, mYaw.Goal(), mPitch.Goal(), 0.0f);
            flags = zPlayerConstrainer::PlayersOOB(extent, *this, coll_mat.at, 0);
            if (flags & zPlayerConstrainer::flag_OOBCameraFlags) {
                mCurveFollower = lastGoodFollower;
                mCurveFollower.InterpBeadInfo(false);
                coll_mat.pos = mCurveFollower.CameraPathPos();
                mCameraPathPosSprung.SetGoal(coll_mat.pos);
                CalcOrientGoals(coll_mat.pos);
                break;
            } else {
                lastGoodFollower = mCurveFollower;
            }
        }
    }

    mCameraPathPosSprung.Update(dt);
    mYaw.Update(dt);
    mPitch.Update(dt);

    bead = &mCurveFollower.InterpBeadInfo(false);
    spatial.coord.cart = mCameraPathPosSprung;
    xMat3x3Euler(&mat, mYaw, mPitch, 0.0f);
    xQuatFromMat(&spatial.orient.quat, &mat);

    xMat3x3Euler(&coll_mat, mYaw.Goal(), mPitch.Goal(), 0.0f);
    coll_spatial.coord.cart = mCameraPathPosSprung.Goal();
    xQuatFromMat(&coll_spatial.orient.quat, &coll_mat);

    if (bead->mLeadOffset != 0.0f || xabs(mXOffset) > EPSILON) {
        coord_to_world(coll_mat.pos, coll_spatial.coord.cart);
        CalcXOffsetGoal(bead->mLeadOffset);
        mXOffset.Update(dt);
        spatial.coord.cart.AddScale(mat.right, mXOffset);
        coll_spatial.coord.cart.AddScale(coll_mat.right, mXOffset.Goal());
    }

    if (bead->mYOffset != 0.0f || xabs(mYOffset) > EPSILON) {
        mYOffset.SetGoal(bead->mYOffset);
        mYOffset.Update(dt);
        spatial.coord.cart.y += mYOffset;
        coll_spatial.coord.cart.y += mYOffset.Goal();
    }

    coord_to_world(mat.pos, spatial.coord.cart);
    coord_to_world(coll_mat.pos, coll_spatial.coord.cart);
}

#if DEBUG || RELEASE
void zCamSpline::debug_mode_draw()
{
    if (!active())
        return;

    xprintf("Curve Rail\n");
    xCam::debug_mode_draw();
    DM_Draw();
    const zCameraCurve::BeadInfo& beadValues = mCurveFollower.InterpBeadInfo(false);
    xprintf("Near Wall:%3.1f      Far Wall:%3.1f\n", beadValues.mNearWallScale, beadValues.mFarWallScale);
}

void zCamSpline::add_tweaks(const char* prefix)
{
    xCam::add_tweaks(prefix);
    AddTweaks(prefix);
    zAUTOTWEAK(prefix, "Yaw Correction", &yawCorrection, 0, 1, NULL, NULL, 2);
}
#endif
