#ifndef XLIGHTVOLUME_H
#define XLIGHTVOLUME_H

#include "xColor.h"

#include <rpworld.h>

class xLightVolume
{
public:
    static void scene_enter();
    static void scene_exit();
    static void reset_all();

    void create();
    bool activate();
    void deactivate();

    virtual void render() = 0;
    virtual void render_immediate_pass();

    static void render_all();
    static void render_atomic(RpAtomic*, xColor, bool, U32);

private:
    struct activity_type;

    activity_type* activity;

    static activity_type* activities;
    static S32 activities_used;
};

#endif
