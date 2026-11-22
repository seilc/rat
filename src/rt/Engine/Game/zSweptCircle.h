#ifndef ZSWEPTCIRCLE_H
#define ZSWEPTCIRCLE_H

#include "xMath2.h"
#include "xVec3.h"

class zSweptCircle
{
public:
    void Prepare(const xVec2& start, F32 radius, const xVec2& dir, F32 dist); // UNUSED
    void PrepareXZ(const xVec3& start, F32 radius, const xVec3& dir, F32 dist);
    void Reset();
    bool SweptCircleToPoint(xVec2& point);
    void DebugRenderXZ(F32 drawY); // UNUSED

    xVec2 start;
    F32 radius;
    F32 dist;
    xVec2 dir;
    xVec2 end;
    xVec2 left;
    F32 curDist;
    xVec2 curEnd;
    bool hitSomething;
    xVec2 contactPoint;
};

#endif
