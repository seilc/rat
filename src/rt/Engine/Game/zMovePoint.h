#ifndef ZMOVEPOINT_H
#define ZMOVEPOINT_H

#include "xMovePoint.h"

struct zMovePoint : xMovePoint
{
#if DEBUG || RELEASE
    S32 dbdraw_inprog;
#endif
};

xVec3* zMovePointGetPos(const zMovePoint* m);

#endif
