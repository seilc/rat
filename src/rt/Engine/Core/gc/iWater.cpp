#include "xCommon.h"
#include "iWater.h"

#include "xCam.h"
#include "xDebugTweak.h"
#include "xDraw.h"
#include "xEnt.h"
#include "xFX.h"
#include "xLightKit.h"
#include "xMath.h"
#include "xMath3.h"
#include "xModel.h"
#include "xRenderState.h"
#include "xShadow.h"
#include "xstransvc.h"
#include "xutil.h"
#include "xVec3.h"
#include "iCamera.h"
#include "iDebug.h"
#include "iModel.h"
#include "zHashes.h"
#include "zBase.h"
#include "zEntSimpleObj.h"
#include "zGame.h"
#include "zGlobals.h"
#include "zPlayer.h"
#include "zScene.h"
#include "zWater.h"
#include "water/fxembm.h"
#include "water/texconv.h"
#include "water/textures.h"

#include <rwcore.h>
#include <rpworld.h>
#include <rpmatfx.h>
#include <stdio.h>

#define WATER_PRESET(width, shininess, height, frame)                                             \
{                                                                                                 \
    BumpWidth = (width);                                                                          \
    Shininess = (shininess);                                                                      \
    BumpHeight = (height);                                                                        \
    water_frame = (frame);                                                                        \
}

struct bump_layer
{
    F32 u;
    F32 v;
    F32 us;
    F32 vs;
    F32 scale;
    U8 alpha;
    U8 pad[3];
};

S32 gGCwaterDisable;
RpLight* iWaterDirectionalLight;
FxParam* iWaterEMBM;
xEnt* waterObjects[8];
U32 numWaterObjects;
xEnt* water2;
xEnt* water3;
xEnt* water4;
xEnt* water5;
xEnt* water7;
xTWEAKABLE F32 frameCountScale = 0.2f;
xTWEAKABLE S32 bump_mode = 1;
xTWEAKABLE U8 dff_alpha = 128;
xTWEAKABLE S32 env_clamp = FALSE;
xTWEAKABLE S32 override_width = TRUE;
S32 water_frame = 2;
xTWEAKABLE F32 yscale = 1.0f;
xVec3 camscale = { 0.4f, -1.0f, -0.5f };
static RwTexture* saveBump;
xTWEAKABLE bump_layer bump_layers[4] = {
    { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0x60 },
    { 0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 0x40 },
    { 0.25f, 0.1f, 1.0f, -1.0f, 3.0f, 0x20 },
    { 0.69f, 0.47f, 1.0f, 1.0f, 4.0f, 0x40 },
};
xTWEAKABLE S32 num_bump_layers = 4;
static RwTexture* bumpTex;
static RwRaster* bumpRaster;

static RpLight* CreateDirectionalLight()
{
    RpLight* light = RpLightCreate(rpLIGHTDIRECTIONAL);
    if (light) {
        RwFrame* frame = RwFrameCreate();
        if (frame) {
            RwRGBAReal color;

            color.red = color.green = color.blue = 1.0f;
            color.alpha = 1.0f;

            RpLightSetColor(light, &color);
            RpLightSetFrame(light, frame);

            return light;
        }

        RpLightDestroy(light);
    }

    return NULL;
}

static RwCamera* saveCamera;
static RwCamera* bumpCamera;
static RwCamera* clonedShadowCamera;

static void iWaterBumpGenRenderLayer(RwCamera* camera, RwTexture* texture, F32 u, F32 v, U8 alpha, F32 scale)
{
    RwIm2DVertex vx[4];
    F32 crw, crh;
    RwRaster* camRas;
    F32 recipZ;

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)RwTextureGetRaster(texture));

    camRas = RwCameraGetRaster(camera);
    crw = RwRasterGetWidth(camRas);
    crh = RwRasterGetHeight(camRas);

    recipZ = 1.0f / RwCameraGetNearClipPlane(camera);

    RwIm2DVertexSetScreenX(&vx[0], 0.0f);
    RwIm2DVertexSetScreenY(&vx[0], 0.0f);
    RwIm2DVertexSetScreenZ(&vx[0], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetRecipCameraZ(&vx[0], recipZ);
    RwIm2DVertexSetIntRGBA(&vx[0], alpha, alpha, alpha, 255);
    RwIm2DVertexSetU(&vx[0], u, recipZ);
    RwIm2DVertexSetV(&vx[0], v, recipZ);

    RwIm2DVertexSetScreenX(&vx[1], 0.0f);
    RwIm2DVertexSetScreenY(&vx[1], crh);
    RwIm2DVertexSetScreenZ(&vx[1], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetRecipCameraZ(&vx[1], recipZ);
    RwIm2DVertexSetIntRGBA(&vx[1], alpha, alpha, alpha, 255);
    RwIm2DVertexSetU(&vx[1], u, recipZ);
    RwIm2DVertexSetV(&vx[1], v + scale, recipZ);

    RwIm2DVertexSetScreenX(&vx[2], crw);
    RwIm2DVertexSetScreenY(&vx[2], 0.0f);
    RwIm2DVertexSetScreenZ(&vx[2], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetRecipCameraZ(&vx[2], recipZ);
    RwIm2DVertexSetIntRGBA(&vx[2], alpha, alpha, alpha, 255);
    RwIm2DVertexSetU(&vx[2], u + scale, recipZ);
    RwIm2DVertexSetV(&vx[2], v, recipZ);

    RwIm2DVertexSetScreenX(&vx[3], crw);
    RwIm2DVertexSetScreenY(&vx[3], crh);
    RwIm2DVertexSetScreenZ(&vx[3], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetRecipCameraZ(&vx[3], recipZ);
    RwIm2DVertexSetIntRGBA(&vx[3], alpha, alpha, alpha, 255);
    RwIm2DVertexSetU(&vx[3], u + scale, recipZ);
    RwIm2DVertexSetV(&vx[3], v + scale, recipZ);

    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, vx, 4);
}

static S32 iWaterBumpGenBegin(RwCamera* bumpCamera, RwRGBA* bgColor)
{
    saveCamera = RwCameraGetCurrentCamera();
    if (saveCamera) {
        RwCameraEndUpdate(saveCamera);
    }

    if (bgColor) {
        RwCameraClear(bumpCamera, bgColor, rwCAMERACLEARIMAGE);
    }

    if (!RwCameraBeginUpdateWrapper(bumpCamera)) {
        return FALSE;
    }

    GXSetDither(GX_FALSE);

    return TRUE;
}

static S32 iWaterUpdateEMBM(F32, S32 check);

static void iWaterBumpGenEnd()
{
    GXSetDither(GX_TRUE);

    RwCameraEndUpdate(bumpCamera);

    RwGameCubeCameraTextureFlush(RwCameraGetRaster(bumpCamera), FALSE);
    GXPixModeSync();

    if (saveCamera) {
        RwCameraBeginUpdateWrapper(saveCamera);
    }

    iWaterUpdateEMBM(xglobals->update_dt, FALSE);
}

static F32 foo;

static RwTexture* iWaterBumpGen(RwTexture* bump, RwTexture* src)
{
    bool genNeeded = false;

    for (S32 i = 0; i < numWaterObjects; i++) {
        if (waterObjects[i]->model && xEntIsVisible(waterObjects[i])) {
            xModelInstance* modelInst = waterObjects[i]->model;
            while (modelInst) {
                if ((modelInst->Flags & (ModelInstance_Render | ModelInstance_NoRender)) == ModelInstance_Render)
                    genNeeded = true;
                modelInst = modelInst->Next;
            }
        }
    }

    if (!genNeeded) {
        return NULL;
    }

    if (!bump) {
        return NULL;
    }

    if (!src) {
        return NULL;
    }

    foo += sTimeElapsed * frameCountScale;
    foo = xfmod(foo, 1.0f);

    RwRGBA black = { 0, 0, 0, 255 };

    if (!clonedShadowCamera) {
        clonedShadowCamera = RwCameraClone(ShadowCamera);
        RwCameraSetRaster(clonedShadowCamera, bumpRaster);
    }

    bumpCamera = clonedShadowCamera;

    if (iWaterBumpGenBegin(bumpCamera, &black)) {
        xTWEAKABLE bump_layer* bp = bump_layers;
        for (S32 i = 0; i < num_bump_layers; i++, bp++) {
            if (bp->alpha) {
                iWaterBumpGenRenderLayer(bumpCamera, src, bp->u + bp->us * foo, bp->v + bp->vs * foo, bp->alpha, bp->scale);
            }
        }

        iWaterBumpGenEnd();
    }

    return bump;
}

void iWaterBumpGen()
{
    if (bump_mode == 1)
        iWaterBumpGen(bumpTex, iWaterBumpTex);
}

void iWaterCreateMTEffect()
{
    if (!gGCwaterDisable)
        FxOpen();
}

static xLightKit* waterLightKit;
static xTWEAKABLE RwRGBAReal waterHackColor = { 1.0f, 1.0f, 1.0f, 1.0f };

static void iWaterObjectRender(xEnt* ent)
{
    if (!ent->model || !xEntIsVisible(ent)) {
        return;
    }

    if (waterLightKit)
        xModelSetLightKit(ent->model, waterLightKit, 0.0f);
    ent->model->RedMultiplier = waterHackColor.red;
    ent->model->GreenMultiplier = waterHackColor.green;
    ent->model->BlueMultiplier = waterHackColor.blue;
    ent->model->Alpha = waterHackColor.alpha;

    xModelRender(ent->model);
}

F32 bw_factor = 0.001f;
static RpAtomicCallBackRender defaultRenderCB;

static RpAtomic* iWaterAtomicRender(RpAtomic* atomic)
{
    return defaultRenderCB(atomic);
}

U32 waterTexID = HASH_Griver_green_mt;

#pragma push
#pragma optimization_level 0
static void iWaterObjectSetup(xEnt* ent)
{
    RpAtomic* atomic = ent->model->Data;
    RpGeometry* geom = RpAtomicGetGeometry(atomic);

    RwTexture* newtex = (RwTexture*)xSTFindAsset(waterTexID, NULL);
    if (newtex) {
        for (S32 i = 0; i < geom->matList.numMaterials; i++) {
            RpMaterial* material = geom->matList.materials[i];
            RwTexture* tex = RpMaterialGetTexture(material);
            if (tex != newtex) {
                PKRAssetTOCInfo ainfo;
                xSTRandMemToAsset(&ainfo, tex);

#if DEBUG || RELEASE
                OSReport("### water material %d texture:  %s => %s\n", i, xSTAssetName(ainfo.aid), xSTAssetName(waterTexID));
#endif

                RpMaterialSetTexture(material, newtex);
            }
        }
    }
}
#pragma pop

void iWaterSceneEnter(RpWorld*)
{
    zScene& scene = *globals.sceneCur;
    zWaterBody* it = (zWaterBody*)scene.baseList[eBaseTypeWaterBody], *end = it + scene.baseCount[eBaseTypeWaterBody];

    numWaterObjects = 0;
    for (; it < end; ++it) {
        U32 body = it->get_bodyID();
        if (body == HASH_S02_GOO_SIMP)
            body = HASH_S02_GOO_SIMP_GC;
        else if (body == HASH_S02_GOO_SIMP_B)
            body = HASH_S02_GOO_SIMP_B_GC;
        else if (body == HASH_S04_GOO_SIMP)
            body = HASH_S04_GOO_SIMP_GC;
        zEntSimpleObj* ent = (zEntSimpleObj*)zSceneFindObject(body);
        iprintf("iWaterSceneEnter:  water body %08X (%s)\n", body, xSTAssetName(body));
        if (ent) {
            waterObjects[numWaterObjects] = ent;
            numWaterObjects++;
        }
    }

    if (numWaterObjects == 0) {
        gGCwaterDisable = TRUE;
        return;
    }

    water2 = NULL;
    water3 = NULL;
    water4 = NULL;
    water5 = NULL;
    water7 = NULL;

    S32 flags = rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888 | rwRASTERFORMATAUTOMIPMAP | rwRASTERFORMATMIPMAP;

    bumpRaster = RwRasterCreate(64, 64, 32, flags);
    if (bumpRaster)
        bumpTex = RwTextureCreate(bumpRaster);
    saveBump = NULL;

    iWaterDirectionalLight = CreateDirectionalLight();
    if (!iWaterDirectionalLight) {
        return;
    }

    iWaterCreateMTEffect();

    for (S32 i = 0; i < numWaterObjects; i++) {
        xEnt* ent = waterObjects[i];

        if (!ent || !ent->model || !ent->model->Mat) {
            continue;
        }

        RwMatrix newMat;
        newMat = *ent->model->Mat;
        RwMatrixOrthoNormalize(&newMat, ent->model->Mat);
        *ent->model->Mat = newMat;

        iWaterObjectSetup(ent);
    }

#if DEBUG || RELEASE
    xTWEAK("FX|GC Water|Bump width", &BumpWidth, 0.0f, 20.0f, NULL, NULL, 0);
    xTWEAK("FX|GC Water|Bump height", &BumpHeight, 0.0f, 20.0f, NULL, NULL, 0);
    xTWEAK("FX|GC Water|Shininess", &Shininess, 0.0f, 5.0f, NULL, NULL, 0);
    xTWEAK("FX|GC Water|Env mode", &EnvMode, 0, 1, NULL, NULL, 0);
    xTWEAK("FX|GC Water|BW factor", &bw_factor, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("FX|GC Water|Bump mode", &bump_mode, 0, 1, NULL, NULL, 0);
    xTWEAK("FX|GC Water|Count scale", &frameCountScale, -5.0f, 5.0f, NULL, NULL, 0);
    xTWEAK("FX|GC Water|Alpha", &dff_alpha, 0, 255, NULL, NULL, 0);
    xTWEAK("FX|GC Water|Env clamp", &env_clamp, 0, 1, NULL, NULL, 0);
    xTWEAK("FX|GC Water|Override Width", &override_width, 0, 1, NULL, NULL, 0);
    xTWEAK("FX|GC Water|Water Frame", &water_frame, 0, 3, NULL, NULL, 0);
    xTWEAK("FX|GC Water|Y Scale", &yscale, 0.0f, 10.0f, NULL, NULL, 0);
    xTWEAK("FX|GC Water|Cam Scale X", &camscale.x, -10.0f, 10.0f, NULL, NULL, 0);
    xTWEAK("FX|GC Water|Cam Scale Y", &camscale.y, -10.0f, 10.0f, NULL, NULL, 0);
    xTWEAK("FX|GC Water|Cam Scale Z", &camscale.z, -10.0f, 10.0f, NULL, NULL, 0);

    char pfx[64], str[64];
    bump_layer* bp = bump_layers;
    for (S32 i = 0; i < num_bump_layers; i++, bp++) {
        sprintf(pfx, "FX|GC Water|Layer %d", i);

        sprintf(str, "%s|U", pfx);
        xTWEAK(str, &bp->u, 0.0f, 1.0f, NULL, NULL, 0x2);
        sprintf(str, "%s|V", pfx);
        xTWEAK(str, &bp->v, 0.0f, 1.0f, NULL, NULL, 0x2);

        sprintf(str, "%s| U scale", pfx);
        xTWEAK(str, &bp->us, -5.0f, 5.0f, NULL, NULL, 0x2);
        sprintf(str, "%s| V scale", pfx);
        xTWEAK(str, &bp->vs, -5.0f, 5.0f, NULL, NULL, 0x2);

        sprintf(str, "%s|Scale", pfx);
        xTWEAK(str, &bp->scale, -16.0f, 16.0f, NULL, NULL, 0x2);

        sprintf(str, "%s|Alpha", pfx);
        xTWEAK(str, &bp->alpha, 0, 255, NULL, NULL, 0x2);
    }
#endif

    iWaterReset();

    switch (globals.sceneCur->sceneID) {
    case IDTAG('M','T','0','1'):
        WATER_PRESET(0.08f, 0.17f, 1.0f, 1);
        water2 = (xEnt*)zSceneFindObject(xStrHash("S02_GOO_SIMP_GC"));
        water3 = (xEnt*)zSceneFindObject(xStrHash("S03_GOO_SIMP"));
        water4 = (xEnt*)zSceneFindObject(xStrHash("S04_GOO_SIMP_GC"));
        water5 = (xEnt*)zSceneFindObject(xStrHash("S05_GOO_SIMP"));
        water7 = (xEnt*)zSceneFindObject(xStrHash("S07_GOO_SIMP"));
        break;
    case IDTAG('R','F','0','1'):
        WATER_PRESET(0.095f, 0.3f, 1.0f, 2);
        break;
    case IDTAG('D','D','0','1'):
        WATER_PRESET(0.02f, 0.2f, 1.0f, 1);
        break;
    }
}

void iWaterSceneExit()
{
    if (!gGCwaterDisable) {
        FxClose();
    }

    gGCwaterDisable = FALSE;
    waterLightKit = NULL;
    numWaterObjects = 0;

    if (clonedShadowCamera) {
        RwCameraDestroy(clonedShadowCamera);
        clonedShadowCamera = NULL;
        bumpCamera = NULL;
    }

    if (bumpTex) {
        RwTextureDestroy(bumpTex);
        bumpTex = NULL;
    }

    if (iWaterDirectionalLight) {
        RwFrame* frame = RpLightGetFrame(iWaterDirectionalLight);

        _rwFrameSyncDirty();
        if (frame) {
            RwFrameDestroy(frame);
        }

        RpLightDestroy(iWaterDirectionalLight);
        iWaterDirectionalLight = NULL;
    }
}

static S32 iWaterUpdateEMBM(F32, S32 check)
{
    if (!check) {
        RpMultiTexture* mt;
        RpGameCubeMTConfig* config;
        U32 rasSize;
        F32 scale;

        RwTexture* bump = NULL;
#if DEBUG || RELEASE
        if (!bumpTex) {
            bump_mode = 0;
        }
#endif

        RwTexture* env = iWaterEnvTex;
        if (bump_mode == 0) {
            bump = iWaterBumpTex;
        } else if (bump_mode == 1) {
            bump = bumpTex;
        }

        static xTWEAKABLE U32 override_bump = 0;
        static xTWEAKABLE U32 override_env = 0;
        static xTWEAKABLE F32 uvscale = 1280.0f;
        static xTWEAKABLE F32 range_scale = 1.0f;
#if DEBUG || RELEASE
        static bool render_texture = false;
        xTEMPTWEAKS_BEGIN(prefix)
            prefix = "FX|GC Water|";
            xAUTOTWEAK_SELECT(prefix, "override_bump", &override_bump, all_texture_labels, all_texture_ids, all_texture_count, NULL, NULL, 0, true);
            xAUTOTWEAK_SELECT(prefix, "override_env", &override_env, all_texture_labels, all_texture_ids, all_texture_count, NULL, NULL, 0, true);
            xAUTOTWEAK_BOOL(prefix, "render_texture", &render_texture, NULL, NULL, 0, true);
            xAUTOTWEAK(prefix, "uvscale", &uvscale, -1000000000.0f, 1000000000.0f, NULL, NULL, 0, true);
            xAUTOTWEAK(prefix, "range_scale", &range_scale, 0.0f, 1.0f, NULL, NULL, 0, true);
        xTEMPTWEAKS_END

        if (render_texture) {
            xDrawCard(RwTextureGetRaster(bumpTex), 50.0f, 50.0f, 128.0f, 128.0f, TRUE);
        }
#endif

        if (override_bump) {
            bump = (RwTexture*)xSTFindAsset(override_bump, NULL);
        }

        if (override_env) {
            env = (RwTexture*)xSTFindAsset(override_env, NULL);
        }

        if (!saveBump) {
            if (bump == bumpTex) {
                TextureDiffBumpRGBA8(bumpTex, range_scale);
            } else {
                TextureDiffBumpRGBA8(bump, bumpTex);
                bump = bumpTex;
            }

            saveBump = bump;
        } else {
            bump = saveBump;
        }

        if (!env || !bump)
            return FALSE;
        RwTextureSetFilterMode(bump, rwFILTERLINEARMIPLINEAR);
        RwTextureSetAddressing(bump, rwTEXTUREADDRESSWRAP);
        RwTextureSetFilterMode(env, rwFILTERLINEARMIPLINEAR);
        if (env_clamp)
            RwTextureSetAddressing(env, rwTEXTUREADDRESSCLAMP);
        else
            RwTextureSetAddressing(env, rwTEXTUREADDRESSWRAP);

        mt = RpMaterialGetMultiTexture(EMBM.material, rwID_GAMECUBE);
        config = RpGameCubeMTEffectGetConfig(EMBM.effect);
        RpMultiTextureSetTexture(mt, 1, bump);
        RpMultiTextureSetTexture(mt, 2, env);
        rasSize = RwRasterGetWidth(RwTextureGetRaster(bump));

        scale = BumpWidth * (uvscale / rasSize);

        config->texMtx[0].data[0][0] = scale;
        config->texMtx[0].data[1][1] = scale;
        config->texMtx[1].data[0][0] = 0.5f * -BumpHeight;
        config->texMtx[1].data[1][1] = 0.5f * -BumpHeight;
        config->kreg[0].alpha = 255.0f * Shininess;

        RwFrame* frame = RpLightGetFrame(iWaterDirectionalLight);
        frame->ltm.flags = 0;

        xVec3 at;
        switch (water_frame) {
        case 0:
        default:
            FxFrameArray[0] = frame;
            config->texMtx[1].refFrame = rpGAMECUBETEXFRAME_CAMERA;
            config->texMtx[2].refFrame = rpGAMECUBETEXFRAME_CAMERA;
            break;
        case 1:
            xMat3x3LookVec((xMat3x3*)RwFrameGetLTM(frame), &g_Y3);
            FxFrameArray[0] = frame;
            config->texMtx[1].refFrame = rpGAMECUBETEXFRAME_MISC0;
            config->texMtx[2].refFrame = rpGAMECUBETEXFRAME_MISC0;
            break;
        case 2:
            xVec3Sub(&at, &g_Y3, (xVec3*)&xglobals->screen->icam->viewMatrix.at);
            xVec3SMul(&at, &at, 0.5f);
            xVec3Normalize(&at, &at);
            xMat3x3LookVec((xMat3x3*)RwFrameGetLTM(frame), &at);
            FxFrameArray[0] = frame;
            config->texMtx[1].refFrame = rpGAMECUBETEXFRAME_MISC0;
            config->texMtx[2].refFrame = rpGAMECUBETEXFRAME_MISC0;
            break;
        case 3:
            at = g_O3;
            at.AddScale(g_Y3, yscale);
            at.AddScale(*(xVec3*)&xglobals->screen->icam->viewMatrix.right, camscale.x);
            at.AddScale(*(xVec3*)&xglobals->screen->icam->viewMatrix.up, camscale.y);
            at.AddScale(*(xVec3*)&xglobals->screen->icam->viewMatrix.at, camscale.z);
            xVec3Normalize(&at, &at);
            xMat3x3LookVec((xMat3x3*)RwFrameGetLTM(frame), &at);
            FxFrameArray[0] = frame;
            config->texMtx[1].refFrame = rpGAMECUBETEXFRAME_MISC0;
            config->texMtx[2].refFrame = rpGAMECUBETEXFRAME_MISC0;
            break;
        }

        FxFrameArray[1] = NULL;
        RpGameCubeMTSetMiscFrameArray(FxFrameArray, NUM_FX_FRAMES);

        if (EnvMode == 0) {
            config->tevStages[3].op.colorA = rpGX_CC_CPREV;
            config->tevStages[3].op.colorB = rpGX_CC_TEXC;
            config->tevStages[3].op.colorC = rpGX_CC_KONST;
            config->tevStages[3].op.colorD = rpGX_CC_ZERO;

            config->tevStages[2].op.colorSel = rpGX_TEV_KCSEL_1;
            config->tevStages[2].op.colorScale = rpGX_CS_SCALE_1;
        } else {
            config->tevStages[3].op.colorA = rpGX_CC_ZERO;
            config->tevStages[3].op.colorB = rpGX_CC_TEXC;
            config->tevStages[3].op.colorC = rpGX_CC_CPREV;
            config->tevStages[3].op.colorD = rpGX_CC_ZERO;

            config->tevStages[2].op.colorSel = rpGX_TEV_KCSEL_K0_A;
            config->tevStages[2].op.colorScale = rpGX_CS_SCALE_4;
        }
    }

    return TRUE;
}

U32 enableWater = TRUE;

void iWaterEnable(RpAtomic* atomic)
{
    if (gGCwaterDisable) {
        return;
    }

    if (enableWater) {
        AtomicSetupEffect(atomic, iWaterEMBM->material);
    } else {
        AtomicDisableMatFX(atomic);
    }
}

void iWaterUpdate(F32)
{
    if (gGCwaterDisable) {
        return;
    }

    if (override_width) {
        if (globals.sceneCur->sceneID == IDTAG('M','T','0','1')) {
            xVec3 playerPos = *xEntGetPos(xglobals->players[0]);
            F32 dist2 = xVec3Dist2(&playerPos, xEntGetPos(water2));
            F32 dist3 = xVec3Dist2(&playerPos, xEntGetPos(water3));
            F32 dist4 = xVec3Dist2(&playerPos, xEntGetPos(water4));
            F32 dist5 = xVec3Dist2(&playerPos, xEntGetPos(water5));
            F32 dist7 = xVec3Dist2(&playerPos, xEntGetPos(water7));

            S32 closestWater = 2;
            F32 closestDist = dist2;

            if (dist3 < dist2) {
                closestWater = 3;
                closestDist = dist3;
            }

            if (dist4 < closestDist) {
                closestWater = 4;
                closestDist = dist4;
            }

            if (dist5 < closestDist + 1600.0f) {
                closestWater = 5;
                closestDist = dist5;
            }

            if (dist7 < closestDist + 6400.0f) {
                closestWater = 7;
            }

            switch (closestWater) {
            case 2:
                WATER_PRESET(0.05f, 0.13f, 1.0f, 1);
                break;
            case 3:
                WATER_PRESET(0.08f, 0.17f, 1.0f, 2);
                break;
            case 4:
                WATER_PRESET(0.08f, 0.17f, 1.0f, 2);
                break;
            case 5:
                WATER_PRESET(0.08f, 0.17f, 1.0f, 3);
                break;
            case 7:
            default:
                WATER_PRESET(0.08f, 0.17f, 1.0f, 1);
                break;
            }
        }
    }

    for (S32 i = 0; i < numWaterObjects; i++) {
        iModelSetMaterialAlpha(waterObjects[i]->model->Data, dff_alpha);
        iWaterEnable(waterObjects[i]->model->Data);
        iWaterObjectSetup(waterObjects[i]);
    }
}

void iWaterReset()
{
    if (gGCwaterDisable) {
        return;
    }

    waterLightKit = (xLightKit*)xSTFindAsset(HASH_gc_water_lightkit, NULL);
    if (!waterLightKit)
        return;

    for (S32 i = 0; i < numWaterObjects; i++) {
        zEntSimpleObj* ent = (zEntSimpleObj*)waterObjects[i];

        zScene* s = globals.sceneCur;

        iprintf("Setting water light kit on object %08X (%s) model %08X (%s)\n", ent->id, xSTAssetName(ent->id), ent->model->modelID, xSTAssetName(ent->model->modelID));

        xModelSetLightKit(ent->model, waterLightKit, 0.0f);
        ent->sflags |= 0x8;
        ent->render = iWaterObjectRender;

        if (!defaultRenderCB) {
            defaultRenderCB = RpAtomicGetRenderCallBack(ent->model->Data);
        }

        RpAtomicSetRenderCallBack(ent->model->Data, iWaterAtomicRender);
    }
}

asm void CalcNBT(register _rwDlNBTData* nbtData)
{
    nofralloc
    lbz r4, 0x1c(r3)
    rotlwi r4, r4, 1

    lwz r5, 0x0(r3)
    psq_l f0, 0x0(r5), 0, 5
    add r5, r5, r4
    psq_l f1, 0x0(r5), 1, 5

    lwz r5, 0x4(r3)
    psq_l f2, 0x0(r5), 0, 5
    add r5, r5, r4
    psq_l f3, 0x0(r5), 1, 5

    lwz r5, 0x8(r3)
    psq_l f4, 0x0(r5), 0, 5
    add r5, r5, r4
    psq_l f5, 0x0(r5), 1, 5

    lwz r5, 0x10(r3)
    psq_l f6, 0x0(r5), 0, 7

    lwz r5, 0x14(r3)
    psq_l f7, 0x0(r5), 0, 7

    lwz r5, 0x18(r3)
    psq_l f8, 0x0(r5), 0, 7

    ps_sub f9, f2, f0
    ps_sub f10, f3, f1

    ps_sub f11, f4, f0
    ps_sub f12, f5, f1

    ps_sub f0, f7, f6
    ps_sub f1, f8, f6

    ps_muls1 f2, f9, f1
    ps_muls1 f3, f10, f1

    ps_muls1 f4, f11, f0
    ps_muls1 f5, f12, f0

    ps_sub f6, f4, f2
    ps_sub f7, f5, f3

    ps_merge10 f1, f1, f1
    ps_mul f0, f0, f1
    ps_merge10 f1, f0, f0

    ps_cmpu0 cr2, f1, f0

    lbz r4, 0x1d(r3)
    rotlwi r5, r4, 1

    lwz r6, 0xc(r3)

    psq_l f8, 0x0(r6), 0, 6
    add r6, r6, r5

    psq_l f9, 0x0(r6), 1, 6
    add r6, r6, r4

    ps_mul f10, f8, f6

    ps_mul f11, f9, f7

    ps_sum1 f12, f10, f10, f10

    ps_sum0 f12, f11, f11, f12

    ps_merge00 f12, f12, f12

    ps_neg f12, f12

    ps_madd f0, f8, f12, f6
    ps_madd f1, f9, f12, f7

    ps_mul f2, f0, f0
    ps_mul f3, f1, f1

    ps_sum1 f12, f2, f2, f2
    ps_sum0 f12, f3, f3, f12

    ps_sub f4, f4, f4

    ps_cmpu1 cr1, f12, f4
    ble cr1, skip

    ps_cmpu0 cr1, f12, f4
    ble cr1, skip

    ps_rsqrte f12, f12
    ps_muls0 f0, f0, f12
    ps_muls0 f1, f1, f12

skip:
    ps_muls0 f2, f0, f9
    ps_muls0 f3, f8, f1

    ps_merge10 f8, f8, f8
    ps_mul f4, f0, f8

    ps_merge10 f5, f2, f3
    ps_merge10 f6, f3, f2
    ps_merge10 f7, f4, f4

    ps_sub f2, f5, f6
    ps_sub f3, f4, f7

    bge cr2, store
    ps_neg f0, f0
    ps_neg f1, f1

store:
    psq_st f0, 0x0(r6), 0, 6
    add r6, r6, r5

    psq_st f1, 0x0(r6), 1, 6
    add r6, r6, r4

    psq_st f2, 0x0(r6), 0, 6
    add r6, r6, r5

    psq_st f3, 0x0(r6), 1, 6
    blr
}
