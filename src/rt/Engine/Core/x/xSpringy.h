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

#endif
