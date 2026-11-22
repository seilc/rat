#ifndef ZFXSCREENDECAL_H
#define ZFXSCREENDECAL_H

#include "xMath3.h"

#include <rwcore.h>

class zFXScreenDecalConfig
{
public:
    F32 fadeInTime;
    F32 fadeOutTime;
    F32 lifetime;
    xFRect screenSpace;
    U32 textureID;
    RwRaster* raster;
    U8 alpha;

    void SetRasterFromID(); // UNUSED
#if DEBUG || RELEASE
    void AddTweaks(const char* prefix); // UNUSED
#endif
};

void zFXScreenDecalSceneEnter();
void zFXScreenDecalSceneReset();
void zFXScreenDecalUpdate(F32 dt);
bool zFXScreenDecalStart(const zFXScreenDecalConfig* config); // UNUSED
void zFXScreenDecalRender();

#endif
