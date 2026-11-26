#ifndef ZRETICLE_H
#define ZRETICLE_H

#include "types.h"

struct xVec3;

F32 zIcon_GetNewAlpha(F32 currAlpha, F32 dt, const xVec3& pos, bool active, F32 radius);

#endif
