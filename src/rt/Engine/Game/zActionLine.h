#ifndef ZACTIONLINE_H
#define ZACTIONLINE_H

#include "xMath3.h"

typedef struct _tagActionLine ActionLine;

void zActionLineInit();
ActionLine* zActionLineAdd(xVec3* start, xMat3x3* mat, F32 len, F32 wid, F32 duration); // UNUSED
void zActionLineUpdate(F32 seconds);
void zActionLineDebugCB(); // UNUSED
void zActionLineRender();
void zActionLineKill(ActionLine* l); // UNUSED

#endif
