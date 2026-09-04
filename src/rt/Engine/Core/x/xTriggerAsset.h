#ifndef XTRIGGERASSET_H
#define XTRIGGERASSET_H

#include "xMath3.h"

struct xTriggerAsset
{
    xVec3 p[4];
    union
    {
        xVec3 direction;
        xVec3 safeAreaCenter;
        struct
        {
            U32 curveID;
            U32 cameraAID;
            U32 cameraBID;
        } camTransition;
    };
    U16 flags;
    U16 safeAreaRadius;
};

#endif
