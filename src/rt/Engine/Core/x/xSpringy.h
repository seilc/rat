#ifndef XSPRINGY_H
#define XSPRINGY_H

#include "xVec3.h"

class xSpringy
{
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
