#ifndef XSPRINGY_H
#define XSPRINGY_H

#include "xVec3.h"

struct xSpringy
{
protected:
    F32 mResponse;
};

struct xSpringyVec3 : xSpringy
{
    void Update(F32 dt);
    
protected:
    xVec3 mVelocity;
    xVec3 mGoal;
    xVec3 mCurrent;
};

#endif
