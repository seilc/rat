#ifndef ZANIMCUSTOM_H
#define ZANIMCUSTOM_H

#include "types.h"

struct xModelInstance;

void zAnimCustomPlay(U32 id, xModelInstance* model, U32 outStateID, bool loopAnim, F32 inBlend, F32 outBlend,
                     F32 speed);

#endif
