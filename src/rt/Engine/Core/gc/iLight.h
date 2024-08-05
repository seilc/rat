#ifndef ILIGHT_H
#define ILIGHT_H

#include "xMath3.h"
#include "xColor.h"

#include <rwcore.h>
#include <rpworld.h>

struct iLight
{
    U32 type;
    RpLight* hw;
    RpLightTie lightTie;
    xSphere sph;
    F32 radius_sq;
    xFColor color;
    xVec3 dir;
    F32 coneangle;
};

#endif
