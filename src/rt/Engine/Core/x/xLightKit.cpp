#include "xLightKit.h"

#include "iAsync.h"
#include "xDebug.h"
#include "xMath.h"
#include "xModel.h"

#include <string.h>

xLightKitBlendInfo gLastLightKit;

#ifndef NON_MATCHING
void xLightKit_Parse(char*, U32, U32*, U32, U32)
{
    xLightKit* lightKit = NULL;
    *(F32*)0 = 0.0f;

    iprintf("\n\r");
    iprintf("intensity=");
    iprintf("color=");
    xASSERT(0, lightKit);
    iprintf("%f");
    iprintf("%f,%f,%f");
    iprintf("matrix=");
    iprintf("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f");
    iprintf("radius=");
    iprintf("angle=");
}
#endif

xLightKit* xLightKit_Prepare(void* data)
{
    iAsyncLockRWSentry rw_sentry;

    xLightKit* lkit = (xLightKit*)data;
    xLightKitLight* currlight;
    RwUInt32 i, numdir = 0, numamb = 0;

    currlight = lkit->lightList = (xLightKitLight*)(lkit+1);
    for (i = 0; i < lkit->lightCount; i++) {
        if (currlight->platLight) {
            iprintf("WARNING:  Light kit previously initalized!!!\n");
            return lkit;
        }

        xASSERT(258, currlight->color.red >= 0.0f);
        xASSERT(259, currlight->color.green >= 0.0f);
        xASSERT(260, currlight->color.blue >= 0.0f);

        if (currlight->color.red > 1.0f || currlight->color.green > 1.0f || currlight->color.blue > 1.0f) {
            iprintf("BOGUS LIGHT COLOR:  % 5.2f % 5.2f % 5.2f\n",
                    currlight->color.red, currlight->color.green, currlight->color.blue);
            
            RwReal s = xmax3(currlight->color.red, currlight->color.green, currlight->color.blue);
            s = xmax(s, 0.00001f);
            s = 1.0f / s;

            currlight->color.red *= s;
            currlight->color.green *= s;
            currlight->color.blue *= s;

            iprintf("corrected to:       % 5.2f % 5.2f % 5.2f\n",
                    currlight->color.red, currlight->color.green, currlight->color.blue);
        }

        switch (currlight->type) {
        case 1:
            currlight->platLight = RpLightCreate(rpLIGHTAMBIENT);
            numamb++;
            break;
        case 2:
            currlight->platLight = RpLightCreate(rpLIGHTDIRECTIONAL);
            numdir++;
            break;
        case 3:
            currlight->platLight = RpLightCreate(rpLIGHTPOINT);
            break;
        case 4:
            currlight->platLight = RpLightCreate(rpLIGHTSPOTSOFT);
            break;
        }

        xASSERT(296, currlight->platLight);

        RpLightSetColor(currlight->platLight, &currlight->color);

        if (currlight->type >= 2) {
            RwFrame* frame = RwFrameCreate();

            RwMatrix tmpmat;
            memset(&tmpmat, 0, sizeof(tmpmat));

            tmpmat.right.x = -currlight->matrix[0];
            tmpmat.right.y = -currlight->matrix[1];
            tmpmat.right.z = -currlight->matrix[2];
            tmpmat.up.x = currlight->matrix[4];
            tmpmat.up.y = currlight->matrix[5];
            tmpmat.up.z = currlight->matrix[6];
            tmpmat.at.x = -currlight->matrix[8];
            tmpmat.at.y = -currlight->matrix[9];
            tmpmat.at.z = -currlight->matrix[10];
            tmpmat.pos.x = currlight->matrix[12];
            tmpmat.pos.y = currlight->matrix[13];
            tmpmat.pos.z = currlight->matrix[14];

            RwV3dNormalize(&tmpmat.right, &tmpmat.right);
            RwV3dNormalize(&tmpmat.up, &tmpmat.up);
            RwV3dNormalize(&tmpmat.at, &tmpmat.at);

            RwFrameTransform(frame, &tmpmat, rwCOMBINEREPLACE);
            RpLightSetFrame(currlight->platLight, frame);
        }

        if (currlight->type >= 3) {
            RpLightSetRadius(currlight->platLight, currlight->radius);
        }

        if (currlight->type >= 4) {
            RpLightSetConeAngle(currlight->platLight, currlight->angle);
        }

        currlight++;
    }

    lkit->blended = FALSE;

    xASSERTM(340, (numdir<=4)&&(numamb<=1), "ERROR:  Light kit with too many lights  %d dir, %d ambient!\n", numdir, numamb);

    return lkit;
}

void xLightKit_GetBlendInfo(xLightKitBlendInfo& lkitInfo, xModelInstance* model)
{
    lkitInfo.lightKit = model->LightKit;
    lkitInfo.lightKitBlend = model->LightKitBlend;
    lkitInfo.blendPercentage = model->BlendTimeRemaining / model->BlendDuration;
}

S32 xLightKit_Compare(xLightKitBlendInfo& lkitInfo1, xLightKitBlendInfo& lkitInfo2)
{
    if (lkitInfo1.lightKit < lkitInfo2.lightKit) return 1;
    if (lkitInfo1.lightKit > lkitInfo2.lightKit) return -1;
    if (lkitInfo1.lightKitBlend < lkitInfo2.lightKitBlend) return 1;
    if (lkitInfo1.lightKitBlend > lkitInfo2.lightKitBlend) return -1;
    if (lkitInfo1.blendPercentage < lkitInfo2.blendPercentage) return 1;
    if (lkitInfo1.blendPercentage > lkitInfo2.blendPercentage) return -1;
    return 0;
}

void xLightKit_Enable(const xModelInstance& model, RpWorld* world)
{
    if (model.LightKitBlend) {
        xASSERT(441, model.BlendDuration);
        xLightKit_Enable(model.LightKit, world, model.LightKitBlend, model.BlendTimeRemaining / model.BlendDuration);
    } else {
        xLightKit_Enable(model.LightKit, world, NULL, 0.0f);
    }
}

void xLightKit_Enable(xLightKit* lkit, RpWorld* world, xLightKit* lkit_blend, RwReal blendPercentage)
{
    RwUInt32 i;

    if (lkit != gLastLightKit.lightKit) {
        if (gLastLightKit.lightKit) {
            for (i = 0; i < gLastLightKit.lightKit->lightCount; i++) {
                RpWorldRemoveLight(world, gLastLightKit.lightKit->lightList[i].platLight);
            }
        }

        gLastLightKit.lightKit = lkit;
        gLastLightKit.lightKitBlend = NULL;
        gLastLightKit.blendPercentage = 0.0f;

        if (lkit) {
            for (i = 0; i < lkit->lightCount; i++) {
                RpWorldAddLight(world, lkit->lightList[i].platLight);
            }
        }
    }

    if (lkit_blend) {
        if (gLastLightKit.blendPercentage != blendPercentage && gLastLightKit.lightKitBlend != lkit_blend) {
            xASSERT(491, blendPercentage >= 0.0f);
            xASSERT(492, blendPercentage < 1.0f);

            for (i = 0; i < lkit->lightCount; i++) {
                RwRGBAReal blendedColor;
                blendedColor.alpha = 1.0f;
                blendedColor.red = (1.0f - blendPercentage) * lkit->lightList[i].color.red +
                                blendPercentage * lkit_blend->lightList[i].color.red;
                blendedColor.green = (1.0f - blendPercentage) * lkit->lightList[i].color.green +
                                    blendPercentage * lkit_blend->lightList[i].color.green;
                blendedColor.blue = (1.0f - blendPercentage) * lkit->lightList[i].color.blue +
                                    blendPercentage * lkit_blend->lightList[i].color.blue;
                RpLightSetColor(lkit->lightList[i].platLight, &blendedColor);
            }

            lkit->blended = TRUE;

            gLastLightKit.lightKitBlend = lkit_blend;
            gLastLightKit.blendPercentage = blendPercentage;
        }
    } else if (lkit && lkit->blended) {
        for (i = 0; i < lkit->lightCount; i++) {
            RpLightSetColor(lkit->lightList[i].platLight, &lkit->lightList[i].color);
        }
        lkit->blended = FALSE;
    }
}

xLightKitBlendInfo* xLightKit_GetCurrent(RpWorld*)
{
    return &gLastLightKit;
}

void xLightKit_Destroy(xLightKit* lkit)
{
    RwUInt32 i;
    xLightKitLight* currLight;

    if (!lkit) return;

    currLight = lkit->lightList;
    for (i = 0; i < lkit->lightCount; i++) {
        if (currLight->platLight) {
            _rwFrameSyncDirty();

            RwFrame* tframe = RpLightGetFrame(currLight->platLight);
            if (tframe) {
                RpLightSetFrame(currLight->platLight, NULL);
                RwFrameDestroy(tframe);
            }

            RpLightDestroy(currLight->platLight);
            currLight->platLight = NULL;
        }

        currLight++;
    }
    
}
