#include "xSpringy.h"

void xSpringyVec3::Update(F32 dt)
{
    F32 e = xDampSpringCoeff(dt, mResponse);

    xVec3 delta;
    delta.Sub(mGoal, mCurrent);

    xDampSpring(delta.x, mVelocity.x, dt, mResponse, e);
    xDampSpring(delta.y, mVelocity.y, dt, mResponse, e);
    xDampSpring(delta.z, mVelocity.z, dt, mResponse, e);

    mCurrent.Sub(mGoal, delta);
}
