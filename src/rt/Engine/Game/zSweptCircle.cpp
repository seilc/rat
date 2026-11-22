#include "xCommon.h"
#include "zSweptCircle.h"

void zSweptCircle::PrepareXZ(const xVec3& start, F32 radius, const xVec3& dir, F32 dist)
{
    this->start.assign(start.x, start.z);
    this->radius = radius;

    xVec3 end;
    end.AddScale(start, dir, dist);
    this->end.assign(end.x, end.z);

    if (dir.y < 0.00001f) {
        this->dir.assign(dir.x, dir.z);
        this->dist = dist;
    } else {
        this->dir.Sub(end, start);
        this->dist = this->dir.SafeNormalize();
    }

    left.assign(-this->dir.y, this->dir.x);

    Reset();
}

void zSweptCircle::Reset()
{
    curDist = dist;
    curEnd = end;
    hitSomething = false;
}

bool zSweptCircle::SweptCircleToPoint(xVec2& point)
{
    xVec2 toStart;
    toStart.Sub(start, point);

    F32 startAlong = -dir.dot(toStart);
    if (startAlong <= 0.0f) {
        return false;
    }

    F32 distAway = left.dot(toStart);
    if (distAway <= -radius || distAway >= radius) {
        return false;
    }

    F32 distToStart2 = toStart.length2();
    if (distToStart2 <= radius * radius) {
        hitSomething = true;
        curDist = 0.0f;
        curEnd = start;
        contactPoint = point;
        return true;
    }

    F32 backDist = radius - xsqrt(radius * radius - distAway * distAway + 0.00001f);
    if (startAlong < curDist + radius - backDist) {
        curDist = startAlong + backDist - radius;
        curEnd.AddScale(start, dir, curDist);
        hitSomething = true;
        contactPoint = point;
        return true;
    }

    return false;
}
