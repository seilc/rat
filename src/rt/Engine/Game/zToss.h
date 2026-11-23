#ifndef ZTOSS_H
#define ZTOSS_H

#include "zlightweight.h"
#include "xMath3.h"

struct xBase;
struct xEnt;
struct pointer_asset;

#define MAX_TOSSES 10

namespace z {
class toss : public zLightweight<toss, MAX_TOSSES>
{
    xEnt* ent;
    pointer_asset* dest_pointer;
    F32 arc_height;
    F32 speed;
    xBase* toss_event_sender;
    F32 peakDist;
    F32 totalDist;
    F32 parabola_scale;
    xVec3 toDest;
    xQuat startQuat;
    xQuat destQuat;
    F32 dist;

public:
    toss() : zLightweight<toss, MAX_TOSSES>(false) {}
    toss(xEnt* ent, pointer_asset* dest_pointer, F32 arc_height, F32 speed, F32, xBase* toss_event_sender);

    static void Setup();
    static toss* find(xEnt* ent);

    void Update(F32 dt);
};
} // namespace z

#endif
