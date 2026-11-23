#include "xCommon.h"
#include "zToss.h"

#include "xEnt.h"
#include "xEvent.h"
#include "zEvent.h"
#include "xMath2.h"
#include "zOtherAsset.h"
#include "containers.h"

namespace z {
void toss::Setup()
{
}

toss* toss::find(xEnt* ent)
{
    for (U32 i = 0; i < MAX_TOSSES; i++) {
        toss* cur = GetIndex(i);
        if (cur->IsUsed() && cur->ent == ent) {
            return cur;
        }
    }
    return NULL;
}

toss::toss(xEnt* ent, pointer_asset* dest_pointer, F32 arc_height, F32 speed, F32, xBase* toss_event_sender)
    : zLightweight<toss, MAX_TOSSES>(true), ent(ent), dest_pointer(dest_pointer), arc_height(arc_height),
      speed(speed), toss_event_sender(toss_event_sender)
{
    toDest.Sub(dest_pointer->loc, *xEntGetPos(ent));
    xVec2 toDest2d = { toDest.x, toDest.z };
    dist = totalDist = toDest2d.length();

    F32 a = -toDest.y, b = -2.0f * arc_height * dist, c = arc_height * dist * dist, m0, m1;
    U32 num = xMathSolveQuadratic(a, b, c, &m0, &m1);
    xASSERT_L(69, num>0);
    peakDist = m0;
    if (m0 >= 0.0f && m0 <= dist)
        peakDist = m0;
    else if (m1 >= 0.0f && m1 <= dist)
        peakDist = m1;
    else {
        xASSERT_L(77, !"Impossible arc");
        dist = 0.0f;
        return;
    }

    if (peakDist == 0.0f) {
        dist = 0.0f;
        return;
    }

    parabola_scale = arc_height / (peakDist * peakDist);

    xMat3x3 finalMat;
    xMat3x3Euler(&finalMat, (PI / 180.0f) * dest_pointer->yaw, (PI / 180.0f) * dest_pointer->pitch,
                 (PI / 180.0f) * dest_pointer->roll);
    xQuatFromMat(&destQuat, &finalMat);

    xMat3x3* curMat = xEntGetFrame(ent);
    xQuatFromMat(&startQuat, curMat);
}

void toss::Update(F32 dt)
{
    dist -= speed * dt;
    if (dist <= 0.0f) {
        dist = 0.0f;
        SetUsed(false);
        zEntEvent(toss_event_sender, ent, eEventStopMoving);
    }

    F32 pct = dist / totalDist;
    xVec3 newLoc;
    newLoc.AddScale(dest_pointer->loc, toDest, -pct);
    F32 ox = dist - peakDist;
    newLoc.y = dest_pointer->loc.y + arc_height - parabola_scale * ox * ox;
    *xEntGetPos(ent) = newLoc;

    xQuat curOrient;
    xQuatSlerp(&curOrient, &destQuat, &startQuat, pct);
    xEntGetFrame(ent);
}
} // namespace z
