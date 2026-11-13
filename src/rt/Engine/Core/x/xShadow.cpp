#include "xShadow.h"

#include "xDebug.h"
#include "xClumpColl.h"
#include "xCollide.h"
#include "xCam.h"
#include "xCollideSweptSphere.h"
#include "xColor.h"
#include "xDebugTweak.h"
#include "xDraw.h"
#include "xEnv.h"
#include "xEnt.h"
#include "xGlobals.h"
#include "xGrid.h"
#include "xJSP.h"
#include "xMath.h"
#include "xutil.h"
#include "xIM.h"
#include "xLightKit.h"
#include "xMemMgr.h"
#include "xModel.h"
#include "xRenderBuffer.h"
#include "xRenderState.h"
#include "xShadowSimple.h"
#include "xstransvc.h"
#include "zBase.h"
#include "zEnt.h"
#include "zGrid.h"
#include "zPad.h"
#include "zScene.h"
#include "iCamera.h"
#include "iModel.h"
#include "iSystem.h"
#include "iTime.h"

#include "decomp.h"

#include <stdlib.h>
#include <string.h>

xTWEAKABLE F32 SHADOW_BIAS_AMT = 0.008f;
xTWEAKABLE F32 SHADOW_MINNORMY = 0.0871557f;
xTWEAKABLE U32 rrs = 1;
xTWEAKABLE U32 no_shadows = 0;
xTWEAKABLE F32 SHADOW_EPISLON = -0.0001f;
RpLight* ShadowLight;
RwRGBAReal ShadowLightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
xTWEAKABLE U32 sCacheSrcBlend = 5;
xTWEAKABLE U32 sCacheDestBlend = 6;
xTWEAKABLE bool sDrawCameraRaster = false;
static xTWEAKABLE bool drawFillCacheBox = false;
static xTWEAKABLE bool drawCombinedAnimBounds = false;
static xTWEAKABLE bool drawLinesToEnvContacts = false;
static xTWEAKABLE bool drawShadowVolume = false;
static xTWEAKABLE S32 enableNPCShadowOneEnt = 1;
static xTWEAKABLE S32 enableNPCShadowAllEnt = 0;
static xTWEAKABLE F32 DefaultShadowDepth = 10.0f;
S32 SHADOW_CACHE_POLY_MAX = 256;
#if DEBUG || RELEASE
char* sCacheBlendLabels[11] = {
    "BLENDZERO",
    "BLENDONE",
    "BLENDSRCCOLOR",
    "BLENDINVSRCCOLOR",
    "BLENDSRCALPHA",
    "BLENDINVSRCALPHA",
    "BLENDDESTALPHA",
    "BLENDINVDESTALPHA",
    "BLENDDESTCOLOR",
    "BLENDINVDESTCOLOR",
    "BLENDSRCALPHASAT"
};
static const U32 sCacheBlendValues[11] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
static U32 sCacheNumBlendModes = 11;
#endif
static F32 SHADOW_BOTH;
static RwIm3DVertex* Im3DBuffer;
static U32 Im3DBufferPos;
#if DEBUG
static U32 ShadowNumTriangles;
#endif
RwCamera* ShadowCamera;
RwRaster* ShadowCameraRaster;
static RwRaster* ShadowRenderRaster;
xTWEAKABLE S32 ShadowAA = 0;
xTWEAKABLE bool shadow_tri_depth_cull = true;
xTWEAKABLE F32 shadow_max_tri_seperation = 4.5f;
xTWEAKABLE bool shadow_tri_depth_culled = false;
xTWEAKABLE bool shadow_tri_outline = false;
xTWEAKABLE bool shadow_tri_reject = false;
xTWEAKABLE bool shadow_tri_accept = false;
xTWEAKABLE bool shadow_showcam = false;
xTWEAKABLE bool shadow_preview = false;
xTWEAKABLE bool shadow_zone = false;
xTWEAKABLE bool shadow_normal_flip = false;
#if DEBUG || RELEASE
static RwV2d ShadowPreviewVerts[2] = { { 0.83f, 0.02f }, { 0.98f, 0.22f } };
#endif
xVec3 gShadowObjectRadius;
static U32 shadow_ent_count;
static S32 sShadowCollJSP;
static RwRaster* gc_saveraster;

typedef struct _ProjectionParam
{
    RwV3d at;
    RwMatrix invMatrix;
    U8 shadowValue;
    S32 fade;
    U32 numIm3DBatch;
    U32 shadowWord;
} ProjectionParam;

static S32 SetupShadow();
static RwCamera* ShadowCameraCreatePersp(S32 size);
static RwRaster* ShadowRasterCreate(S32 size);
static RwCamera* ShadowCameraUpdate(RwCamera* shadowCamera, void* model, void (*renderCB)(void*), xVec3* center, xVec3* radius, S32 shadowMode);
static RwCamera* ShadowCameraUpdatePreRendered(RwCamera* shadowCamera, void* model, void (*renderCB)(void*), xVec3* center, xVec3* radius, S32 preRenderedSlot, S32 shadowMode);
static RwCamera* ShadowCameraFlushPreRendered(RwCamera* shadowCamera, void* model, void (*renderCB)(void*), xVec3* center, xVec3* radius, S32 preRenderedSlot, S32 shadowMode);
static void InvertRaster(RwCamera* camera);
static RwCamera* ShadowCameraSetSpherePersp(RwCamera* camera, RwV3d* center, xVec3* radius);

void xShadowInit()
{
    S32 size;
    RwFrame* frame;

    xShadowCameraCreate();

    size = 128;

    gc_saveraster = RwRasterCreate(size, size, 32, 0x504);

    xASSERT_MSG_L(278, gc_saveraster != 0L, "gc_saveraster != NULL");

    shadow_ent_count = 0;

    ShadowLight = RpLightCreate(rpLIGHTDIRECTIONAL);
    RpLightSetColor(ShadowLight, &ShadowLightColor);
    frame = RwFrameCreate();
    RpLightSetFrame(ShadowLight, frame);

#if DEBUG || RELEASE
    xTWEAK_SELECT("Player|BowlingDecal|cache|SrcBlend", &sCacheSrcBlend, (const char**)sCacheBlendLabels, sCacheBlendValues, sCacheNumBlendModes, NULL, NULL, 0x2);
    xTWEAK_SELECT("Player|BowlingDecal|cache|DestBlend", &sCacheDestBlend, (const char**)sCacheBlendLabels, sCacheBlendValues, sCacheNumBlendModes, NULL, NULL, 0x2);

    xTWEAK_BOOL("FX|Shadow|shadow_tri_depth_cull", &shadow_tri_depth_cull, NULL, NULL, 0);
    xTWEAK_BOOL("FX|Shadow|shadow_tri_depth_culled", &shadow_tri_depth_culled, NULL, NULL, 0);
    xTWEAK_BOOL("FX|Shadow|shadow_tri_outline", &shadow_tri_outline, NULL, NULL, 0);
    xTWEAK_BOOL("FX|Shadow|shadow_tri_reject", &shadow_tri_reject, NULL, NULL, 0);
    xTWEAK_BOOL("FX|Shadow|shadow_tri_accept", &shadow_tri_accept, NULL, NULL, 0);
    xTWEAK_BOOL("FX|Shadow|shadow_showcam", &shadow_showcam, NULL, NULL, 0);
    xTWEAK_BOOL("FX|Shadow|shadow_preview", &shadow_preview, NULL, NULL, 0);
    xTWEAK_BOOL("FX|Shadow|shadow_zone", &shadow_zone, NULL, NULL, 0);
    xTWEAK_BOOL("FX|Shadow|shadow_normal_flip", &shadow_normal_flip, NULL, NULL, 0);
    xTWEAK("FX|Shadow|shadow_max_tri_seperation", &shadow_max_tri_seperation, 0.5f, 10.0f, NULL, NULL, 0);
    xTWEAK("FX|Shadow|SHADOW_EPISLON", &SHADOW_EPISLON, -1.0f, 1.0f, NULL, NULL, 0);
#endif
}

static S32 SetupShadow()
{
    S32 res, res2;

    res = 128;

    while (res > (S32)FB_XRES || res > (S32)FB_YRES) {
        res >>= 1;
    }

    if (ShadowAA) {
        res2 = res * 2;
    } else {
        res2 = res;
    }

    ShadowCamera = ShadowCameraCreatePersp(res);

    if (!ShadowCamera) {
        return FALSE;
    }

    ShadowCameraRaster = ShadowRasterCreate(res);
    if (!ShadowCameraRaster) {
        return FALSE;
    }

    RwCameraSetRaster(ShadowCamera, ShadowCameraRaster);
    return TRUE;
}

static void DestroyShadow(); // UNUSED

void xShadowSetWorld(RpWorld* world)
{
    RpWorldAddCamera(world, ShadowCamera);

    SHADOW_BOTH = 2.0f;
}

void xShadowSetLight(xVec3* target_pos, xVec3* in_vec, F32)
{
    xVec3 zvec;
    xMat4x3 matrix;

    xVec3Normalize(&zvec, in_vec);

    xMat3x3LookVec(&matrix, &zvec);

    matrix.pos = *target_pos;

    RwFrame* camFrame = RwCameraGetFrame(ShadowCamera);
    RwMatrix* camMatrix = RwFrameGetMatrix(camFrame);
    xMat4x3Copy((xMat4x3*)camMatrix, &matrix);

    RwFrameOrthoNormalize(camFrame);

    RwMatrixUpdate(camMatrix);

    RwFrameUpdateObjects(camFrame);
}

S32 xShadowCameraCreate()
{
    if (!SetupShadow()) {
        return FALSE;
    }

#if DEBUG || RELEASE
    xDebugModeRegisterCallback(7, xShadowDebug);
#endif

    return TRUE;
}

void xShadowCameraUpdatePreRendered(void* model, void (*renderCB)(void*), xVec3* center, xVec3* radius, S32 preRenderedSlot, S32 shadowMode)
{
    ShadowCameraUpdatePreRendered(ShadowCamera, model, renderCB, center, radius, preRenderedSlot, shadowMode);

    ShadowRenderRaster = ShadowCameraRaster;
}

void xShadowCameraUpdate(void* model, void (*renderCB)(void*), xVec3* center, xVec3* radius, S32 shadowMode)
{
    xSTATS_BEGIN(t1);

    xSTATS_BEGIN(tt);

    gShadowObjectRadius = *radius;

    xSTATS_END(tt, shadow_track);

    xSTATS_BEGIN(tr);

    ShadowCameraUpdate(ShadowCamera, model, renderCB, center, radius, shadowMode);

    xSTATS_END(tr, shadow_texrender);

    ShadowRenderRaster = ShadowCameraRaster;
    xSTATS_END(t1, shadow);
}

void xShadowCameraFlushPreRendered(void* model, void (*renderCB)(void*), xVec3* center, xVec3* radius, S32 preRenderedSlot, S32 shadowMode)
{
    ShadowCameraFlushPreRendered(ShadowCamera, model, renderCB, center, radius, preRenderedSlot, shadowMode);
}

static void modelRenderCB(void* model); // UNUSED

#if RELEASE
DECOMP_FORCEFLOAT(0.0f)
DECOMP_FORCEFLOAT(100000.0f)
DECOMP_FORCEFLOAT(0.00001f)
#elif MASTERDEBUG || MASTER
DECOMP_FORCEFLOAT(0.5f)
DECOMP_FORCEFLOAT(0.0f)
DECOMP_FORCEFLOAT(1.0f)
DECOMP_FORCEFLOAT(0.00001f)
#endif

#if DEBUG || RELEASE
U32 gEntID;
#endif

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("zPlayerContainer.h", "index >= 0 && index < numPlayers")
#endif

U32 xShadowReceiveShadowSetup(xEnt* ent)
{
    xSTATS_BEGIN(tt);
    xSTATS_BEGIN(tt2);

#if DEBUG || RELEASE
    if (gEntID && ent->id == gEntID) {
        iDebugBreak();
    }
#endif

    if (ent->model && xEntIsVisible(ent)) {
        if (!rrs || (ent->baseFlags & k_XBASE_RECEIVES_SHADOWS))
            if (!xModelCullGroup(ent->model)) {
                xSTATS_END(tt2, shadow_frustumtest);

                xSTATS_END(tt, shadow_receivesetup);

                return TRUE;
            }
    }

    xSTATS_END(tt, shadow_receivesetup);
    xSTATS_END(tt2, shadow_frustumtest);

    return FALSE;
}

xTWEAKABLE U32 enable_shadowmap_pipeline = 0;

void xShadowReceiveShadow(xEnt* ent, F32 shadowFactor, S32 shadowMode, RwMatrix* shadowMat, RwRaster* shadowRast)
{
    xASSERT_L(1445, ent);
    xASSERT_L(1446, ent->model);
    xASSERT_L(1447, ent->model->Data);

    xModelInstance* m = ent->model;
    if (xModelIsPreinstanced(ent->model)) {
        m = ent->collModel ? ent->collModel : ent->model;
    }

    if (!xModelIsPreinstanced(m)) {
        RwMatrix oldroot = { 0 };
        if (m->Scale.x) {
            oldroot = *m->Mat;
            m->Mat->right.x *= m->Scale.x,
            m->Mat->right.y *= m->Scale.x,
            m->Mat->right.z *= m->Scale.x,
            m->Mat->up.x *= m->Scale.y,
            m->Mat->up.y *= m->Scale.y,
            m->Mat->up.z *= m->Scale.y,
            m->Mat->at.x *= m->Scale.z,
            m->Mat->at.y *= m->Scale.z,
            m->Mat->at.z *= m->Scale.z;
        }

        RwCamera* shadowCamera = ShadowCamera;
        RwMatrix* shadowMatrix;
        RwMatrix invMatrix;
        RwV3d at, scl, tr;
        F32 fadeDist = 0.0f;
        U32 i, num_verts;
        xVec3* xvert;
        RpTriangle* tri;
        RpGeometry* geom;
        RwMatrix* mat;
        xVec3 radius;

        xSTATS_BEGIN(t0);

        xSTATS_BEGIN(tt);

        if (shadow_tri_outline) {
            xprintf("ent shadow %s type %d\n", xSTAssetName(ent->id), ent->baseType);
        }

        S32 fogstate;

        if (shadowRast) {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, shadowRast);
        } else {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwCameraGetRaster(ShadowCamera));
        }

        RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);

        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

        RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fogstate);
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);

        switch (shadowMode) {
        case 1:
            if (shadowFactor < 0.0f) {
                shadowFactor = -shadowFactor;
                xRenderStateBlendModesSet(rwBLENDINVSRCALPHA, rwBLENDSRCALPHA);
            } else {
                xRenderStateBlendModesSet((RwBlendFunction)sCacheSrcBlend, (RwBlendFunction)sCacheDestBlend);
            }
            break;
        case 0:
        default:
            if (shadowFactor < 0.0f) {
                shadowFactor = -shadowFactor;
                xRenderStateBlendModesSet(rwBLENDZERO, rwBLENDSRCCOLOR);
            } else {
                xRenderStateBlendModesSet(rwBLENDZERO, rwBLENDINVSRCCOLOR);
            }
            break;
        }

        xRenderStateZModesSet(FALSE, TRUE);

        if (shadowMat) {
            shadowMatrix = shadowMat;
        } else {
            shadowMatrix = RwFrameGetMatrix(RwCameraGetFrame(shadowCamera));
        }

        at = *RwMatrixGetAt(shadowMatrix);
        radius = gShadowObjectRadius;

        RwMatrixInvert(&invMatrix, shadowMatrix);

        scl.x = -0.5f / radius.x;
        scl.y = -0.5f / radius.y;
        scl.z = 1.0f / (fadeDist + radius.z);
        RwMatrixScale(&invMatrix, &scl, rwCOMBINEPOSTCONCAT);

        tr.x = tr.y = 0.5f;
        tr.z = 0.0f;
        RwMatrixTranslate(&invMatrix, &tr, rwCOMBINEPOSTCONCAT);

        xModelInstance* model = m;
        U32 max_verts = 0;
        while (model) {
            if (model->Flags & 0x1) {
                geom = RpAtomicGetGeometry(model->Data);
                mat = model->Mat;

                num_verts = RpGeometryGetNumVertices(geom);
                if (num_verts > max_verts)
                    max_verts = num_verts;
            }
            if (ent->pflags & 0x40) break;
            model = model->Next;
        }

        if (max_verts) {
            xvert = (xVec3*)xMEMPUSHTEMP_L(max_verts * sizeof(xVec3), 0, 1625);
            if (xvert) {
                Im3DBuffer = gRenderBuffer.m_vertex;

                model = m;
                while (model) {
                    if (model->Flags & 0x1) {
                        xVec3* vert = xvert;

                        geom = RpAtomicGetGeometry(model->Data);
                        mat = model->Mat;

                        num_verts = RpGeometryGetNumVertices(geom);

                        xASSERT_L(1647, num_verts <= max_verts);

                        if (!xModelIsPreinstanced(model)) {
                            iModelVertEval(model->Data, 0, num_verts, mat, NULL, xvert);

                            RwIm3DVertex* imv;
                            U8 val = 255.0f * shadowFactor;

                            tri = geom->triangles;
                            for (i = 0; i < geom->numTriangles; i++, tri++) {
                                if (Im3DBufferPos > 477) {
                                    xIMRenderLegacy(Im3DBuffer, Im3DBufferPos, NULL, rwPRIMTYPETRILIST);
                                    Im3DBufferPos = 0;
                                }

                                RwV3d vShad[3];
                                imv = &Im3DBuffer[Im3DBufferPos];
                                xVec3* v0 = &vert[tri->vertIndex[0]];
                                xVec3* v1 = &vert[tri->vertIndex[1]];
                                xVec3* v2 = &vert[tri->vertIndex[2]];
                                RwV3d vIn[3];

                                vIn[0] = *(RwV3d*)v0;
                                vIn[1] = *(RwV3d*)v1;
                                vIn[2] = *(RwV3d*)v2;

                                RwV3dTransformPoints(vShad, vIn, ARRAY_LENGTH(vIn), &invMatrix);

                                if ((vShad[0].z < 0.0f && vShad[1].z < 0.0f && vShad[2].z < 0.0f) || (vShad[0].x < 0.0f && vShad[1].x < 0.0f && vShad[2].x < 0.0f) || (vShad[0].x > 1.0f && vShad[1].x > 1.0f && vShad[2].x > 1.0f) || (vShad[0].y < 0.0f && vShad[1].y < 0.0f && vShad[2].y < 0.0f) || (vShad[0].y > 1.0f && vShad[1].y > 1.0f && vShad[2].y > 1.0f)) {
                                    continue;
                                }

                                RwV3d a, b, c;
                                RwV3dSub(&a, (RwV3d*)v1, (RwV3d*)v0);
                                RwV3dSub(&b, (RwV3d*)v2, (RwV3d*)v0);
                                RwV3dCrossProduct(&c, &a, &b);
                                RwReal len = RwV3dLength(&c);
                                if (xabs(len) < 0.00001f) {
                                    if (len < 0.0f) {
                                        len = -0.00001f;
                                    } else {
                                        len = 0.00001f;
                                    }
                                }

                                len = SHADOW_BIAS_AMT / len;
                                RwV3dScale(&c, &c, len);

                                if (RwV3dDotProduct(&c, &at) > -SHADOW_BIAS_AMT * SHADOW_MINNORMY) {
                                    continue;
                                }

                                RwIm3DVertexSetPos(&imv[0], v0->x + c.x, v0->y + c.y, v0->z + c.z);
                                RwIm3DVertexSetPos(&imv[1], v1->x + c.x, v1->y + c.y, v1->z + c.z);
                                RwIm3DVertexSetPos(&imv[2], v2->x + c.x, v2->y + c.y, v2->z + c.z);

                                imv[0].u = vShad[0].x;
                                imv[1].u = vShad[1].x;
                                imv[2].u = vShad[2].x;
                                imv[0].v = vShad[0].y;
                                imv[1].v = vShad[1].y;
                                imv[2].v = vShad[2].y;

                                RwIm3DVertexSetRGBA(&imv[0], val, val, val, val);
                                RwIm3DVertexSetRGBA(&imv[1], val, val, val, val);
                                RwIm3DVertexSetRGBA(&imv[2], val, val, val, val);

                                Im3DBufferPos += 3;
                            }

                            if (Im3DBufferPos) {
                                xIMRenderLegacy(Im3DBuffer, Im3DBufferPos, NULL, rwPRIMTYPETRILIST);
                                Im3DBufferPos = 0;
                            }

#if DEBUG || RELEASE
                            if (xDebugModeGet() == 7)
                                xprintf("ent shadow tris:  %d\n", geom->numTriangles);
#endif
                        }
                    }
                    if (ent->pflags & 0x40) break;
                    model = model->Next;
                }

                xMEMPOPTEMP(xvert);

                xRenderStateBlendModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);

                RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fogstate);

                xRenderStateZModesSet(TRUE, TRUE);

                xSTATS_END(tt, shadow_receive);

                xSTATS_END(t0, shadow);
            }
        }

        if (m->Scale.x)
            *m->Mat = oldroot;
    }
}

void xShadow_ListAdd(xEnt* ent)
{
    xShadowManager_Add(ent);
}

static void InvertRaster(RwCamera* camera)
{
    RwIm2DVertex vx[4];
    F32 crw, crh;
    RwRaster* camRas;
    F32 recipZ;

    camRas = RwCameraGetRaster(camera);
    crw = RwRasterGetWidth(camRas);
    crh = RwRasterGetHeight(camRas);

    recipZ = 1.0f / RwCameraGetNearClipPlane(camera);

    RwIm2DVertexSetScreenX(&vx[0], 0.0f);
    RwIm2DVertexSetScreenY(&vx[0], 0.0f);
    RwIm2DVertexSetScreenZ(&vx[0], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetIntRGBA(&vx[0], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[0], 0.0f, recipZ);
    RwIm2DVertexSetV(&vx[0], 0.0f, recipZ);

    RwIm2DVertexSetScreenX(&vx[1], 0.0f);
    RwIm2DVertexSetScreenY(&vx[1], crh);
    RwIm2DVertexSetScreenZ(&vx[1], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetIntRGBA(&vx[1], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[1], 0.0f, recipZ);
    RwIm2DVertexSetV(&vx[1], 1.0f, recipZ);

    RwIm2DVertexSetScreenX(&vx[2], crw);
    RwIm2DVertexSetScreenY(&vx[2], 0.0f);
    RwIm2DVertexSetScreenZ(&vx[2], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetIntRGBA(&vx[2], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[2], 1.0f, recipZ);
    RwIm2DVertexSetV(&vx[2], 0.0f, recipZ);

    RwIm2DVertexSetScreenX(&vx[3], crw);
    RwIm2DVertexSetScreenY(&vx[3], crh);
    RwIm2DVertexSetScreenZ(&vx[3], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetIntRGBA(&vx[3], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[3], 1.0f, recipZ);
    RwIm2DVertexSetV(&vx[3], 1.0f, recipZ);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

    xRenderStateBlendAndZModesSet(rwBLENDINVDESTCOLOR, rwBLENDZERO, FALSE, FALSE);

    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, vx, ARRAY_LENGTH(vx));

    xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA, TRUE, TRUE);
}

static void ShadowCameraDestroy(RwCamera* shadowCamera)
{
    if (shadowCamera) {
        RwRaster* raster;

        _rwFrameSyncDirty();

        RwFrame* frame = RwCameraGetFrame(shadowCamera);
        if (frame) {
            RwCameraSetFrame(shadowCamera, NULL);
            RwFrameDestroy(frame);
        }

        raster = RwCameraGetZRaster(shadowCamera);
        if (raster) {
            RwCameraSetZRaster(shadowCamera, NULL);
            RwRasterDestroy(raster);
        }

        raster = RwCameraGetRaster(shadowCamera);
        if (raster) {
            RwCameraSetRaster(shadowCamera, NULL);
            RwRasterDestroy(raster);
        }

        RwCameraDestroy(shadowCamera);
    }
}

static RwCamera* ShadowCameraCreate(S32 size); // UNUSED
static RwCamera* ShadowCameraSetFrustum(RwCamera* camera, F32 objectRadius); // UNUSED

DECOMP_FORCEFLOAT(0.001f)

static RwCamera* ShadowCameraSetLight(RwCamera* camera, RpLight* light); // UNUSED
static RwCamera* ShadowCameraSetCenter(RwCamera* camera, RwV3d* center); // UNUSED

static RwCamera* ShadowCameraUpdate(RwCamera* shadowCamera, void* model, void (*renderCB)(void*), xVec3*, xVec3*, S32 shadowMode)
{
    RwRGBA bgColor = { 255, 255, 255, 0 };

    RwCamera* camera = RwCameraGetCurrentCamera();

    S32 fogstate;
    RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fogstate);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);

    if (camera) {
        RwCameraEndUpdate(camera);
    }

    GCSaveFrameBuffer();

    shadowCamera->frameBuffer->width--;
    shadowCamera->frameBuffer->height--;
    RwCameraClear(shadowCamera, &bgColor, rwCAMERACLEARIMAGE);
    shadowCamera->frameBuffer->width++;
    shadowCamera->frameBuffer->height++;

    RwFrameOrthoNormalize(RwCameraGetFrame(shadowCamera));

    if (RwCameraBeginUpdateWrapper(shadowCamera)) {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
        xRenderStateZModesSet(FALSE, FALSE);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

        renderCB(model);

        if (!shadowMode) {
            InvertRaster(shadowCamera);
        }

        xRenderStateZModesSet(TRUE, TRUE);
        RwCameraEndUpdate(shadowCamera);

        RwGameCubeCameraTextureFlush(RwCameraGetRaster(shadowCamera), FALSE);
    }

    if (camera) {
        RwCameraBeginUpdateWrapper(camera);
    }

    GCRestoreFrameBuffer();

    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fogstate);

    return shadowCamera;
}

static RwCamera* ShadowCameraUpdatePreRendered(RwCamera* shadowCamera, void* model, void (*renderCB)(void*), xVec3*, xVec3*, S32 preRenderedSlot, S32 shadowMode)
{
    RwRGBA bgColor = { 255, 255, 255, 0 };

    RwCamera* camera = RwCameraGetCurrentCamera();

    S32 fogstate;
    RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fogstate);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);

    if (camera) {
        RwCameraEndUpdate(camera);
    }

    shadowCamera->frameBuffer->nOffsetX = 512;
    shadowCamera->frameBuffer->nOffsetY = (preRenderedSlot + 1) * RwRasterGetHeight(shadowCamera->frameBuffer);
    RwCameraClear(shadowCamera, &bgColor, rwCAMERACLEARIMAGE);

    RwFrameOrthoNormalize(RwCameraGetFrame(shadowCamera));

    if (RwCameraBeginUpdateWrapper(shadowCamera)) {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
        xRenderStateZModesSet(FALSE, FALSE);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

        renderCB(model);

        if (!shadowMode) {
            F32 vp[6];

            GXGetViewportv(vp);
            GXSetViewport(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
            GXSetScissor(0, 0, 640, 480);
            GXSetScissorBoxOffset(512, (S16)-((preRenderedSlot + 1) * RwRasterGetHeight(shadowCamera->frameBuffer)));
            shadowCamera->frameBuffer->nOffsetX = 0;
            shadowCamera->frameBuffer->nOffsetY = 0;

            InvertRaster(shadowCamera);

            shadowCamera->frameBuffer->nOffsetX = 512;
            shadowCamera->frameBuffer->nOffsetY = (preRenderedSlot + 1) * RwRasterGetHeight(shadowCamera->frameBuffer);
            GXSetViewportv(vp);
            GXSetScissor(vp[0], vp[1], vp[2], vp[3]);
            GXSetScissorBoxOffset(0, 0);
        }

        xRenderStateZModesSet(TRUE, TRUE);
        RwCameraEndUpdate(shadowCamera);
    }

    if (camera) {
        RwCameraBeginUpdateWrapper(camera);
    }

    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fogstate);

    return shadowCamera;
}

static RwCamera* ShadowCameraFlushPreRendered(RwCamera* shadowCamera, void*, void (*)(void*), xVec3*, xVec3*, S32 preRenderedSlot, S32)
{
    RwRaster* raster = RwCameraGetRaster(shadowCamera);
    raster->nOffsetX = 512;
    raster->nOffsetY = (preRenderedSlot + 1) * RwRasterGetHeight(raster);
    GameCubeCameraTextureFlush(RwCameraGetRaster(shadowCamera), FALSE);
    shadowCamera->frameBuffer->nOffsetX = 0;
    shadowCamera->frameBuffer->nOffsetY = 0;

    return shadowCamera;
}

void GameCubeCameraTextureFlush(RwRaster* raster, S32 boxFilter)
{
    RwRaster* dstParent = RwRasterGetParent(raster);
    RwGameCubeRasterExtension* dstRasExt = RwGameCubeRasterGetExtension(dstParent);

    GXSetCopyFilter(GX_FALSE, NULL, GX_FALSE, NULL);

    if (boxFilter) {
        GXSetTexCopySrc(raster->nOffsetX * 2, raster->nOffsetY * 2, RwRasterGetWidth(raster) * 2, RwRasterGetHeight(raster) * 2);
    } else {
        GXSetTexCopySrc(raster->nOffsetX, raster->nOffsetY, RwRasterGetWidth(raster), RwRasterGetHeight(raster));
    }

    GXSetTexCopyDst(RwRasterGetWidth(dstParent), RwRasterGetHeight(dstParent), (GXTexFmt)dstRasExt->format, boxFilter);

    GXCopyTex(dstRasExt->pixels, GX_TRUE);

    GXPixModeSync();

    GXSetCopyFilter(renderObject.aa, renderObject.sample_pattern, GX_TRUE, renderObject.vfilter);

    GXInvalidateTexAll();
}

static RwCamera* ShadowCameraUpdate(RwCamera* shadowCamera, xModelInstance* modelInst, RwRGBA bgColor); // UNUSED

static RwRaster* ShadowRasterCreate(S32 size)
{
    RwRaster* raster;

    raster = RwRasterCreate(size, size, 0, rwRASTERTYPECAMERATEXTURE);

    return raster;
}

static void ShadowRasterDestroy(RwRaster* raster); // UNUSED
static S32 ShadowRasterResample(RwRaster* destRaster, RwRaster* sourceRaster, RwCamera* camera); // UNUSED
static S32 ShadowRasterBlur(RwRaster* shadowRaster, RwRaster* tempRaster, RwCamera* camera, U32 numPass); // UNUSED

RwRaster* ShadowRasterRender(RwRaster* shadowRaster, RwV2d* vx)
{
    RwCamera* camera = RwCameraGetCurrentCamera();
    RwRaster* camRas;
    F32 crw, crh;
    F32 recipCamZ;

    camRas = RwCameraGetRaster(camera);
    crw = RwRasterGetWidth(camRas);
    crh = RwRasterGetHeight(camRas);

    recipCamZ = 1.0f / RwCameraGetNearClipPlane(camera);

    xRenderStateBlendModesSet(rwBLENDONE, rwBLENDZERO);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, shadowRaster);

    Im2DRenderQuad(vx[0].x * crw, vx[0].y * crh, vx[1].x * crw, vx[1].y * crh, RwIm2DGetNearScreenZ(), recipCamZ, 0.0f);

    xRenderStateBlendModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);

    return shadowRaster;
}

static RpCollisionTriangle* ShadowRenderTriangleCB(RpIntersection*, RpWorldSector*, RpCollisionTriangle* collTriangle, RwReal, void* data); // UNUSED

#if DEBUG || RELEASE
static void RenderShadowPolyOutline(xShadowPoly* poly)
{
    xDrawLine((xVec3*)&poly->vert[0], (xVec3*)&poly->vert[1]);
    xDrawLine((xVec3*)&poly->vert[0], (xVec3*)&poly->vert[2]);
    xDrawLine((xVec3*)&poly->vert[1], (xVec3*)&poly->vert[2]);
}
#endif

static void ShadowRenderCachePolys(ProjectionParam* param, xShadowPoly* poly, S32 count)
{
    if (shadow_normal_flip) {
        param->at.x = -param->at.x;
        param->at.y = -param->at.y;
        param->at.z = -param->at.z;
    }

    for (S32 i = 0; i < count; i++, poly++) {
        RwIm3DVertex* imv;
        RwV3d vShad[3];
        RwV3d vIn[3];
        F32 cx, cy, cz;

        vIn[0] = *(RwV3d*)&poly->vert[0];
        vIn[1] = *(RwV3d*)&poly->vert[1];
        vIn[2] = *(RwV3d*)&poly->vert[2];

        RwV3dTransformPoints(vShad, vIn, ARRAY_LENGTH(vIn), &param->invMatrix);

        if ((vShad[0].z < 0.0f && vShad[1].z < 0.0f && vShad[2].z < 0.0f) || (vShad[0].x < 0.0f && vShad[1].x < 0.0f && vShad[2].x < 0.0f) || (vShad[0].x > 1.0f && vShad[1].x > 1.0f && vShad[2].x > 1.0f) || (vShad[0].y < 0.0f && vShad[1].y < 0.0f && vShad[2].y < 0.0f) || (vShad[0].y > 1.0f && vShad[1].y > 1.0f && vShad[2].y > 1.0f)) {
#if DEBUG || RELEASE
            if (shadow_tri_reject) {
                xDrawSetColor(255, 255, 255, 255);
                RenderShadowPolyOutline(poly);
            }
#endif
            continue;
        }

#if DEBUG || RELEASE
        if (shadow_tri_accept) {
            xDrawSetColor(255, 0, 255, 255);
            RenderShadowPolyOutline(poly);
        }
#endif

        if (Im3DBufferPos > 477) {
            xIMRenderLegacy(Im3DBuffer, Im3DBufferPos, NULL, rwPRIMTYPETRILIST);
            param->numIm3DBatch++;
            Im3DBufferPos = 0;
        }

        imv = &Im3DBuffer[Im3DBufferPos];

        cx = poly->vert[0].w * SHADOW_BIAS_AMT;
        cy = poly->vert[1].w * SHADOW_BIAS_AMT;
        cz = poly->vert[2].w * SHADOW_BIAS_AMT;
        RwIm3DVertexSetPos(&imv[0], poly->vert[0].x + cx, poly->vert[0].y + cy, poly->vert[0].z + cz);
        RwIm3DVertexSetPos(&imv[1], poly->vert[1].x + cx, poly->vert[1].y + cy, poly->vert[1].z + cz);
        RwIm3DVertexSetPos(&imv[2], poly->vert[2].x + cx, poly->vert[2].y + cy, poly->vert[2].z + cz);

        imv[0].u = vShad[0].x;
        imv[1].u = vShad[1].x;
        imv[2].u = vShad[2].x;

        imv[0].v = vShad[0].y;
        imv[1].v = vShad[1].y;
        imv[2].v = vShad[2].y;

        U8 val = param->shadowValue;
        RwIm3DVertexSetRGBA(&imv[0], val, val, val, val);
        RwIm3DVertexSetRGBA(&imv[1], val, val, val, val);
        RwIm3DVertexSetRGBA(&imv[2], val, val, val, val);

        Im3DBufferPos += 3;

#if DEBUG
        ShadowNumTriangles++;
#endif
    }

    if (shadow_normal_flip) {
        param->at.x = -param->at.x;
        param->at.y = -param->at.y;
        param->at.z = -param->at.z;
    }
}

static S32 ShadowRender(RwCamera* shadowCamera, RwRaster*, RpIntersection* shadowZone, F32 shadowFactor, F32 fadeDist); // UNUSED

DECOMP_FORCEBLOCK((const iEnv* env) {
    iEnvIsJSPVisible(env, 0);
})

DECOMP_FORCEACTIVE(
    (DEBUG || RELEASE) ? "xMemMgr.h" : 0,
    (DEBUG || RELEASE) ? "ptr" : 0,
    DEBUG ? "for %d bytes from %s, in %s line %d" : 0,
    DEBUG ? "shadow tris:  %d\n" : 0
)

xTWEAKABLE F32 zlim = 0.0f;
xTWEAKABLE F32 zamt = 0.05f;

#if DEBUG || RELEASE
void xShadowDebug()
{
    if (zPad::GetDebugPad()->Pressed(0x10000)) {
        shadow_tri_outline ^= 1;
    }
    if (zPad::GetDebugPad()->Pressed(0x20000)) {
        shadow_zone ^= 1;
        shadow_preview ^= 1;
    }
    if (zPad::GetDebugPad()->Pressed(0x40000)) {
        shadow_showcam ^= 1;
        enable_shadowmap_pipeline ^= 1;
    }
    if (zPad::GetDebugPad()->Pressed(0x80000)) {
        SHADOW_BIAS_AMT *= -1.0f;
        shadow_normal_flip ^= 1;
    }

    if (shadow_normal_flip)
        xprintf("shadow normals flipped\n");
    if (enable_shadowmap_pipeline) {
        xprintf("shadowmap pipeline enabled\n");
    }

    if (zPad::GetDebugPad()->On(0x80))
        zlim -= zamt;
    if (zPad::GetDebugPad()->On(0x20)) {
        zlim += zamt;
    }

    F32 f = 100.0f / MAX(gStats.shadow, 1);
    xStats* st = &gStats;

    xprintf("shadow time:          %6d\n", st->shadow);
    xprintf("shadow track:         %6d (%4.0f)\n", (S32)st->shadow_track, f * st->shadow_track);
    xprintf("shadow texrender:     %6d (%4.0f)\n", (S32)st->shadow_texrender, f * st->shadow_texrender);
    xprintf("shadow collideworld:  %6d (%4.0f)\n", (S32)st->shadow_collideworld, f * st->shadow_collideworld);
    xprintf("shadow renderworld:   %6d (%4.0f)\n", (S32)st->shadow_renderworld, f * st->shadow_renderworld);
    xprintf("shadow receivesetup:  %6d (%4.0f)\n", (S32)st->shadow_receivesetup, f * st->shadow_receivesetup);
    xprintf("shadow frustumtest:   %6d (%4.0f)\n", (S32)st->shadow_frustumtest, f * st->shadow_frustumtest);
    xprintf("shadow receive:       %6d (%4.0f)\n", (S32)st->shadow_receive, f * st->shadow_receive);
}
#endif

void GCSaveFrameBuffer()
{
    RwGameCubeCameraTextureFlush(gc_saveraster, FALSE);
}

void GCRestoreFrameBuffer()
{
    F32 size = 128.0f;
    F32 uvOffset = 0.5f / size;
    F32 recipCamZ = 1.0f / RwCameraGetFarClipPlane(RwCameraGetCurrentCamera());
    void* oldAlphaFunction;
    void* oldTextureFilter;

    RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &oldAlphaFunction);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);

    xRenderStateBlendAndZModesSet(rwBLENDONE, rwBLENDZERO, FALSE, FALSE);

    RwRenderStateGet(rwRENDERSTATETEXTUREFILTER, &oldTextureFilter);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, gc_saveraster);

    Im2DRenderQuad(0.0f, 0.0f, size, size, RwIm2DGetFarScreenZ(), recipCamZ, uvOffset);

    if ((S32)oldAlphaFunction >= 1)
        RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, oldAlphaFunction);
    else
        RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONGREATEREQUAL);

    xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA, TRUE, TRUE);

    if ((S32)oldTextureFilter >= 1) {
        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, oldTextureFilter);
    }

#if DEBUG || RELEASE
    if (xDebugModeGet() == 7) {
        xDrawSetColor(255, 255, 255, 255);
        xDrawLine2D(size, 2.0f + size, size, 18.0f + size);
        xDrawLine2D(2.0f + size, size, 18.0f + size, size);
    }
#endif
}

static RwCamera* ShadowCameraCreatePersp(S32 size)
{
    RwCamera* shadowCamera;
    RwV3d shadowCameraCenter = { 0.0f, 0.0f, 0.0f };

    shadowCamera = RwCameraCreateWrapper();
    if (shadowCamera) {
        RwCameraSetFrame(shadowCamera, RwFrameCreate());

        RwV2d vw;
        vw.x = 0.001745331f;
        vw.y = 0.001745331f;
        RwCameraSetViewWindow(shadowCamera, &vw);

        if (RwCameraGetFrame(shadowCamera)) {
            RwRaster* zRaster = RwRasterCreate(size, size, 0, rwRASTERTYPEZBUFFER);
            if (zRaster) {
                RwCameraSetZRaster(shadowCamera, zRaster);

                ShadowCameraSetSpherePersp(shadowCamera, &shadowCameraCenter, NULL);

                return shadowCamera;
            }
        }
    }

    ShadowCameraDestroy(shadowCamera);

    return NULL;
}

static RwCamera* ShadowCameraSetSpherePersp(RwCamera* camera, RwV3d* center, xVec3* radius)
{
    RwFrame* camFrame = RwCameraGetFrame(camera);
    RwMatrix* camMatrix = RwFrameGetMatrix(camFrame);
    RwV3d* camPos = RwMatrixGetPos(camMatrix);

    F32 objDepth = 286.4786f;

    F32 nearZ = objDepth - 0.5f;
    F32 farZ = objDepth + 0.5f;

    camera->nearPlane = nearZ;
    camera->farPlane = farZ;

    RwCameraSetNearClipPlane(camera, nearZ);
    RwCameraSetFarClipPlane(camera, farZ);

    *camPos = *center;

    RwV3dIncrementScaled(camPos, RwMatrixGetAt(camMatrix), -objDepth);

    RwMatrixUpdate(camMatrix);
    RwFrameUpdateObjects(camFrame);

    if (radius) {
        gShadowObjectRadius = *radius;
    }

    return camera;
}

struct ShadowCacheContext
{
    xShadowCache* cache;
    F32 minNormY;
};

#if DEBUG || RELEASE
static S32 MAX_CACHE_POLYS;
#endif

static RpCollisionTriangle* shadowCacheEnvCB(RpIntersection*, RpWorldSector*, RpCollisionTriangle* tri, RwReal, void* data)
{
    ShadowCacheContext* context = (ShadowCacheContext*)data;
    xShadowCache* cache = context->cache;

    if (cache->polyCount >= SHADOW_CACHE_POLY_MAX) {
#if DEBUG || RELEASE
        if (!MAX_CACHE_POLYS) {
            MAX_CACHE_POLYS = SHADOW_CACHE_POLY_MAX;
        }
        MAX_CACHE_POLYS++;
        return tri;
#else
        return NULL;
#endif
    }

    if (tri->normal.y < context->minNormY) {
        return tri;
    }

    if (sShadowCollJSP && !(((xClumpCollBSPTriangle*)tri->index)->flags & 0x8)) {
        return tri;
    }

    xShadowPoly* poly = &cache->poly[cache->polyCount];
    cache->polyCount++;
    *(xVec3*)&poly->vert[0] = *(xVec3*)tri->vertices[0];
    *(xVec3*)&poly->vert[1] = *(xVec3*)tri->vertices[1];
    *(xVec3*)&poly->vert[2] = *(xVec3*)tri->vertices[2];
    poly->vert[0].w = tri->normal.x;
    poly->vert[1].w = tri->normal.y;
    poly->vert[2].w = tri->normal.z;

    F32 dydx = -tri->normal.x / tri->normal.y;
    F32 dydz = -tri->normal.z / tri->normal.y;
    F32 depth0 = poly->vert[0].y + (dydx * (cache->pos.x - poly->vert[0].x) + dydz * (cache->pos.z - poly->vert[0].z));

    poly->depth = cache->pos.y - poly->vert[0].y;
    if (poly->depth < cache->minDepth) {
        cache->minDepth = poly->depth;
    }
    if (poly->depth > cache->maxDepth) {
        cache->maxDepth = poly->depth;
    }

    F32 n0x = poly->vert[0].z - poly->vert[1].z;
    F32 n0z = poly->vert[1].x - poly->vert[0].x;
    F32 n0d = n0x * (cache->pos.x - poly->vert[0].x) + n0z * (cache->pos.z - poly->vert[0].z);

    F32 n1x = poly->vert[1].z - poly->vert[2].z;
    F32 n1z = poly->vert[2].x - poly->vert[1].x;
    F32 n1d = n1x * (cache->pos.x - poly->vert[1].x) + n1z * (cache->pos.z - poly->vert[1].z);

    F32 n2x = poly->vert[2].z - poly->vert[0].z;
    F32 n2z = poly->vert[0].x - poly->vert[2].x;
    F32 n2d = n2x * (cache->pos.x - poly->vert[2].x) + n2z * (cache->pos.z - poly->vert[2].z);

    if (n0d <= 0.00001f && n1d <= 0.00001f && n2d <= 0.00001f) {
        cache->polyRayDepth[0] = MAX(depth0, cache->polyRayDepth[0]);
    }

    if (n0d + 0.5f * n0x * cache->radius.x <= 0.00001f && n1d + 0.5f * n1x * cache->radius.x <= 0.00001f && n2d + 0.5f * n2x * cache->radius.x <= 0.00001f) {
        cache->polyRayDepth[1] = MAX(depth0 + 0.5f * dydx * cache->radius.x, cache->polyRayDepth[1]);
    }

    if (n0d - 0.5f * n0x * cache->radius.x <= 0.00001f && n1d - 0.5f * n1x * cache->radius.x <= 0.00001f && n2d - 0.5f * n2x * cache->radius.x <= 0.00001f) {
        cache->polyRayDepth[2] = MAX(depth0 - 0.5f * dydx * cache->radius.x, cache->polyRayDepth[2]);
    }

    if (n0d + 0.5f * n0z * cache->radius.y <= 0.00001f && n1d + 0.5f * n1z * cache->radius.y <= 0.00001f && n2d + 0.5f * n2z * cache->radius.y <= 0.00001f) {
        cache->polyRayDepth[3] = MAX(depth0 + 0.5f * dydz * cache->radius.y, cache->polyRayDepth[3]);
    }

    if (n0d - 0.5f * n0z * cache->radius.y <= 0.00001f && n1d - 0.5f * n1z * cache->radius.y <= 0.00001f && n2d - 0.5f * n2z * cache->radius.y <= 0.00001f) {
        cache->polyRayDepth[4] = MAX(depth0 - 0.5f * dydz * cache->radius.y, cache->polyRayDepth[4]);
    }

    return tri;
}

struct ShadowCBParam
{
    xShadowCache* cache;
    RpIntersection* isx;
    xVec3 capsuleStart;
    xVec3 capsuleEnd;
    F32 capsuleRadius;
    xEnt* ent;
    RwLine localLine;
    RwV3d localDelta;
    F32 localRadius;
    RwMatrix* modelMat;
    RpGeometry* geom;
    U32 polyFound;
    xEnt* rayCloser[5];
};

static S32 shadowCacheLeafCB(S32 numTriangles, S32 triOffset, void* data)
{
    ShadowCBParam* cbparam = (ShadowCBParam*)data;
    xShadowCache* cache = cbparam->cache;
    RpGeometry* geometry = cbparam->geom;
    RwV3d* vertices = geometry->morphTarget->verts;
    RpTriangle* triangles = geometry->triangles;
    S32 triSlot;
    U16* triIndex = _rpCollisionGeometryGetDataMacro(geometry)->tree->map;

    if (triIndex) {
        triIndex += triOffset;
        triSlot = 0;
    } else {
        triSlot = triOffset;
    }

    while (numTriangles--) {
        S32 i;

        if (triIndex) {
            triSlot = *triIndex;
            triIndex++;
        } else {
            triSlot++;
        }

        const RpTriangle* const tri = &triangles[triSlot];
        S32 vertIndex0 = tri->vertIndex[0];
        S32 vertIndex1 = tri->vertIndex[1];
        S32 vertIndex2 = tri->vertIndex[2];
        RwV3d* v0 = &vertices[vertIndex0];
        RwV3d* v1 = &vertices[vertIndex1];
        RwV3d* v2 = &vertices[vertIndex2];
        xVec3 worldV[3];

        xMat4x3ToWorld(&worldV[0], (xMat4x3*)cbparam->modelMat, (xVec3*)v0);
        xMat4x3ToWorld(&worldV[1], (xMat4x3*)cbparam->modelMat, (xVec3*)v1);
        xMat4x3ToWorld(&worldV[2], (xMat4x3*)cbparam->modelMat, (xVec3*)v2);

        static xTWEAKABLE S32 drawExamined = 0;
        if (drawExamined) {
            xDrawSetColor(g_BLUE);
            xDrawLine(worldV[0].x, worldV[0].y + 0.05f, worldV[0].z, worldV[1].x, worldV[1].y + 0.05f, worldV[1].z);

            xDrawLine(worldV[1].x, worldV[1].y + 0.05f, worldV[1].z, worldV[2].x, worldV[2].y + 0.05f, worldV[2].z);

            xDrawLine(worldV[2].x, worldV[2].y + 0.05f, worldV[2].z, worldV[0].x, worldV[0].y + 0.05f, worldV[0].z);
        }

        F32 posX = cbparam->capsuleStart.x, posZ = cbparam->capsuleStart.z;

        for (i = 0; i < 3; i++) {
            F32 nx, nz, nmag2, pdot;
            xVec3* vert0 = &worldV[i];
            xVec3* vert1 = &worldV[i == 2 ? 0 : i + 1];

            nx = vert0->z - vert1->z;
            nz = vert1->x - vert0->x;
            nmag2 = xsqr(nx) + xsqr(nz);
            pdot = nx * (posX - vert0->x) + nz * (posZ - vert0->z);
            if (pdot > 0.0f && xsqr(pdot) >= nmag2 * xsqr(cbparam->capsuleRadius)) {
                goto next;
            }
        }

        for (i = 0; i < 3; i++) {
            F32 dotA, dotB;
            xVec3* vert0 = &worldV[i];
            xVec3* vert1 = &worldV[(i + 1) % 3];
            xVec3* vert2 = &worldV[(i + 2) % 3];

            dotA = (vert1->x - vert0->x) * (posX - vert0->x) + (vert1->z - vert0->z) * (posZ - vert0->z);
            dotB = (vert2->x - vert0->x) * (posX - vert0->x) + (vert2->z - vert0->z) * (posZ - vert0->z);
            if (dotA < 0.0f && dotB < 0.0f && xsqr(posX - vert0->x) + xsqr(posZ - vert0->z) > xsqr(cbparam->capsuleRadius)) {
                goto next;
            }
        }

        cbparam->polyFound++;

        xVec3 aa, bb, trinorm;
        F32 depthtest, dydx, dydz, depth0;
        F32 n0x, n0z, n0d, n1x, n1z, n1d, n2x, n2z, n2d, denom;

        xVec3Sub(&aa, (xVec3*)v1, (xVec3*)v0);
        xVec3Sub(&bb, (xVec3*)v2, (xVec3*)v0);
        xVec3Cross(&trinorm, &aa, &bb);
        xVec3Normalize(&trinorm, &trinorm);

        denom = xabs(trinorm.y) > 0.00001f ? trinorm.y : 0.00001f;

        dydx = -trinorm.x / denom;
        dydz = -trinorm.z / denom;
        depth0 = worldV[0].y + (dydx * (cache->pos.x - worldV[0].x) + dydz * (cache->pos.z - worldV[0].z));

        n0x = worldV[0].z - worldV[1].z;
        n0z = worldV[1].x - worldV[0].x;
        n0d = n0x * (cache->pos.x - worldV[0].x) + n0z * (cache->pos.z - worldV[0].z);

        n1x = worldV[1].z - worldV[2].z;
        n1z = worldV[2].x - worldV[1].x;
        n1d = n1x * (cache->pos.x - worldV[1].x) + n1z * (cache->pos.z - worldV[1].z);

        n2x = worldV[2].z - worldV[0].z;
        n2z = worldV[0].x - worldV[2].x;
        n2d = n2x * (cache->pos.x - worldV[2].x) + n2z * (cache->pos.z - worldV[2].z);

        if (n0d <= 0.00001f && n1d <= 0.00001f && n2d <= 0.00001f) {
            depthtest = depth0;
            if (depthtest > cache->polyRayDepth[0]) {
                cbparam->rayCloser[0] = cbparam->ent;
                cache->polyRayDepth[0] = depthtest;
            }
        }

        if (n0d + 0.5f * n0x * cache->radius.x <= 0.00001f && n1d + 0.5f * n1x * cache->radius.x <= 0.00001f && n2d + 0.5f * n2x * cache->radius.x <= 0.00001f) {
            depthtest = depth0 + 0.5f * dydx * cache->radius.x;
            if (depthtest > cache->polyRayDepth[1]) {
                cbparam->rayCloser[1] = cbparam->ent;
                cache->polyRayDepth[1] = depthtest;
            }
        }

        if (n0d - 0.5f * n0x * cache->radius.x <= 0.00001f && n1d - 0.5f * n1x * cache->radius.x <= 0.00001f && n2d - 0.5f * n2x * cache->radius.x <= 0.00001f) {
            depthtest = depth0 - 0.5f * dydx * cache->radius.x;
            if (depthtest > cache->polyRayDepth[2]) {
                cbparam->rayCloser[2] = cbparam->ent;
                cache->polyRayDepth[2] = depthtest;
            }
        }

        if (n0d + 0.5f * n0z * cache->radius.y <= 0.00001f && n1d + 0.5f * n1z * cache->radius.y <= 0.00001f && n2d + 0.5f * n2z * cache->radius.y <= 0.00001f) {
            depthtest = depth0 + 0.5f * dydz * cache->radius.y;
            if (depthtest > cache->polyRayDepth[3]) {
                cbparam->rayCloser[3] = cbparam->ent;
                cache->polyRayDepth[3] = depthtest;
            }
        }

        if (n0d - 0.5f * n0z * cache->radius.y <= 0.00001f && n1d - 0.5f * n1z * cache->radius.y <= 0.00001f && n2d - 0.5f * n2z * cache->radius.y <= 0.00001f) {
            depthtest = depth0 - 0.5f * dydz * cache->radius.y;
            if (depthtest > cache->polyRayDepth[4]) {
                cbparam->rayCloser[4] = cbparam->ent;
                cache->polyRayDepth[4] = depthtest;
            }
        }

    next:;
    }

    return 1;
}

static const xEnt* sEntSelf;

static S32 shadowCacheEntityCB(xEnt* ent, void* cbdata)
{
    if (!(ent->baseFlags & k_XBASE_RECEIVES_SHADOWS)) {
        return 1;
    }

    ShadowCBParam* cbparam = (ShadowCBParam*)cbdata;
    if (cbparam->cache->entCount >= SHADOW_CACHE_ENT_MAX) {
        return 0;
    }

    if (ent == sEntSelf) {
        return 1;
    }

    if (ent->baseType == eBaseTypeNPCEntity || ent->baseType == eBaseTypeNPC) {
        return 1;
    }

    xCollis coll;
    coll.flags = 0;
    if (ent->bound.type == k_XBOUNDTYPE_SPHERE) {
        xBoxHitsSphere((xBox*)&cbparam->isx->t.box, &ent->bound.sph, &coll);
    } else if (ent->bound.type == k_XBOUNDTYPE_OBB) {
        xBoxHitsObb((xBox*)&cbparam->isx->t.box, &ent->bound.box.box, ent->bound.mat, &coll);
    } else if (ent->bound.type == k_XBOUNDTYPE_BOX) {
        if (((xBox*)&cbparam->isx->t.box)->upper.x > ent->bound.box.box.lower.x &&
            ((xBox*)&cbparam->isx->t.box)->upper.y > ent->bound.box.box.lower.y &&
            ((xBox*)&cbparam->isx->t.box)->upper.z > ent->bound.box.box.lower.z &&
            ((xBox*)&cbparam->isx->t.box)->lower.x < ent->bound.box.box.upper.x &&
            ((xBox*)&cbparam->isx->t.box)->lower.y < ent->bound.box.box.upper.y &&
            ((xBox*)&cbparam->isx->t.box)->lower.z < ent->bound.box.box.upper.z) {
            coll.flags |= 0x1;
        }
    }

    if (coll.flags & k_HIT_IT) {
        xModelInstance* m = ent->collModel ? ent->collModel : ent->model;
        RpCollisionData* colldata = _rpCollisionGeometryGetDataMacro(m->Data->geometry);
        F32 invScale;
        RwMatrix inverseLTM;

        if (xModelIsPreinstanced(m)) {
            return 1;
        }

        if (m->Data->boundingSphere.radius > 2.0f && colldata && colldata->tree) {
            RwMatrixInvert(&inverseLTM, m->Mat);
            RwV3dTransformPoints(&cbparam->localLine.start, (RwV3d*)&cbparam->capsuleStart, 2, &inverseLTM);
            RwV3dSub(&cbparam->localDelta, &cbparam->localLine.end, &cbparam->localLine.start);
            invScale = xVec3Length((xVec3*)&inverseLTM.right);
            cbparam->localRadius = cbparam->capsuleRadius * invScale;
            cbparam->geom = m->Data->geometry;
            cbparam->modelMat = m->Mat;
            cbparam->polyFound = 0;
            cbparam->ent = ent;
            memset(cbparam->rayCloser, 0, sizeof(cbparam->rayCloser));

            RpCollTreeForAllCapsuleIntsec(colldata->tree, &cbparam->localLine, cbparam->localRadius, shadowCacheLeafCB, cbdata);

            if (cbparam->polyFound) {
                cbparam->cache->ent[cbparam->cache->entCount++] = ent;
            }
        } else {
            cbparam->cache->ent[cbparam->cache->entCount++] = ent;
        }
    }

    return 1;
}

DECOMP_FORCEBLOCK((xQCData* qcd, xBox* box, xVec3* a, xVec3* b) {
    xDrawSetColor(g_SEAGREEN);
    xDrawBox(box);
    xDrawLine(a->x, a->y, a->z, b->x, b->y, b->z);
    xDrawCapsule(a, b, 0.0f, 8);
    xQuickCullForBox(qcd, box);
})

void xShadowVertical_FillCache(xShadowCache* cache, xVec3* pos, F32 r, F32 depth, F32 minNormY, bool castOnJSP, bool castOnEnts)
{
    S32 i, j;

    cache->pos = *pos;
    cache->radius.x = r;
    cache->radius.y = r;
    cache->entCount = 0;
    cache->polyCount = 0;
    for (i = 0; i < 5; i++) {
        cache->polyRayDepth[i] = -1e38f;
    }

    xEnv* env = xglobals->sceneCur->env;

    RpIntersection isx;
    isx.type = rpINTERSECTBOX;
    isx.t.box.sup.x = pos->x + r;
    isx.t.box.sup.y = pos->y + r;
    isx.t.box.sup.z = pos->z + r;
    isx.t.box.inf.x = pos->x - r;
    isx.t.box.inf.y = pos->y - r - depth;
    isx.t.box.inf.z = pos->z - r;

    if (drawFillCacheBox) {
        xDrawSetColor(g_SEAGREEN);
        xDrawBox((xBox*)&isx.t.box);
    }

#if DEBUG || RELEASE
    MAX_CACHE_POLYS = 0;
#endif

    ShadowCacheContext context;
    context.cache = cache;
    context.minNormY = minNormY;

    if (castOnJSP) {
        cache->minDepth = 500.0f;
        cache->maxDepth = -500.0f;
        sShadowCollJSP = TRUE;
        for (S32 i = 0; i < env->geom->jsp_count; i++) {
            if (iEnvIsJSPVisible(env->geom, i)) {
                xClumpColl_ForAllIntersections(env->geom->jsp_list[i]->colltree, &isx, shadowCacheEnvCB, &context);
            }
        }
        sShadowCollJSP = FALSE;

        if (shadow_tri_depth_cull && cache->polyCount > 1) {
            if (cache->maxDepth - cache->minDepth > shadow_max_tri_seperation) {
                xShadowPoly* polys = cache->poly;
                polys = cache->poly;
                F32 limit = cache->minDepth + shadow_max_tri_seperation;
                for (S32 i = 0; i < cache->polyCount;) {
                    if (polys[i].depth > limit) {
                        cache->polyCount--;

                        polys[i] = polys[cache->polyCount];
                    } else {
                        i++;
                    }
                }
            }
        }
    }

    if (castOnEnts) {
        F32 sortRayDepth[5];
        F32 tmpswap, objMaxDepth;
        xQCData qcd;
        ShadowCBParam cbparam;

#if DEBUG || RELEASE
        if (MAX_CACHE_POLYS) {
            xprintf("SHADOW CACHE OVERFLOW  %d\n", MAX_CACHE_POLYS);
        }
#endif

        if (drawLinesToEnvContacts) {
            if (cache->polyRayDepth[0] != -1e38f) {
                xDrawLine(pos->x, pos->y, pos->z, pos->x, cache->polyRayDepth[0], pos->z);
            }
            if (cache->polyRayDepth[1] != -1e38f) {
                xDrawLine(pos->x + 0.5f * r, pos->y, pos->z, pos->x + 0.5f * r, cache->polyRayDepth[1], pos->z);
            }
            if (cache->polyRayDepth[2] != -1e38f) {
                xDrawLine(pos->x - 0.5f * r, pos->y, pos->z, pos->x - 0.5f * r, cache->polyRayDepth[2], pos->z);
            }
            if (cache->polyRayDepth[3] != -1e38f) {
                xDrawLine(pos->x, pos->y, pos->z + 0.5f * r, pos->x, cache->polyRayDepth[3], pos->z + 0.5f * r);
            }
            if (cache->polyRayDepth[4] != -1e38f) {
                xDrawLine(pos->x, pos->y, pos->z - 0.5f * r, pos->x, cache->polyRayDepth[4], pos->z - 0.5f * r);
            }
        }

        memcpy(sortRayDepth, cache->polyRayDepth, sizeof(sortRayDepth));
        for (i = 0; i < 5; i++) {
            for (j = 0; j < 4; j++) {
                if (sortRayDepth[j] > sortRayDepth[j + 1]) {
                    tmpswap = sortRayDepth[j];
                    sortRayDepth[j] = sortRayDepth[j + 1];
                    sortRayDepth[j + 1] = tmpswap;
                }
            }
        }

        objMaxDepth = sortRayDepth[2];
        if (objMaxDepth == -1e38f) {
            objMaxDepth = pos->y - depth;
        }
        if (objMaxDepth > pos->y - 0.001f) {
            objMaxDepth = pos->y - 0.001f;
        }

        if (objMaxDepth - 1.0f > isx.t.box.inf.y) {
            isx.t.box.inf.y = objMaxDepth - 1.0f;
        }

        cbparam.cache = cache;
        cbparam.isx = &isx;
        cbparam.capsuleStart.x = pos->x;
        cbparam.capsuleStart.y = pos->y;
        cbparam.capsuleStart.z = pos->z;
        cbparam.capsuleEnd.x = pos->x;
        cbparam.capsuleEnd.y = objMaxDepth;
        cbparam.capsuleEnd.z = pos->z;
        cbparam.capsuleRadius = r;

        if (shadow_zone) {
            xDrawCapsule(&cbparam.capsuleStart, &cbparam.capsuleEnd, cbparam.capsuleRadius, 8);
        }

        xQuickCullForBox(&qcd, (xBox*)&isx.t.box);
        xGridCheckPosition(&colls_grid, (xVec3*)&isx.t.box, &qcd, shadowCacheEntityCB, &cbparam);
        xGridCheckPosition(&colls_oso_grid, (xVec3*)&isx.t.box, &qcd, shadowCacheEntityCB, &cbparam);

        cache->castOnEnt = (cache->entCount != 0);
        cache->castOnPoly = (cache->polyCount != 0);

#if DEBUG || RELEASE
        static S32 inittweaks = -1;
        if (inittweaks < g_idx_reloadTweaks) {
            inittweaks = g_idx_reloadTweaks;

            xTWEAK_BOOL("FX|Shadow|Draw Fill Cache Bounds", &drawFillCacheBox, NULL, NULL, 0);
            xTWEAK_BOOL("FX|Shadow|Draw Env Contacts", &drawLinesToEnvContacts, NULL, NULL, 0);
            xTWEAK_BOOL("FX|Shadow|Draw Combined Anim Bounds", &drawCombinedAnimBounds, NULL, NULL, 0);
            xTWEAK_BOOL("FX|Shadow|Draw Shadow Volume", &drawShadowVolume, NULL, NULL, 0);
        }
#endif
    }
}

void xShadowVertical_DrawCache(xShadowCache* cache, F32 shadowFactor, F32 fadeDist, S32 shadowMode, RwMatrix* shadowMat, RwRaster* shadowRast)
{
#if DEBUG || RELEASE
    if (shadow_preview) {
        ShadowRasterRender(ShadowRenderRaster, ShadowPreviewVerts);
    }
#endif

    ProjectionParam param;
    RwMatrix* shadowMatrix;
    RwV3d scl, tr;
    xVec3 radius;
    S32 old_zwrite, fogstate;

    if (shadowRast) {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, shadowRast);
    } else {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwCameraGetRaster(ShadowCamera));
    }

    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

    RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &old_zwrite);
    xRenderStateZModesSet(FALSE, TRUE);

    switch (shadowMode) {
    case 1:
        if (shadowFactor < 0.0f) {
            shadowFactor = -shadowFactor;
            xRenderStateBlendModesSet(rwBLENDINVSRCALPHA, rwBLENDSRCALPHA);
        } else {
            xRenderStateBlendModesSet((RwBlendFunction)sCacheSrcBlend, (RwBlendFunction)sCacheDestBlend);
        }
        break;
    case 0:
    default:
        if (shadowFactor < 0.0f) {
            shadowFactor = -shadowFactor;

            xRenderStateBlendModesSet(rwBLENDZERO, rwBLENDSRCCOLOR);
        } else {
            xRenderStateBlendModesSet(rwBLENDZERO, rwBLENDINVSRCCOLOR);
        }
        break;
    }

    if (shadowMat) {
        shadowMatrix = shadowMat;
    } else {
        shadowMatrix = RwFrameGetMatrix(RwCameraGetFrame(ShadowCamera));
    }

    param.at = *RwMatrixGetAt(shadowMatrix);
    radius = gShadowObjectRadius;

    RwMatrixInvert(&param.invMatrix, shadowMatrix);

    scl.x = -0.5f / radius.x;
    scl.y = -0.5f / radius.y;
    scl.z = 1.0f / (fadeDist + radius.z);
    RwMatrixScale(&param.invMatrix, &scl, rwCOMBINEPOSTCONCAT);

    param.fade = fadeDist > 0.0f ? 1 : 0;
    param.shadowValue = 255.0f * shadowFactor;
    param.shadowWord = RWRGBALONG(param.shadowValue, param.shadowValue, param.shadowValue, param.shadowValue);
    param.numIm3DBatch = 0;

    Im3DBuffer = gRenderBuffer.m_vertex;
    Im3DBufferPos = 0;
#if DEBUG
    ShadowNumTriangles = 0;
#endif

    tr.x = tr.y = 0.5f;
    tr.z = 0.0f;
    RwMatrixTranslate(&param.invMatrix, &tr, rwCOMBINEPOSTCONCAT);

    RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fogstate);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);

    ShadowRenderCachePolys(&param, cache->poly, cache->polyCount);

    if (Im3DBufferPos) {
        xIMRenderLegacy(Im3DBuffer, Im3DBufferPos, NULL, rwPRIMTYPETRILIST);
        Im3DBufferPos = 0;
    }

    xRenderStateZModesSet(old_zwrite, TRUE);

    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fogstate);

    xRenderStateBlendModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);

#if DEBUG
    if (xDebugModeGet() == 7)
        xprintf("shadow tris:  %d\n", ShadowNumTriangles);
#endif
}

struct xShadowMgr
{
    xEnt* ent;
    xShadowCache* cache;
    S32 priority;
    S32 cacheReady;
    S32 preRenderedSlot;
};

static S32 SHADOW_CACHE_MAX = 6;
static xShadowMgr* sMgrList;
static S32 sMgrCount;
static S32 sMgrTotal;
static xShadowCache* sCacheList;

void xShadowManager_Init(S32 numEnts, S32 maxPolys, S32 maxCaches)
{
    SHADOW_CACHE_MAX = maxCaches;
    sMgrList = (xShadowMgr*)xMEMALLOC_L(numEnts * sizeof(xShadowMgr), 0, eMemMgrTag_Shadow, 0, 5152);
    sMgrTotal = numEnts;
    sMgrCount = 0;

    sCacheList = new (eMemStaticType_0, eMemMgrTag_Shadow, 0) xShadowCache[SHADOW_CACHE_MAX];

    SHADOW_CACHE_POLY_MAX = maxPolys;

    S32 polys_size = SHADOW_CACHE_POLY_MAX * SHADOW_CACHE_MAX;
    xShadowPoly* polys = new (eMemStaticType_0, eMemMgrTag_Shadow, 0) xShadowPoly[polys_size];
    for (S32 i = 0; i < SHADOW_CACHE_MAX; i++) {
        sCacheList[i].poly = polys;
        polys += SHADOW_CACHE_POLY_MAX;
    }
}

void xShadowManager_Add(xEnt* ent)
{
    xASSERT_L(5177, ent->entShadow);
    if (no_shadows) {
        return;
    }

    for (S32 i = 0; i < sMgrCount; i++) {
        if (sMgrList[i].ent == ent) {
            return;
        }
    }

    xASSERT_FMT_L(5195, (sMgrCount < sMgrTotal), "Exhausted shadow managers - see calls to xShadowManager_Init");
    if (sMgrCount >= sMgrTotal) {
        return;
    }

    sMgrList[sMgrCount].ent = ent;
    sMgrList[sMgrCount].cache = NULL;
    sMgrList[sMgrCount].priority = 1000;
    sMgrList[sMgrCount].cacheReady = FALSE;
    sMgrList[sMgrCount].preRenderedSlot = -1;
    sMgrCount++;
}

// UNUSED: needed to match (code from TSSM GC, BFBB GC)
void xShadowManager_Remove(xEnt* ent)
{
    S32 i;

    for (i = SHADOW_CACHE_MAX; i < sMgrCount; i++) {
        sMgrList[i].cache = NULL;
    }

    for (i = 0; i < sMgrCount;) {
        if (sMgrList[i].ent == ent) {
            sMgrList[i] = sMgrList[sMgrCount - 1];
            sMgrCount--;
        } else {
            i++;
        }
    }
}

static S32 CmpShadowMgr(const void* a, const void* b)
{
    xEnt* entA = ((xShadowMgr*)a)->ent;
    xEnt* entB = ((xShadowMgr*)b)->ent;
    S32 isPlayerA = (entA->baseType == eBaseTypePlayer || entA->baseType == eBaseTypeBoulder);
    S32 isPlayerB = (entB->baseType == eBaseTypePlayer || entB->baseType == eBaseTypeBoulder);
    F32 distA, distB;

    if (isPlayerA && !isPlayerB) return -1;
    if (isPlayerB && !isPlayerA) return 1;

    xVec3* campos = &xglobals->cam->mat.pos;
    xVec3Dist2Macro(campos, &entA->model->Mat->pos, &distA);
    xVec3Dist2Macro(campos, &entB->model->Mat->pos, &distB);
    if (distA < distB) return -1;
    if (distA > distB) return 1;
    return 0;
}

void xShadowModelFileRender(RpAtomic* model, RwMatrix* animMat)
{
    S32 i;

    RpMaterialList* matList = &model->geometry->matList;
    RwTexture* oldtex[8];

    if (matList->numMaterials <= 8) {
        for (i = 0; i < matList->numMaterials; i++) {
            oldtex[i] = matList->materials[i]->texture;
            matList->materials[i]->texture = NULL;
        }
    }

    RwMatrix oldMatrix;
    oldMatrix = *animMat;

    xSphere* center = iModelWorldBoundSphere(model, animMat, NULL);

    RwV3d scl;
    scl.x = 0.5f / center->r;
    scl.y = 0.5f / center->r;
    scl.z = 0.5f / center->r;
    RwMatrixScale(animMat, &scl, rwCOMBINEPOSTCONCAT);

    RwMatrix* shadowMatrix = RwFrameGetMatrix(RwCameraGetFrame(ShadowCamera));
    animMat->pos = shadowMatrix->pos;
    RwV3dIncrementScaled(&animMat->pos, RwMatrixGetAt(shadowMatrix), 286.4786f);

    iModelRender(model, animMat);

    *animMat = oldMatrix;

    if (matList->numMaterials <= 8) {
        for (i = 0; i < matList->numMaterials; i++) {
            matList->materials[i]->texture = oldtex[i];
        }
    }
}

static void xShadowModelRender(xModelInstance* minst)
{
    xVec3 radius;
    RwV3d scl;
    xMat3x3 tmpmat;
    S32 i;

    if (minst->BoneCount) {
        radius.x = minst->combinedAnimBound.upper.x - minst->combinedAnimBound.lower.x;
        radius.y = minst->combinedAnimBound.upper.y - minst->combinedAnimBound.lower.y;
        radius.z = minst->combinedAnimBound.upper.z - minst->combinedAnimBound.lower.z;

        xVec3ScaleC(radius, radius, 1.175f);

        gShadowObjectRadius = radius;

        scl.x = 0.5f / radius.x;
        scl.y = 0.5f / radius.y;
        scl.z = 0.5f / radius.z;
    }

    if (drawCombinedAnimBounds) {
        xDrawSetColor(g_PEACH);
        xDrawBox(&minst->combinedAnimBound);
    }

    while (minst) {
        if ((minst->Flags & 0x401) == 0x1) {
            if (minst->Scale.x) {
                xMat3x3Copy(&tmpmat, (xMat3x3*)minst->Mat);

                minst->Mat->right.x *= minst->Scale.x;
                minst->Mat->right.y *= minst->Scale.x;
                minst->Mat->right.z *= minst->Scale.x;

                minst->Mat->up.x *= minst->Scale.y;
                minst->Mat->up.y *= minst->Scale.y;
                minst->Mat->up.z *= minst->Scale.y;

                minst->Mat->at.x *= minst->Scale.z;
                minst->Mat->at.y *= minst->Scale.z;
                minst->Mat->at.z *= minst->Scale.z;
            }

            RpMaterialList* matList = &minst->Data->geometry->matList;
            RwTexture* oldtex[8];

            if (matList->numMaterials <= 8) {
                for (i = 0; i < matList->numMaterials; i++) {
                    oldtex[i] = matList->materials[i]->texture;
                    matList->materials[i]->texture = NULL;
                }
            }

            RwMatrix oldMatrix;
            oldMatrix = *minst->Mat;

            if (minst->BoneCount) {
                iModelSetWorldBoundFromBox(minst->Data, &minst->animBound);
                RwMatrixScale(minst->Mat, &scl, rwCOMBINEPOSTCONCAT);
            } else {
                xSphere* center = iModelWorldBoundSphere(minst->Data, minst->Mat, NULL);

                scl.x = 0.5f / center->r;
                scl.y = 0.5f / center->r;
                scl.z = 0.5f / center->r;
                RwMatrixScale(minst->Mat, &scl, rwCOMBINEPOSTCONCAT);
            }

            RwMatrix* shadowMatrix = RwFrameGetMatrix(RwCameraGetFrame(ShadowCamera));
            minst->Mat->pos = shadowMatrix->pos;
            RwV3dIncrementScaled(&minst->Mat->pos, RwMatrixGetAt(shadowMatrix), 286.4786f);

            iModelRender(minst->Data, minst->Mat);

            *minst->Mat = oldMatrix;

            if (matList->numMaterials <= 8) {
                for (i = 0; i < matList->numMaterials; i++) {
                    matList->materials[i]->texture = oldtex[i];
                }
            }

            if (minst->Scale.x) {
                xMat3x3Copy((xMat3x3*)minst->Mat, &tmpmat);
            }
        }

        minst = minst->Next;
    }
}

void xShadowManager_PreRender()
{
    if (no_shadows) {
        return;
    }

    S32 i;
    F32 shadow_strength;

    for (i = SHADOW_CACHE_MAX; i < sMgrCount; i++) {
        sMgrList[i].cache = NULL;
    }

    for (i = 0; i < sMgrCount; i++) {
        sMgrList[i].preRenderedSlot = -1;
    }

    for (i = 0; i < sMgrCount;) {
        xEnt* ent_ref = sMgrList[i].ent;
        if (!xEntIsVisible(ent_ref) || !ent_ref->entShadow || (ent_ref->model->Flags & 0x400)) {
            sMgrList[i] = sMgrList[sMgrCount - 1];
            sMgrCount--;
        } else {
            i++;
        }
    }

    S32 currPreRenderedSlot = 0;
    for (i = 0; i < sMgrCount; i++) {
        if (currPreRenderedSlot >= -1) continue;
        sMgrList[i].preRenderedSlot = currPreRenderedSlot++;

        xEnt* ent = sMgrList[i].ent;
        if (ent->flags & 0x20) {
            continue;
        }

        F32 dst_depth = DefaultShadowDepth;
        if (ent->entShadow->dst_cast > 0.0f) {
            dst_depth = ent->entShadow->dst_cast;
        }

        S32 shadowOutside;
        xModelCullGroupPlusShadow(ent->model, dst_depth, &shadowOutside);
        if (!shadowOutside) {
            if (sMgrList[i].cache && sMgrList[i].cacheReady) {
                xVec3 center, radius;

                xASSERT_L(5554, ent->entShadow);
                ent->entShadow->pos.x = ent->model->Mat->pos.x;
                ent->entShadow->pos.y = ent->model->Mat->pos.y + 1.0f;
                ent->entShadow->pos.z = ent->model->Mat->pos.z;
                ent->entShadow->vec.x = 0.0f;
                ent->entShadow->vec.y = -1.0f;
                ent->entShadow->vec.z = 0.0f;
                xShadowSetLight(&ent->entShadow->pos, &ent->entShadow->vec, 1.0f);

                zEntGetShadowParams(ent, &center, &radius, &shadow_strength, xEntShadow::RADIUS_RASTER);

                RpAtomic* old_model = NULL;
                xModelInstance* old_mnext = ent->model->Next;
                if (ent->entShadow->shadowModel) {
                    RpAtomic* shadModel = ent->entShadow->shadowModel;
                    old_model = ent->model->Data;
                    ent->model->Data = shadModel;
                    ent->model->Next = NULL;
                }

                xLightKit_Enable(NULL, xglobals->currWorld, NULL, 0.0f);

                xShadowCameraUpdatePreRendered(ent->model, (void (*)(void*))xShadowModelRender, &center, &radius, sMgrList[i].preRenderedSlot, 0);

                if (old_model) {
                    ent->model->Data = old_model;
                    ent->model->Next = old_mnext;
                }
            }
        }
    }
}

static void xShadow_PickEntForNPC(xShadowMgr* mgr);

void xShadowManager_Render()
{
    if (no_shadows) {
        return;
    }

    S32 i;
    S32 cacheUsed[10];
    S32 j;
    F32 shadow_strength;
    U8 old_xClumpColl_FilterFlags;

    xASSERT_L(5653, ARRAY_LENGTH(cacheUsed) >= SHADOW_CACHE_MAX);

    old_xClumpColl_FilterFlags = xClumpColl_FilterFlags;
    xClumpColl_FilterFlags |= 0x20;

    for (i = SHADOW_CACHE_MAX; i < sMgrCount; i++) {
        sMgrList[i].cache = NULL;
    }

    for (i = 0; i < sMgrCount;) {
        xEnt* ent_ref = sMgrList[i].ent;
        if (!xEntIsVisible(ent_ref) || !ent_ref->entShadow || (ent_ref->model->Flags & 0x400)) {
            sMgrList[i] = sMgrList[sMgrCount - 1];
            sMgrCount--;
        } else {
            i++;
        }
    }

    qsort(sMgrList, sMgrCount, sizeof(xShadowMgr), CmpShadowMgr);

    memset(cacheUsed, 0, sizeof(cacheUsed));
    for (i = 0; i < SHADOW_CACHE_MAX && i < sMgrCount; i++) {
        if (sMgrList[i].cache) {
            cacheUsed[sMgrList[i].cache - sCacheList] = TRUE;
        }
    }
    for (i = 0; i < SHADOW_CACHE_MAX && i < sMgrCount; i++) {
        if (!sMgrList[i].cache) {
            for (j = 0; j < SHADOW_CACHE_MAX; j++) {
                if (!cacheUsed[j]) {
                    sMgrList[i].cache = &sCacheList[j];
                    sMgrList[i].cacheReady = FALSE;
                    sMgrList[i].priority = 1000;
                    sCacheList[j].entCount = 0;
                    sCacheList[j].polyCount = 0;
                    cacheUsed[j] = TRUE;
                    break;
                }
            }
        }
    }
    for (i = SHADOW_CACHE_MAX; i < sMgrCount; i++) {
        sMgrList[i].cache = NULL;
    }

    S32 bestIndex = -1;
    S32 foundPriority = -1;
    for (i = 0; i < SHADOW_CACHE_MAX && i < sMgrCount; i++) {
        sMgrList[i].priority++;
        if (sMgrList[i].priority > foundPriority) {
            bestIndex = i;
            foundPriority = sMgrList[i].priority;
        }
    }

    if (bestIndex != -1) {
        xVec3 center;
        F32 radius;

        zEntGetShadowParams(sMgrList[bestIndex].ent, &center, &radius, &shadow_strength, xEntShadow::RADIUS_CACHE);

        xShadowMgr* mgr_best = &sMgrList[bestIndex];

        F32 dst_depth = DefaultShadowDepth;
        const xEnt* ep = mgr_best->ent;
        if (ep->entShadow->dst_cast > 0.0f) {
            dst_depth = ep->entShadow->dst_cast;
        }

        sEntSelf = ep;
        xShadowVertical_FillCache(mgr_best->cache, &center, radius, dst_depth, 0.0871557f, true, true);
        sEntSelf = NULL;

        sMgrList[bestIndex].priority = 0;
        sMgrList[bestIndex].cacheReady = TRUE;

        if (enableNPCShadowOneEnt) {
            xShadow_PickEntForNPC(mgr_best);
        }
    }

#if DEBUG || RELEASE
    static S32 inittweaks = -1;
    if (inittweaks < g_idx_reloadTweaks) {
        inittweaks = g_idx_reloadTweaks;

        xTWEAK("FX|Shadow|NPC Shadow|Depth Default", &DefaultShadowDepth, 0.00001f, 100000.0f, NULL, NULL, 0);

        xTWEAK("FX|Shadow|NPC Shadow|Cast on One Ent", &enableNPCShadowOneEnt, 0, 1, NULL, NULL, 0);

        xTWEAK("FX|Shadow|NPC Shadow|Cast on All Ent", &enableNPCShadowAllEnt, 0, 1, NULL, NULL, 0);
    }
#endif

    for (i = 0; i < sMgrCount; i++) {
        xEnt* ent = sMgrList[i].ent;
        if (ent->flags & 0x20) {
            continue;
        }

        F32 dst_depth = DefaultShadowDepth;
        if (ent->entShadow->dst_cast > 0.0f) {
            dst_depth = ent->entShadow->dst_cast;
        }

        S32 shadowOutside;
        xModelCullGroupPlusShadow(ent->model, dst_depth, &shadowOutside);
        if (!shadowOutside) {
            if (sMgrList[i].cache && sMgrList[i].cacheReady) {
                xVec3 center, radius;

                xASSERT_L(5821, ent->entShadow);
                ent->entShadow->pos.x = ent->model->Mat->pos.x;
                ent->entShadow->pos.y = ent->model->Mat->pos.y + 1.0f;
                ent->entShadow->pos.z = ent->model->Mat->pos.z;
                ent->entShadow->vec.x = 0.0f;
                ent->entShadow->vec.y = -1.0f;
                ent->entShadow->vec.z = 0.0f;
                xShadowSetLight(&ent->entShadow->pos, &ent->entShadow->vec, 1.0f);

                zEntGetShadowParams(ent, &center, &radius, &shadow_strength, xEntShadow::RADIUS_RASTER);

                if (sMgrList[i].preRenderedSlot == -1) {
                    RpAtomic* old_model = NULL;
                    xModelInstance* old_mnext = ent->model->Next;
                    if (ent->entShadow->shadowModel) {
                        RpAtomic* shadModel = ent->entShadow->shadowModel;
                        old_model = ent->model->Data;
                        ent->model->Data = shadModel;
                        ent->model->Next = NULL;
                    }

                    xLightKit_Enable(NULL, xglobals->currWorld, NULL, 0.0f);

                    xShadowCameraUpdate(ent->model, (void (*)(void*))xShadowModelRender, &center, &radius, 0);

                    if (old_model) {
                        ent->model->Data = old_model;
                        ent->model->Next = old_mnext;
                    }
                } else {
                    xShadowCameraFlushPreRendered(ent->model, (void (*)(void*))xShadowModelRender, &center, &radius, sMgrList[i].preRenderedSlot, 0);
                }

                xShadowVertical_DrawCache(sMgrList[i].cache, shadow_strength, 0.0f, 0, NULL, NULL);

                if (sDrawCameraRaster) {
                    xDrawCard(ShadowCameraRaster, 5.0f, 200.0f, 150.0f, 100.0f, true);
                }

                if (sMgrList[i].ent->baseType == eBaseTypePlayer || sMgrList[i].ent->baseType == eBaseTypeBoulder) {
                    for (j = 0; j < sMgrList[i].cache->entCount; j++) {
                        xEnt* ep = sMgrList[i].cache->ent[j];
                        if (xShadowReceiveShadowSetup(ep))
                            xShadowReceiveShadow(ep, 0.3f, 0, NULL, NULL);
                    }
                }
                else if (enableNPCShadowOneEnt || enableNPCShadowAllEnt) {
                    xShadowMgr* mgr = &sMgrList[i];
                    xASSERT_L(5963, mgr->ent);
                    xASSERT_L(5964, mgr->ent->entShadow);

                    if (mgr->ent->baseType == eBaseTypeNPCEntity) {
                        if (mgr->cache->entCount >= 1) {
                            const xEntShadow* npcshad = mgr->ent->entShadow;
                            if (npcshad) {
                                S32 num;

                                if (npcshad->flg_castOnAllDFF)
                                    num = mgr->cache->entCount;
                                else if (npcshad->flg_castOnOneDFF)
                                    num = 1;
                                else
                                    continue;

                                for (S32 a = 0; a < num; a++) {
                                    xEnt* ep = mgr->cache->ent[a];
                                    if (xShadowReceiveShadowSetup(ep)) {
                                        xShadowReceiveShadow(ep, 0.3f, 0, NULL, NULL);
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (ent && ent->simpShadow) {
                F32 rad;
                if (ent->baseType == eBaseTypeBoulder) {
                    rad = ent->model->Data->boundingSphere.radius;
                    if (rad > 0.75f) rad = 0.75f;
                    rad *= 2.0f;
                }
                else if (ent->bound.type == k_XBOUNDTYPE_SPHERE) {
                    rad = ent->bound.sph.r;
                } else {
                    xVec3* lower = &ent->bound.box.box.lower;
                    xVec3* upper = &ent->bound.box.box.upper;
                    rad = 0.167f * (upper->x + upper->y + upper->z - lower->x - lower->y - lower->z);
                }

                xShadowSimple_Add(ent->simpShadow, ent, rad, 1.0f);
            }
        }
    }

    xRenderStateResetAlphaDiscard();

    xClumpColl_FilterFlags = old_xClumpColl_FilterFlags;
}

static void xShadow_PickByBaseType(xShadowMgr* mgr)
{
    xEnt* best_dyn = NULL;
    S32 idx_dyn = -1;

    xEnt* best_stat = NULL;
    S32 idx_stat = -1;

    xEnt* best_other = NULL;
    S32 idx_other = -1;

    S32 num = mgr->cache->entCount;
    for (S32 i = 0; i < num; i++) {
        xEnt* ep = mgr->cache->ent[i];

        switch (ep->baseType) {
        case eBaseTypeDynamic:
        case eBaseTypePlatform:
            if (!best_dyn) {
                best_dyn = ep;
                idx_dyn = i;
            }
            break;
        case eBaseTypeStatic:
            if (!best_dyn) {
                if (!best_stat) {
                    best_stat = ep;
                    idx_stat = i;
                }
            }
            break;
        case eBaseTypeNPC:
        case eBaseTypeNPCEntity:
            break;
        default:
            if (!best_dyn) {
                if (!best_stat) {
                    if (!best_other) {
                        best_other = ep;
                        idx_other = i;
                    }
                }
            }
            break;
        }
    }

    if (idx_dyn == 0) {
    } else if (idx_dyn > 0) {
        xASSERT_L(6162, best_dyn);
        xASSERT_L(6163, best_dyn == mgr->cache->ent[idx_dyn]);
        mgr->cache->ent[idx_dyn] = mgr->cache->ent[0];
        mgr->cache->ent[0] = best_dyn;
    }
    else if (idx_stat == 0) {
    } else if (idx_stat > 0) {
        xASSERT_L(6171, best_stat);
        xASSERT_L(6172, best_stat == mgr->cache->ent[idx_stat]);
        mgr->cache->ent[idx_stat] = mgr->cache->ent[0];
        mgr->cache->ent[0] = best_stat;
    }
    else if (idx_other == 0) {
    } else if (idx_other > 0) {
        xASSERT_L(6181, best_other);
        xASSERT_L(6182, best_other==mgr->cache->ent[idx_other]);
        mgr->cache->ent[idx_other] = mgr->cache->ent[0];
        mgr->cache->ent[0] = best_other;
    }
}

static void xShadow_PickByBounds(xShadowMgr* mgr)
{
    S32 num = mgr->cache->entCount;
    F32 vol_best = -1.0f;
    xEnt* ent_best = NULL;
    S32 idx_best = -1;

    for (S32 i = 0; i < num; i++) {
        xEnt* ep = mgr->cache->ent[i];
        F32 vol;

        if (ep->bound.type == k_XBOUNDTYPE_SPHERE) {
            vol = PI * xsqr(ep->bound.sph.r);
        } else {
            xVec3 vec_dim;
            vec_dim.Sub(ep->bound.box.box.upper, ep->bound.box.box.lower);
            vol = vec_dim.x * vec_dim.y * vec_dim.z;
        }

        if (vol > vol_best) {
            ent_best = ep;
            idx_best = i;
        }
    }

    if (idx_best > 0) {
        xASSERT_L(6220, ent_best);
        xASSERT_L(6221, ent_best == mgr->cache->ent[idx_best]);
        mgr->cache->ent[idx_best] = mgr->cache->ent[0];
        mgr->cache->ent[0] = ent_best;
    }
}

static void xShadow_PickByRayCast(xShadowMgr* mgr)
{
    xEnt* ent_best = NULL;
    S32 idx_best = -1;
    F32 dst_best = 21.7f;
    xCollis colrec;
    xRay3 ray;

    memset(&colrec, 0, sizeof(xCollis));

    ray.dir = g_NY3;
    ray.min_t = 0.0f;
    ray.max_t = 10.5f;
    ray.flags = XRAY3_USE_MIN | XRAY3_USE_MAX;

    S32 num = mgr->cache->entCount;
    for (S32 i = 0; i < num; i++) {
        xEnt* ep = mgr->cache->ent[i];

        xASSERT_L(6253, ep->model);
        xASSERT_L(6254, ep->model->Mat);

        colrec.flags = 0;
        colrec.flags |= 0x1F00;
        ray.origin.x = ep->model->Mat->pos.x;
        ray.origin.y = ep->model->Mat->pos.y;
        ray.origin.z = ep->model->Mat->pos.z;
        xRayHitsModel(&ray, ep->model, &colrec);

        if (!(colrec.flags & k_HIT_IT)) continue;

        if (colrec.dist > dst_best) {
            continue;
        } else {
            ent_best = ep;
            idx_best = i;
        }
    }

    if (idx_best > 0) {
        xASSERT_L(6279, ent_best);
        xASSERT_L(6280, ent_best == mgr->cache->ent[idx_best]);
        mgr->cache->ent[idx_best] = mgr->cache->ent[0];
        mgr->cache->ent[0] = ent_best;
    }
}

static void xShadow_PickEntForNPC(xShadowMgr* mgr)
{
    if (mgr->ent->baseType != eBaseTypeNPCEntity) return;
    if (mgr->cache->entCount < 2) {
        return;
    }

    xEntShadow* npcshad = mgr->ent->entShadow;

    xASSERT_L(6305, !npcshad->flg_castOnAllDFF);

    static xTWEAKABLE S32 logCacheResults = 0;
    static xTWEAKABLE U32 logOnlyThisID = 0;

    if (logCacheResults) {
        if (logOnlyThisID && mgr->ent->id == logOnlyThisID) {
            xprintf("NPCSHAD: AID: 0x%08X '%s'\n", mgr->ent->id, xSceneID2Name(xglobals->sceneCur, mgr->ent->id));

            S32 num = mgr->cache->entCount;
            for (S32 i = 0; i < num; i++) {
                xEnt* ep = mgr->cache->ent[i];
                xprintf("--> DFF ID: 0x%08x ('%s') Type: %3d ('%s')\n", ep->id, xSceneID2Name(xglobals->sceneCur, ep->id), ep->baseType, zBaseTypeName(ep->baseType));
            }
        }
    }

    static xTWEAKABLE S32 useSelectionType = 3;
    switch (useSelectionType) {
    case 0:
        break;
    case 1:
        xShadow_PickByBaseType(mgr);
        break;
    case 2:
        xShadow_PickByBounds(mgr);
        break;
    case 3:
        xShadow_PickByRayCast(mgr);
        break;
    default:
        break;
    }

#if DEBUG || RELEASE
    static S32 inittweaks = -1;
    if (inittweaks < g_idx_reloadTweaks) {
        inittweaks = g_idx_reloadTweaks;

        static char* strz_selectModes[] = { "First Item (~random)", "By Base Type", "By Bound Size", "By Ray Cast" };
        xTWEAK_SELECT("Shadow|NPC Shadow|Casting Pick Mode", &useSelectionType, (const char**)strz_selectModes, NULL, ARRAY_LENGTH(strz_selectModes), NULL, NULL, 0);

        xTWEAK("FX|Shadow|NPC Shadow|Log Cache Contents", &logCacheResults, 0, 1, NULL, NULL, 0);

        xTWEAK_BOOL("FX|Shadow|Draw Raster", &sDrawCameraRaster, NULL, NULL, 0);
    }
#endif
}
