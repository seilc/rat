#ifndef ZCAM2PLAYER_H
#define ZCAM2PLAYER_H

#include "xCam.h"
#include "xCamSupport.h"
#include "xSpringy.h"

struct xEnt;

class zCam2Player : public xCam
{
public:
    struct rest_config
    {
        F32 dist;
        F32 input_center_phi;
        F32 yaw_offset;
        F32 pitch_offset;
    };

    virtual void create();
    virtual void start();
    virtual void stop();
    virtual void update(xScene& scene, F32 dt);
    virtual void debug_render();
    virtual zCam2Player* get_zCam2Player() const { return (zCam2Player*)this; }

    void set_secondary_target(const xVec3& targetPos, F32 radius, F32 marginAngle);
    void start_position_aim();
    void get_rest_default(rest_config& cfg) const;

    void set_rest(const rest_config& cfg, bool, F32)
    {
        rest = cfg;
    }

protected:
    struct basis_traits
    {
        xVec3 location;
        xSpringyVec3 origin;
        xSpringyAngle theta;
        xSpringyAngle phi;
        xSpringyF32 dist;
        xSpringyAngle yaw;
        xSpringyAngle thetaRestore;
        xSpringyF32 phiRestore;
        xSpringyF32 distRestore;
    };

    struct
    {
        const xEnt* ent;
        xVec3 position;
        F32 radius;
        F32 marginAngle;
    } mSecondary;
    bool mHaveSecondary;
    rest_config rest;
    F32 mPivotYOffset;
    bool mFirstFrame;
    bool mIsLocked;
    xCamSupportLOSBar mLOSBar;
    F32 mPreCollideThetaRestoreGoal;
    xSpringyVec3 mTargetPos;
    basis_traits mBasis;
    basis_traits mLastBasis;
    xSpringyF32 mMountY;
    xSpringyAngle mPitchOffset;
    xSpringyAngle mYawOffset;
#if DEBUG || RELEASE
    basis_traits mLockedBasis;
    bool mLockedBasisIsValid;
    bool mLockWasEnabled;
#endif
};

#endif
