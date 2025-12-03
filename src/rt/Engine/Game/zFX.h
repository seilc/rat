#ifndef ZFX_H
#define ZFX_H

#include "xFX.h"

extern U32 gFXSurfaceFlags;

void zFXModelRender();
void zFXPostModelRender();
void zFXScreenRender();
void zFXUpdate(F32 dt);
void zFX_SceneEnter(RpWorld* world);
void zFX_SceneExit(RpWorld* world);
void zFX_SceneReset();

#endif
