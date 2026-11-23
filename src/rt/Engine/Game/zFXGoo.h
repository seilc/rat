#ifndef ZFXGOO_H
#define ZFXGOO_H

#include "types.h"

struct xModelInstance;
struct xEnt;

void zFXGooEnable(xModelInstance* gooModel, S32 freezeGroup);
void zFXGoo_SceneEnter();
void zFXGoo_SceneReset();
void zFXGoo_SceneExit();
void zFXGooUpdate(F32 dt);
S32 zFXGooIs(xEnt* obj, F32& depth, U32 playerCheck);

#endif
