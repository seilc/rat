#include "xLightVolume.h"

#include "xMemMgr.h"
#include "xDebug.h"

#define MAX_ACTIVE 64

struct xLightVolume::activity_type
{
    xLightVolume* owner;
};

xLightVolume::activity_type* xLightVolume::activities = NULL;
S32 xLightVolume::activities_used = 0;

void xLightVolume::scene_enter()
{
    activities = new (eMemStaticType_0, eMemMgrTag_Light, 0) activity_type[MAX_ACTIVE];
    activities_used = 0;
    reset_all();
}

void xLightVolume::scene_exit()
{
}

void xLightVolume::reset_all()
{
    activity_type* act = activities;
    activity_type* end_act = act + activities_used;

    while (act != end_act) {
        xASSERT(56, act->owner != 0);
        act->owner->activity = 0;
        act++;
    }

    activities_used = 0;
}

void xLightVolume::create()
{
    activity = 0;
}

bool xLightVolume::activate()
{
    xASSERT(73, activity == 0);

#ifdef DEBUGRELEASE
    activity_type* act = activities;
    activity_type* end_act = act + activities_used;

    while (act != end_act) {
        xASSERT(80, (act->owner != this) && "Added light volume more than once.");
        act++;
    }
#endif

    if (activities_used >= MAX_ACTIVE) {
        return false;
    }

    activity = &activities[activities_used];
    activities_used++;
    activity->owner = this;

    return true;
}

void xLightVolume::deactivate()
{
    xASSERT(98, activity != 0);
    xASSERT(99, (activity >= activities) && (activity < (activities + activities_used)));
    xASSERT(100, activity->owner == this);

    activities_used--;

    activity_type* tail_activity = &activities[activities_used];

    if (activity != tail_activity) {
        *activity = *tail_activity;
        activity->owner->activity = activity;
    }

    activity = 0;
}

void xLightVolume::render_all()
{
}

void xLightVolume::render_atomic(RpAtomic*, xColor, bool, U32)
{
}

#ifndef NON_MATCHING
void xLightVolumeModel::render_single(xModelInstance* model, U32)
{
    xASSERT(0, model != 0);
    xLightVolume::render_immediate(g_BLACK, false);
    xMat3x3MulScale(NULL, NULL, NULL);
}
#endif
