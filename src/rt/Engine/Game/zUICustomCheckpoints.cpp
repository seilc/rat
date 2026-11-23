#include "xCommon.h"
#include "zUICustomCheckpoints.h"

#include "i2dQuadBuffer.h"
#include "xDebugTweak.h"
#include "xFont.h"
#include "xMath.h"
#include "xRenderBuffer.h"
#include "xRenderState.h"
#include "xstransvc.h"
#include "zMain.h"
#include "zRenderState.h"
#include "zUI.h"

#include <stdio.h>

#define MAX_CHECKPOINTS 8

namespace {

U32 sceneCheckPointTotal;
U32 sceneCheckPointHit;

} // namespace

void zUIIncrediblesSetSceneCheckPointHit(U32 value)
{
    sceneCheckPointHit = value;
}

U32 zUIIncrediblesGetSceneCheckPointHit()
{
    return sceneCheckPointHit;
}

U32 zUIIncrediblesGetSceneCheckPointTotal()
{
    return sceneCheckPointTotal;
}

zUICustomCheckpoints::zUICustomCheckpoints(zUI* ui) : zUICustom(ui)
{
}

void zUICustomCheckpoints::Reset()
{
    texture = (RwTexture*)xSTFindAsset(ui->asset->customWidget, NULL);
}

bool zUICustomCheckpoints::PreRender()
{
    if (sceneCheckPointTotal == 0) {
        return false;
    }

    RwRaster* raster = RwTextureGetRaster(texture);

    zRenderState(eRenderStateDefault);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA, FALSE, TRUE);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSU, (void*)rwTEXTUREADDRESSWRAP);
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSV, (void*)rwTEXTUREADDRESSWRAP);

    U32 effectiveCheckpoints = MIN(MAX_CHECKPOINTS, sceneCheckPointTotal);

    F32 widthPerItem = ui->current.width / MAX_CHECKPOINTS;
    i2dRect uv_rect;
    i2dRect location_rect;
    for (U32 i = 0; i < effectiveCheckpoints; i++) {
        location_rect.tlx = (ui->current.x + widthPerItem * i) * FB_XRES * (1.0f / 640.0f);
        location_rect.tly = (ui->current.y + (i >= sceneCheckPointHit ? 0.6f * ui->current.height : 0.0f)) * FB_YRES * (1.0f / 480.0f);
        location_rect.brx = location_rect.tlx + 0.75f * widthPerItem * FB_XRES * (1.0f / 640.0f);
        location_rect.bry = (ui->current.y + ui->current.height) * FB_YRES * (1.0f / 480.0f);

        uv_rect.tlu = 0.0f;
        uv_rect.tlv = i >= sceneCheckPointHit ? 0.75f : 1.0f;
        uv_rect.bru = 1.0f;
        uv_rect.brv = 1.0f;

        xColor rect_color = ui->current.color;
        gQuadBuffer->Set(&location_rect, &uv_rect, rect_color);
    }
    gQuadBuffer->Flush();

    return false;
}

#if DEBUG || RELEASE
void zUICustomCheckpoints::AddTweaks(const char* baseName)
{
    char buffer[128];

    sprintf(buffer, "%s|Hit Checkpoints", baseName);
    xTWEAK(buffer, &sceneCheckPointHit, 0, 100, NULL, NULL, 0x2);
    sprintf(buffer, "%s|Total Checkpoints", baseName);
    xTWEAK(buffer, &sceneCheckPointTotal, 0, 100, NULL, NULL, 0x2);
}
#endif
