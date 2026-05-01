#ifndef ZUIMOTION_H
#define ZUIMOTION_H

#include "xColor.h"

struct zUIMotionFrame
{
    F32 offsetX;
    F32 offsetY;
    F32 scaleX;
    F32 scaleY;
    F32 centerScaleX;
    F32 centerScaleY;
    F32 textScaleX;
    F32 textScaleY;
    F32 rotation;
    F32 offsetU;
    F32 offsetV;
    xColor color;
    U8 brightness;
};

#endif
