#include "xCommon.h"
#include "zFXScreenDecal.h"

#include "xEnt.h"
#include "xIM.h"
#include "xMath.h"
#include "xRenderState.h"
#include "xstransvc.h"
#include "zMain.h"

#include "decomp.h"

#define MAX_DECALS 8

namespace {

class Decal
{
public:
    const zFXScreenDecalConfig* config;
    F32 timer;

    void Render()
    {
        U32 alpha = config->alpha;
        F32 fadeInTime = config->lifetime - config->fadeInTime;

        if (timer >= fadeInTime && !xfeq0(config->fadeInTime)) {
            alpha = alpha * (1.0f + (fadeInTime - timer) / config->fadeInTime);
        }
        else if (timer <= config->fadeOutTime && !xfeq0(config->fadeOutTime)) {
            alpha = alpha * timer / config->fadeOutTime;
        }

        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, config->raster);
        xIM2DRenderQuad(config->screenSpace.x * FB_XRES, config->screenSpace.y * FB_YRES,
                        (config->screenSpace.x + config->screenSpace.w) * FB_XRES,
                        (config->screenSpace.y + config->screenSpace.h) * FB_YRES,
                        1.0f, 1.0f, alpha, 255, 255, 255);
    }
};

S32 decalCount;
Decal decals[MAX_DECALS];

} // namespace

#if DEBUG || RELEASE
DECOMP_FORCEFLOAT(0.0f);
DECOMP_FORCEFLOAT(1.0f);
#endif

void zFXScreenDecalSceneEnter()
{
    decalCount = 0;
}

void zFXScreenDecalSceneReset()
{
    decalCount = 0;
}

void zFXScreenDecalUpdate(F32 dt)
{
    S32 i = 0;
    while (i < decalCount) {
        decals[i].timer -= dt;
        if (decals[i].timer <= 0.0f) {
            decalCount--;
            decals[i] = decals[decalCount];
        } else {
            i++;
        }
    }
}

void zFXScreenDecalRender()
{
    for (S32 i = 0; i < decalCount; i++) {
        decals[i].Render();
    }
}
