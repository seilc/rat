#ifndef ZFXSHOCKWAVE_H
#define ZFXSHOCKWAVE_H

#include "types.h"

struct xVec3;
class zFXShockwaveConfig;

void zFXShockwaveSceneEnter(S32 maxInScene);
void zFXShockwaveSceneReset();
void zFXShockwaveUpdate(F32 dt);
void zFXShockwaveRender();
void zFXShockwaveSceneExit();
bool zFXShockwaveStart(zFXShockwaveConfig* config, const xVec3& pos, F32 scale, bool killproj);

#endif
