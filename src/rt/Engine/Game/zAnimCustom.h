#ifndef ZANIMCUSTOM_H
#define ZANIMCUSTOM_H

#include "types.h"

struct xAnimFile;
struct xModelInstance;

void zAnimCustomInit(S32 count);
void zAnimCustomClear();
void zAnimCustomReset();
void zAnimCustomAdd(U32 id, xAnimFile* data, bool hasPhysics);
U32 zAnimCustomGetNextID(U32 previd); // UNUSED
bool zAnimCustomInAnimState(U32 id, xModelInstance* model); // UNUSED
void zAnimCustomPlay(U32 id, xModelInstance* model, U32 outStateID, bool loopAnim, F32 inBlend, F32 outBlend,
                     F32 speed);
void zAnimCustomUpdate();

#endif
