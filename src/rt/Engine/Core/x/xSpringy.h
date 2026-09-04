#ifndef XSPRINGY_H
#define XSPRINGY_H

#include "xVec3.h"

class xSpringy
{
public:
    void SetTension(F32 tension);

protected:
    F32 mResponse;
};

class xSpringyVec3 : public xSpringy
{
public:
    void Update(F32 dt);
    
protected:
    xVec3 mVelocity;
    xVec3 mGoal;
    xVec3 mCurrent;
};

class xSpringyF32 : public xSpringy
{
public:
    xSpringyF32()
    {
        mResponse = 10.0f;
        mGoal = 0.0f;
        mVelocityMax = 0.0f;
        mVelocitySaveMax = 0.0f;
        Reset();
    }

    void SetGoal(F32 target)
    {
        mGoal = target;
    }

    F32 Goal() const
    {
        return mGoal;
    }

    void Reset()
    {
        mCurrent = mGoal;
        mVelocity = 0.0f;
    }

    void SnapGoal()
    {
        Reset();
    }

    void SnapTo(F32 target)
    {
        SetGoal(target);
        SnapGoal();
    }

    void Update(F32 dt)
    {
        F32 delta = mGoal - mCurrent;
        if (xabs(mVelocity) > mVelocitySaveMax) {
            mVelocitySaveMax = xabs(mVelocity);
        }
        if (mVelocityMax != 0.0f && xabs(mVelocity) > mVelocityMax) {
            if (mVelocity < 0.0f) {
                mVelocity = -mVelocityMax;
            } else {
                mVelocity = mVelocityMax;
            }
        }
        xDampSpring(delta, mVelocity, dt, mResponse);
        mCurrent = mGoal - delta;
    }

    F32 Sprung() const
    {
        return mCurrent;
    }

    void operator=(F32 value)
    {
        SetGoal(value);
    }

    operator const F32() const
    {
        return Sprung();
    }

protected:
    F32 mVelocitySaveMax;
    F32 mVelocityMax;
    F32 mVelocity;
    F32 mGoal;
    F32 mCurrent;
};

class xSpringyAngle : public xSpringyF32
{
};

#endif
