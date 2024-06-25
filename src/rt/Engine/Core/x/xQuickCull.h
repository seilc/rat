#ifndef XQUICKCULL_H
#define XQUICKCULL_H

#include "xMath3.h"

struct xQCData
{
    S8 xmin;
    S8 ymin;
    S8 zmin;
    S8 zmin_dup;
    S8 xmax;
    S8 ymax;
    S8 zmax;
    S8 zmax_dup;
    xVec3 min;
    xVec3 max;
};

#endif
