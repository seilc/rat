#include "xCommon.h"
#include "xFX.h"

#include "xDebug.h"
#include "xDebugTweak.h"
#include "xFXHighDynamicRange.h"
#include "xCam.h"
#include "xGlobals.h"
#include "xIM.h"
#include "xMath.h"
#include "xMemMgr.h"
#include "xRand.h"
#include "xRenderBuffer.h"
#include "xRenderState.h"
#include "xScene.h"
#include "xString.h"
#include "xstransvc.h"
#include "xutil.h"
#include "iFX.h"
#include "zAutoTweak.h"
#include "zFX.h"
#include "zPlayer.h"
#include "zRenderState.h"
#include "zScene.h"
#include "zSurface.h"

#include "decomp.h"

#include <string.h>
#include <rpmatfx.h>

#define GLOSS_EDGE_ID 0xB8C2351E // xStrHash("gloss_edge")
#define RAINBOWFILM_SMOOTH32_ID 0x741B0566 // xStrHash("rainbowfilm_smooth32")

#define MAX_RINGS 8
#define MAX_CURVE_NODES 32
#define MAX_STRIP_JOINTS 240

static void LightResetFrame(RpLight* light);
static RpMaterial* MaterialSetEnvMap(RpMaterial* material, void* data);
static RpMaterial* MaterialSetBumpMap(RpMaterial* material, void* data);
static RpMaterial* MaterialSetBumpEnvMap(RpMaterial* material, RwTexture* env, F32 env_coeff, RwTexture* bump, F32 bump_coeff);

struct fxAtomic
{
    RpAtomic* atomic;
    F32* originalUVs;
};

struct xFXBubbleParams
{
    U32 pass1 : 1;
    U32 pass2 : 1;
    U32 pass3 : 1;
    U32 padding : 5;
    U8 pass1_alpha;
    U8 pass2_alpha;
    U8 pass3_alpha;
    U32 pass1_fbmsk;
    U32 fresnel_map;
    F32 fresnel_map_coeff;
    U32 env_map;
    F32 env_map_coeff;
};

RpAtomicCallBackRender gAtomicRenderCallBack;
RpLight* MainLight;
U8 ReportUVProb = 1;
static U32 num_fx_atomics;
static RwFrame* g_matfx_frame;
#if DEBUG || RELEASE
static S32 sAuraRequests;
#endif

#if DEBUG || RELEASE
void xFXDebugSetSurfaceFlags(U32 flags)
{
    gFXSurfaceFlags = flags;
}

U32 xFXDebugGetSurfaceFlags()
{
    return gFXSurfaceFlags;
}
#endif

U32 xfx_initted;

void xFXInit()
{
    if (!xfx_initted) {
        xfx_initted = TRUE;

        g_matfx_frame = RwFrameCreate();

        RpLight* light = RpLightCreate(rpLIGHTDIRECTIONAL);
        if (light) {
            RwFrame* frame = RwFrameCreate();
            if (frame) {
                RpLightSetFrame(light, frame);
                LightResetFrame(light);
                MainLight = light;
            } else {
                RpLightDestroy(light);
            }
        }

        xFXanimUVCreate();

        xFXRingInit();

#if DEBUG || RELEASE
        xTWEAK("FX|Aura Requests", &sAuraRequests, 0, 1000, NULL, NULL, 0);
#endif
    }
}

xFXRing ringlist[MAX_RINGS];

inline RwRaster* xFXRingFindRWRaster(U32 aid_texture)
{
    if (!aid_texture)
        return NULL;
    RwTexture* txtr = (RwTexture*)xSTFindAsset(aid_texture, NULL);
    if (!txtr)
        return NULL;
    RwRaster* rast = RwTextureGetRaster(txtr);
    return rast;
}

inline void xFXRingInit()
{
    for (S32 i = 0; i < MAX_RINGS; i++) {
        xFXRing* ringdata = &ringlist[i];
        ringdata->time = -1.0f;
        ringdata->rast_ring = xFXRingFindRWRaster(ringdata->texture);
    }
}

static void DrawRing(xFXRing* m); // UNUSED

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("xFX.cpp", "m->rast_ring", "%s")
#endif

#if DEBUG
DECOMP_FORCEBLOCK((const RwIm3DVertex* verts, U32 u, S32 s) {
    *(F32*)0 = 0.0f;
    *(F32*)0 = 2.0f * PI;
    *(F32*)0 = 1.0f;
    *(F32*)0 = u;
    *(F32*)0 = s;
    xIMRenderLegacy(verts, 0, NULL, rwPRIMTYPETRILIST);
})
#else
DECOMP_FORCEBLOCK((const RwIm3DVertex* verts, U32 u, S32 s) {
    *(F32*)0 = 0.0f;
    *(F32*)0 = 1.0f;
    *(F32*)0 = 2.0f * PI;
    *(F32*)0 = u;
    *(F32*)0 = s;
    xIMRenderLegacy(verts, 0, NULL, rwPRIMTYPETRILIST);
})
#endif

xFXRing* xFXRingCreate(const xVec3* pos, const xFXRing* params)
{
    xFXRing* ring = &ringlist[0];

    if (!pos || !params) {
        return NULL;
    }

    for (S32 i = 0; i < MAX_RINGS; i++, ring++) {
        if (ring->time <= 0.0f) {
            memcpy(ring, params, sizeof(xFXRing));
            ring->rast_ring = xFXRingFindRWRaster(ring->texture);
            ring->time = 0.001f;
            ring->pos = *pos;
            ring->ring_radius_delta *= 1.0f / ring->lifetime;
            ring->ring_height_delta *= 1.0f / ring->lifetime;
            ring->ring_tilt_delta *= 1.0f / ring->lifetime;
            return ring;
        }
    }

    return NULL;
}

static void xFXRingUpdate(F32 dt)
{
    xFXRing* ring = &ringlist[0];
    if (xabs(dt) < 0.001f) {
        return;
    }

    for (S32 i = 0; i < MAX_RINGS; i++, ring++) {
        if (ring->time <= 0.0f) {
            continue;
        }

        F32 lifetime = (ring->lifetime < dt) ? dt : ring->lifetime;
        ring->time += dt;
        F32 t = ring->time / lifetime;
        if (t > 1.0f) {
            ring->time = 0.0f;
            if (ring->parent) {
                *ring->parent = NULL;
            }
            ring->parent = NULL;
        }
    }
}

static xFXBubbleParams defaultBFX[1] = { { 1, 1, 1, 0, 0, 0, 0xC0, 0xFFFFFFFF, GLOSS_EDGE_ID, 0.75f, RAINBOWFILM_SMOOTH32_ID, 0.5f } };
static xFXBubbleParams* BFX = defaultBFX;

#if DEBUG
static void xFXReadBubbleParams()
{
    BFX = defaultBFX;
}
#endif

#if DEBUG
static void xFXWriteBubbleParams(); // UNUSED
static void xFXBubbleDebug(); // UNUSED

DECOMP_FORCEACTIVE(
    "Writing bubble params to %s...\n", "bubbleparams.ini", "%s %d\n", "paramset", "%s=%d\n", "pass1", "pass2", "pass3",
    "pass1_alpha", "pass2_alpha", "pass3_alpha", "pass1_fbmsk", "%s=\"%s\"\n", "fresnel_map", "%s=%6.2f\n",
    "fresnel_map_coeff", "env_map", "env_map_coeff", "\n\n", "...done\n", "bubble paramset %d\n", "pass1 %d\n",
    "pass2 %d alpha %3d\nfresnel %4.2f %08X %s\n", "pass3 %d alpha %3d\nenvmap  %4.2f %08X %s\n"
)
#endif

xTWEAKABLE F32 shiny_factor = 0.5f;

void xFX_SceneEnter(RpWorld* world)
{
    S32 i;
    RwInt32 num = RpWorldGetNumMaterials(world);

    for (i = 0; i < num; i++) {
        const xSurface* sp = zSurfaceGetSurface(i);
        zSurfaceProps* pp = (zSurfaceProps*)sp->moprops;
        if (!pp || !pp->asset) {
            continue;
        }

        zSurfMatFX* fxp = &pp->asset->matfx;

        if (fxp->flags) {
            if (fxp->flags == 0x10) {
                fxp->flags |= 0x1;
            }

            RpMaterial* mp = RpWorldGetMaterial(world, i);
            RwTexture* mptex = RpMaterialGetTexture(mp);
            if (!mptex) {
                iprintf("ERROR:  can't apply matFX to an untextured material!\n");
                continue;
            }

            gFXSurfaceFlags = fxp->flags;

            if (fxp->flags & 0x1) {
                RwTexture* env = (RwTexture*)xSTFindAsset(fxp->envmapID, NULL);
                if (!env) {
                    iprintf("ERROR:  env map %08X not found!\n", fxp->envmapID);
                    continue;
                }
                MaterialSetEnvMap(mp, env);
                RpMatFXMaterialSetEnvMapCoefficient(mp, shiny_factor * fxp->shininess);
                iprintf("%s mapping material %08X with map %08X, coeff %6.3f\n", (fxp->flags & 0x10) ? "Specular" : "Env", mp, fxp->envmapID, fxp->shininess);
            }

            if (fxp->flags & 0x2) {
                RwTexture* bump = (RwTexture*)xSTFindAsset(fxp->bumpmapID, NULL);
                if (!bump) {
                    iprintf("ERROR:  bump map %08X not found!\n", fxp->bumpmapID);
                    continue;
                }
                MaterialSetBumpMap(mp, bump);
                RpMatFXMaterialSetBumpMapCoefficient(mp, fxp->bumpiness);
                iprintf("Bump mapping material %08X with map %08X, coeff %6.3f\n", mp, fxp->bumpmapID, fxp->bumpiness);
            }

            if (fxp->flags & 0x4) {
                RwTexture* env = (RwTexture*)xSTFindAsset(fxp->envmapID, NULL);
                RwTexture* bump = (RwTexture*)xSTFindAsset(fxp->bumpmapID, NULL);
                if (!env || !bump) {
                    if (!env)
                        iprintf("ERROR:  env map %08X not found!\n", fxp->envmapID);
                    if (!bump)
                        iprintf("ERROR:  bump map %08X not found!\n", fxp->bumpmapID);
                    continue;
                }
                MaterialSetBumpEnvMap(mp, env, fxp->shininess, bump, fxp->bumpiness);
                iprintf("%s mapping material %08X with map %08X, coeff %6.3f\n", (fxp->flags & 0x10) ? "Specular" : "Env", mp, fxp->envmapID, fxp->shininess);
                iprintf("Bump mapping material %08X with map %08X, coeff %6.3f\n", mp, fxp->bumpmapID, fxp->bumpiness);
            }
        }
    }

    xScene* sc = xglobals->sceneCur;

    xASSERT_L(724, sc);

    for (i = 0; i < sc->numActEnts; i++) {
        xEnt* ent = sc->actEnts[i];

        xASSERT_L(736, ent);

        if (!gAtomicRenderCallBack && ent->model) {
            RpAtomicCallBackRender tmp = RpAtomicGetRenderCallBack(ent->model->Data);
            RpAtomicSetRenderCallBack(ent->model->Data, NULL);
            gAtomicRenderCallBack = RpAtomicGetRenderCallBack(ent->model->Data);
            RpAtomicSetRenderCallBack(ent->model->Data, tmp);
        }
    }

    num_fx_atomics = 0;

#if DEBUG
    xFXReadBubbleParams();
#endif
}

void xFX_SceneExit(RpWorld*)
{
}

void xFXUpdate(F32 dt)
{
    xFXRingUpdate(dt);
    xFXRibbon::update_all(dt);
}

U32 xFXEnvMap(RpMaterial* mat, U32 envmapID, F32 coeff)
{
    if (!mat) {
        return 0;
    }

    RwTexture* env = (RwTexture*)xSTFindAsset(envmapID, NULL);
    if (!env) {
        iprintf("xFXEnvMap:  envmapID %08X (%s) not found!\n", envmapID, xSTAssetName(envmapID));
        return 0;
    }

    MaterialSetEnvMap(mat, env);
    RpMatFXMaterialSetEnvMapCoefficient(mat, coeff);
    return 1;
}

U32 xFXBumpMap(RpMaterial* mat, U32 bumpmapID, F32 coeff)
{
    if (!mat) {
        return 0;
    }

    RwTexture* bump = (RwTexture*)xSTFindAsset(bumpmapID, NULL);
    if (!bump) {
        iprintf("xFXBumpMap:  envmapID %08X (%s) not found!\n", bumpmapID, xSTAssetName(bumpmapID));
        return 0;
    }

    MaterialSetBumpMap(mat, bump);
    RpMatFXMaterialSetBumpMapCoefficient(mat, coeff);
    return 1;
}

static void LightResetFrame(RpLight* light)
{
    RwFrame* frame;
    RwV3d xAxis = { 1.0f, 0.0f, 0.0f };
    RwV3d yAxis = { 0.0f, 1.0f, 0.0f };

    frame = RpLightGetFrame(light);

    RwFrameRotate(frame, &xAxis, 45.0f, rwCOMBINEREPLACE);
    RwFrameRotate(frame, &yAxis, 45.0f, rwCOMBINEPOSTCONCAT);
}

static RpMaterial* WorldMaterialEnvMap(RpMaterial* material, void* data); // UNUSED
static RpWorldSector* SectorMatFXEnableCB(RpWorldSector* worldSector, void*); // UNUSED

static RpMaterial* MaterialDisableMatFX(RpMaterial* material, void*)
{
    RpMatFXMaterialSetEffects(material, rpMATFXEFFECTNULL);
    return material;
}

RpAtomic* AtomicDisableMatFX(RpAtomic* atomic)
{
    RpMatFXAtomicEnableEffects(atomic);

    RpGeometry* geometry = RpAtomicGetGeometry(atomic);
    if (geometry) {
        RpGeometryForAllMaterials(geometry, MaterialDisableMatFX, NULL);
    }

    return atomic;
}

static RpMaterial* MaterialSetShininess(RpMaterial* material, void*); // UNUSED
static RpAtomic* AtomicSetShininess(RpAtomic* atomic, void* data); // UNUSED

static RpMaterial* MaterialSetEnvMap(RpMaterial* material, void* data)
{
    if (!data) {
        iprintf("MaterialSetEnvMap texture is NULL\n");
        return NULL;
    }

    RwTexture* matTexture = RpMaterialGetTexture(material);

    if (matTexture) {
        RwChar* textureName;
        RwTexture* texture = (RwTexture*)data;
        textureName = RwTextureGetName(texture);
        RwFrame* frame;

        if (texture) {
            if (gFXSurfaceFlags & 0x10) {
                if (xglobals->screen->icam)
                    frame = RwCameraGetFrame(xglobals->screen->icam);
                else {
                    iprintf("camera frame not ready for specular map yet!\n");
                    frame = RpLightGetFrame(MainLight);
                }
            } else {
                frame = RpLightGetFrame(MainLight);
            }

            RpMatFXMaterialSetEffects(material, rpMATFXEFFECTENVMAP);
            RpMatFXMaterialSetupEnvMap(material, texture, frame, FALSE, 1.0f);
        } else {
            RpMatFXMaterialSetEffects(material, rpMATFXEFFECTNULL);
        }
    }

    return material;
}

static RpMaterial* MatUpdateFrame(RpMaterial* material, void* data); // UNUSED

DECOMP_FORCEACTIVE("spec3", "update frame\n")

DECOMP_FORCEBLOCK((xMat3x3* mat, const xVec3* from, const xVec3* to) {
    xMat3x3LookAt(mat, from, to);
})

static RpAtomic* AtomicSetEnvMap(RpAtomic* atomic, void* data); // UNUSED

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("xFXAtomicBumpEnvSetup:  failed to find bump map %08X\n", "xFXAtomicBumpEnvSetup:  failed to find env map  %08X\n")
#endif

static RpMaterial* MaterialSetBumpiness(RpMaterial* material, void*); // UNUSED
static RpAtomic* AtomicSetBumpiness(RpAtomic* atomic, void* data); // UNUSED

static RpMaterial* MaterialSetBumpMap(RpMaterial* material, void* data)
{
    if (!data) {
        iprintf("MaterialSetBumpMap texture is NULL\n");
        return NULL;
    }

    RwTexture* matTexture = RpMaterialGetTexture(material);

    if (matTexture) {
        RwTexture* texture = (RwTexture*)data;

        if (texture) {
            RwFrame* frame = RpLightGetFrame(MainLight);

            RpMatFXMaterialSetEffects(material, rpMATFXEFFECTBUMPMAP);
            RpMatFXMaterialSetupBumpMap(material, texture, frame, 1.0f);
        } else {
            RpMatFXMaterialSetEffects(material, rpMATFXEFFECTNULL);
        }
    }

    return material;
}

static RpAtomic* AtomicSetBumpMap(RpAtomic* atomic, void* data); // UNUSED

static RpMaterial* MaterialSetBumpEnvMap(RpMaterial* material, RwTexture* env, F32 env_coeff, RwTexture* bump, F32 bump_coeff)
{
    if (!env || !bump) {
        iprintf("MaterialSetEnvBumpMap error:  env %08X bump %08X\n", env, bump);
        return NULL;
    }

    RpMatFXMaterialSetEffects(material, rpMATFXEFFECTBUMPENVMAP);

    RwFrame* frame;
    if (gFXSurfaceFlags & 0x10) {
        frame = RwCameraGetFrame(xglobals->screen->icam);
    } else {
        frame = RpLightGetFrame(MainLight);
    }

    RpMatFXMaterialSetupEnvMap(material, env, frame, TRUE, env_coeff);

    frame = RpLightGetFrame(MainLight);
    RpMatFXMaterialSetupBumpMap(material, bump, frame, bump_coeff);

    return material;
}

static RpMaterial* MaterialResetEffects(RpMaterial* material, void*); // UNUSED
static RpAtomic* AtomicResetMaterialEffects(RpAtomic* atomic, void* data); // UNUSED
static RpAtomic* AtomicAnimateUVs(RpAtomic* atomic, const F32* originalUVs); // UNUSED

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("atomic", "originalUVs")
#endif

static RxPipeline* xFXanimUVPipeline;
F32 xFXanimUVRotMat0[2] = { 1.0f, 0.0f };
F32 xFXanimUVRotMat1[2] = { 0.0f, 1.0f };
F32 xFXanimUVTrans[2] = { 0.0f, 0.0f };
F32 xFXanimUVScale[2] = { 1.0f, 1.0f };

void xFXanimUVSetTranslation(const xVec3* trans)
{
    xFXanimUVTrans[0] = trans->x;
    xFXanimUVTrans[1] = trans->y;
}

void xFXanimUVSetScale(const xVec3* scale)
{
    xFXanimUVScale[0] = scale->x;
    xFXanimUVScale[1] = scale->y;
}

void xFXanimUVSetAngle(F32 angle)
{
    F32 s = isin(angle);
    F32 c = icos(angle);

    xFXanimUVRotMat0[0] = c; xFXanimUVRotMat0[1] = -s;
    xFXanimUVRotMat1[0] = s; xFXanimUVRotMat1[1] = c;
}

F32 xFXanimUV2PRotMat0[2] = { 1.0f, 0.0f };
F32 xFXanimUV2PRotMat1[2] = { 0.0f, 1.0f };
F32 xFXanimUV2PTrans[2] = { 0.0f, 0.0f };
F32 xFXanimUV2PScale[2] = { 1.0f, 1.0f };
RwTexture* xFXanimUV2PTexture;

void xFXanimUV2PSetTranslation(const xVec3* trans)
{
    xFXanimUV2PTrans[0] = trans->x;
    xFXanimUV2PTrans[1] = trans->y;
}

void xFXanimUV2PSetScale(const xVec3* scale)
{
    xFXanimUV2PScale[0] = scale->x;
    xFXanimUV2PScale[1] = scale->y;
}

void xFXanimUV2PSetAngle(F32 angle)
{
    F32 s = isin(angle);
    F32 c = icos(angle);

    xFXanimUV2PRotMat0[0] = c; xFXanimUV2PRotMat0[1] = -s;
    xFXanimUV2PRotMat1[0] = s; xFXanimUV2PRotMat1[1] = c;
}

void xFXanimUV2PSetTexture(RwTexture* texture)
{
    xFXanimUV2PTexture = texture;
}

static RwMatrix gFXUVs[2];

RpAtomic* xFXanimUVAtomicSetup(RpAtomic* atomic, bool dualPass)
{
    if (atomic) {
        RpGeometry* geom = RpAtomicGetGeometry(atomic);
        if (geom) {
            RwInt32 numMaterials = RpGeometryGetNumMaterials(geom);
            for (S32 i = 0; i < numMaterials; i++) {
                RpMaterial* material = RpGeometryGetMaterial(geom, i);
                if (material) {
                    RwMatrixSetIdentity(&gFXUVs[0]);

                    F32 rx = xFXanimUVRotMat0[0] * xFXanimUVScale[0];
                    F32 ry = xFXanimUVRotMat0[1] * xFXanimUVScale[1];
                    F32 ux = xFXanimUVRotMat1[0] * xFXanimUVScale[0];
                    F32 uy = xFXanimUVRotMat1[1] * xFXanimUVScale[1];
                    F32 px = xFXanimUVTrans[0];
                    F32 py = xFXanimUVTrans[1];

                    gFXUVs[0].right.x = rx;
                    gFXUVs[0].right.y = ry;
                    gFXUVs[0].up.x = ux;
                    gFXUVs[0].up.y = uy;
                    gFXUVs[0].pos.x = px;
                    gFXUVs[0].pos.y = py;

                    if (dualPass) {
                        if (RpMatFXMaterialGetEffects(material) != rpMATFXEFFECTDUALUVTRANSFORM) {
                            if (xglobals->fromLauncher) xFAIL_FMT_L(1633, "The model %s has dual pass UV animation info, but it is not specified with the UV2PANIM flag in pipespec. To fix the problem add MATFX=UV2PANIM to the model's pipespec.", xSTAssetNameFromPointer(atomic));
                            return NULL;
                        }

                        RwMatrixSetIdentity(&gFXUVs[1]);

                        rx = xFXanimUV2PRotMat0[0] * xFXanimUV2PScale[0];
                        ry = xFXanimUV2PRotMat0[1] * xFXanimUV2PScale[1];
                        ux = xFXanimUV2PRotMat1[0] * xFXanimUV2PScale[0];
                        uy = xFXanimUV2PRotMat1[1] * xFXanimUV2PScale[1];
                        px = xFXanimUV2PTrans[0];
                        py = xFXanimUV2PTrans[1];

                        gFXUVs[1].right.x = rx;
                        gFXUVs[1].right.y = ry;
                        gFXUVs[1].up.x = ux;
                        gFXUVs[1].up.y = uy;
                        gFXUVs[1].pos.x = px;
                        gFXUVs[1].pos.y = py;

                        RpMatFXMaterialSetupDualTexture(material, xFXanimUV2PTexture, rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);
                        RpMatFXMaterialSetUVTransformMatrices(material, &gFXUVs[0], &gFXUVs[1]);
                    } else {
                        if (RpMatFXMaterialGetEffects(material) != rpMATFXEFFECTUVTRANSFORM) {
                            char text[256];
                            sprintf(text, "The model %s has single pass UV animation info. but it is not specified with the UVANIM flag in pipespec. To fix the problem add MATFX=UVANIM to the model's pipespec.", xSTAssetNameFromPointer(atomic));
                            xFAIL_ONCE_L(1660, "%s", text);
                            return NULL;
                        }

                        RpMatFXMaterialSetEffects(material, rpMATFXEFFECTUVTRANSFORM);
                        RpMatFXMaterialSetUVTransformMatrices(material, &gFXUVs[0], NULL);
                    }
                }
            }
        }
    }

    return atomic;
}

S32 xFXanimUVCreate()
{
    if (!xFXanimUVPipeline) {
        xFXanimUVPipeline = iFXanimUVCreatePipe();
    }

    return xFXanimUVPipeline ? TRUE : FALSE;
}

namespace xFX
{
    inline void lerp(F32& v, F32 frac, F32 v0, F32 v1)
    {
        v = v0 + frac * (v1 - v0);
    }

    inline void lerp(U8& v, F32 frac, U8 v0, U8 v1)
    {
        v = (U8)(v0 + frac * ((F32)v1 - (F32)v0) + 0.5f);
    }
}

tier_queue_allocator xFXRibbon::joint_alloc;
xFXRibbon::activity_data* xFXRibbon::activities[MAX_ACTIVITIES];
S32 xFXRibbon::activities_used;
bool xFXRibbon::need_sort;

#if DEBUG || RELEASE
struct xFXRibbon::debug_info
{
    char tweak_prefix[128];
    char name[32];
    curve_node curve[MAX_CURVE_NODES];
    U32 texture;
    S32 active_joints;
    config_type cfg;
    tweaked_callback tweaked;
    void* context;
    struct
    {
        bool active;
        bool repeat;
        bool bind_to_player;
        F32 time;
        xVec3 center;
        xVec3 offset;
        F32 radius_inner;
        F32 radius_outer;
        F32 freq;
        F32 duration;
        F32 orient;
        F32 random_orient_increase;
    } spiral;
};
#endif

void xFXRibbon::create(S32 disableAutomaticDebugTweaks)
{
    texture_offset = 0.0f;
    texture_increment_scale = 1.0f;

    act = NULL;
    cfg = NULL;

#if DEBUG || RELEASE
    if (disableAutomaticDebugTweaks)
        debug = NULL;
    else
        debug_init();
#endif
}

void xFXRibbon::clear()
{
    if (!act) return;

    act->joints.clear();
    deactivate();
}

void xFXRibbon::load_default_config(config_type& cfg)
{
    cfg.life_time = 1.0f;
    cfg.blend_src = rwBLENDSRCALPHA;
    cfg.blend_dst = rwBLENDINVSRCALPHA;
    cfg.pivot = 0.5f;
    cfg.flags = 0;
    cfg.raster = get_raster("fx_streak1");

    static const curve_node default_curve[] =
    {
        { 0.0f, { 0xFF, 0xFF, 0xFF, 0xFF }, 1.0f },
        { 1.0f, { 0xFF, 0xFF, 0xFF, 0xFF }, 1.0f }
    };

    cfg.curve = default_curve;
    cfg.curve_size = 2;
}

void xFXRibbon::set_config(const config_type* cfg)
{
    xASSERT_L(1913, cfg != 0);
    xASSERT_L(1914, cfg->life_time > 0.0f);
    xASSERT_L(1915, (cfg->pivot >= 0.0f) && (cfg->pivot <= 1.0f));
    this->cfg = cfg;

    debug_refresh_config();

    if (!act) return;

    act->ilife = 1.0f / cfg->life_time;
    act->mlife = 1000.0f * cfg->life_time;
    need_sort = true;
}

void xFXRibbon::insert(const xVec3& loc, const xVec3& norm, F32 scale, F32 alpha, S32 flags)
{
    xASSERT_L(1929, valid());
    activate();
    if (!act) {
        return;
    }

    while (act->joints.front_full() && !act->joints.empty()) act->joints.pop_back();
    if (act->joints.front_full()) return;
    act->joints.push_front(eMemMgrTag_FXRibbon);
    joint_data& joint = act->joints.front();
    joint.flags = flags & 0x1;
    joint.loc = loc;
    joint.norm = norm;
    joint.born = act->mtime;
    joint.scale = scale;
    joint.alpha = alpha;
}

void xFXRibbon::activate()
{
    if (act) return;

    if (activities_used >= MAX_ACTIVITIES) {
        return;
    }

    act = activities[activities_used];
    activities_used++;

    act->owner = this;
    act->mtime = 0;
    set_config(cfg);
}

void xFXRibbon::deactivate()
{
    if (!act) {
        return;
    }

    activities_used--;

    activity_data** move_act_position = act->position;
    activity_data* move_act = *move_act_position;
    activity_data** tail_act_position = &activities[activities_used];
    activity_data* tail_act = *tail_act_position;

    *move_act_position = tail_act;
    (*move_act_position)->position = move_act_position;
    *tail_act_position = move_act;
    (*tail_act_position)->position = tail_act_position;

    act = NULL;
    need_sort = true;
}

void xFXRibbon::update(F32 dt)
{
    if (!act) {
        return;
    }

    xASSERT_L(2006, valid());
    debug_update(dt);

    act->mtime += (U32)(1000.0f * dt);

    while (!act->joints.empty()) {
        joint_data& joint = act->joints.back();
        U32 age = act->mtime - joint.born;
        if (age < act->mlife) break;
        joint.born = act->mtime - act->mlife;
        if (act->joints.size() == 1) { act->joints.clear(); break; }
        const joint_data& joint2 = *(act->joints.end() - 2);
        U32 age2 = act->mtime - joint2.born;
        if (age2 < act->mlife) break;
        act->joints.pop_back();
    }

    if (!need_update()) deactivate();
}

void xFXRibbon::render()
{
    if (!act) {
        return;
    }

    xASSERT_L(2036, valid());

    RwIm3DVertex* verts = gRenderBuffer.m_vertex;
    xFXHighDynamicRangeBrighten((cfg->flags >> 2) & 1, false);

    act->curve_index = cfg->curve_size - 2;
    S32 size = act->joints.size();
    for (S32 it = size; it > 1; ) {
        S32 subsize = it;
        subsize = (subsize > MAX_STRIP_JOINTS) ? MAX_STRIP_JOINTS : subsize;

        S32 next_it = it - subsize + 1;
        for (S32 break_it = it - 1; break_it >= next_it; ) {
            break_it--;
            if (act->joints[break_it].flags & 0x1) {
                next_it = break_it + 1;
                subsize = it - next_it;
                break;
            }
        }

        if (subsize > 1)
            render_strip(verts, act->joints.begin() + it, subsize);

        it = next_it;
    }
}

RwRaster* xFXRibbon::get_raster(const char* name)
{
    return get_raster(xStrHash(name));
}

RwRaster* xFXRibbon::get_raster(U32 id)
{
    if (!id) return NULL;
    RwTexture* texture = (RwTexture*)xSTFindAsset(id, NULL);
    return (texture == NULL) ? NULL : RwTextureGetRaster(texture);
}

static inline void set_vert(RwIm3DVertex& vert, F32 x, F32 y, F32 z, xColor color)
{
    RwIm3DVertexSetPos(&vert, x, y, z);
    RwIm3DVertexSetRGBA(&vert, color.r, color.g, color.b, color.a);
}

static inline void set_vert_uv(RwIm3DVertex& vert, F32 u, F32 v)
{
    vert.u = u;
    vert.v = v;
}

void xFXRibbon::get_normal(xVec3& norm, const xVec3& dir, F32 orient)
{
    xASSERT_L(2095, xfeq0(dir.length2() - 1.0f));
    F32 a = isin(orient), b = icos(orient);

    F32 ax = xabs(dir.x), ay = xabs(dir.y), az = xabs(dir.z);
    if (ax < ay && ax < az) {
        norm.x = -a * (dir.y * dir.y + dir.z * dir.z);
        norm.y = a * dir.y * dir.z + b * dir.z;
        norm.z = a * dir.x * dir.z - b * dir.y;
        norm *= 1.0f / xsqrt(dir.y * dir.y + dir.z * dir.z);
    }
    else if (ay < az) {
        norm.x = a * dir.x * dir.y - b * dir.z;
        norm.y = -a * (dir.x * dir.x + dir.z * dir.z);
        norm.z = a * dir.y * dir.z + b * dir.x;
        norm *= 1.0f / xsqrt(dir.x * dir.x + dir.z * dir.z);
    } else {
        norm.x = a * dir.x * dir.z + b * dir.y;
        norm.y = a * dir.y * dir.z - b * dir.x;
        norm.z = -a * (dir.x * dir.x + dir.y * dir.y);
        norm *= 1.0f / xsqrt(dir.x * dir.x + dir.y * dir.y);
    }
}

void xFXRibbon::refresh_joint(joint_data& joint, const joint_iterator& it)
{
    if (!act) return;

    xVec3* p[2];
    if (!(joint.flags & 0x20000)) return;

    joint.flags &= ~0x20000;

    if (it == act->joints.begin()) {
        p[0] = &joint.loc;
    } else {
        p[0] = &(it - 1)->loc;
    }

    if (it + 1 == act->joints.end()) {
        p[1] = &joint.loc;
    } else {
        p[1] = &(it + 1)->loc;
    }

    xVec3 offset;
    offset.Sub(*p[0], *p[1]);
    F32 mag2 = offset.length2();
    if (xfeq0(mag2)) {
        joint.norm.x = icos(joint.orient);
        joint.norm.y = isin(joint.orient);
        joint.norm.z = 0.0f;
    } else {
        xVec3 dir;
        dir.Scale(offset, 1.0f / xsqrt(mag2));
        get_normal(joint.norm, dir, joint.orient);
    }
}

void xFXRibbon::eval_joint(const joint_data& joint, xColor& color, F32& width)
{
    if (!act) {
        return;
    }

    F32 frac = act->ilife * get_age(joint);
    if (frac > 1.0f) {
        frac = 1.0f;
    }

    const curve_node* curve = cfg->curve;

    for (; act->curve_index > 0; act->curve_index--) {
        if (frac >= curve[act->curve_index].time && frac <= curve[act->curve_index + 1].time) break;
    }

    const curve_node& node0 = curve[act->curve_index], & node1 = curve[act->curve_index + 1];
    F32 ifrac = 1.0f / (node1.time - node0.time), subfrac = ifrac * (frac - node0.time);

    xFX::lerp(color.r, subfrac, node0.color.r, node1.color.r);
    xFX::lerp(color.g, subfrac, node0.color.g, node1.color.g);
    xFX::lerp(color.b, subfrac, node0.color.b, node1.color.b);

    F32 alpha = 0.0f;
    xFX::lerp(alpha, subfrac, node0.color.a, node1.color.a);
    color.a = (U8)(alpha * joint.alpha + 0.5f);

    xFX::lerp(width, subfrac, node0.scale, node1.scale);
    width *= joint.scale;
}

void xFXRibbon::render_strip(RwIm3DVertex* verts, joint_iterator first, size_t size)
{
    if (!act) {
        return;
    }

    RwIm3DVertex* v = verts;
    for (joint_iterator it = first, last = first - size; it != last; ) {
        --it;
        joint_data& joint = *it;
        refresh_joint(joint, it);

        xColor color;
        color.a = color.b = color.g = color.r = 0;
        F32 width = 0.0f;
        eval_joint(joint, color, width);

        F32 off1 = cfg->pivot * width, off2 = (cfg->pivot - 1.0f) * width;
        F32 cx = joint.loc.x, cy = joint.loc.y, cz = joint.loc.z;
        F32 nx = joint.norm.x, ny = joint.norm.y, nz = joint.norm.z;
        set_vert(*v++, cx + off1 * nx, cy + off1 * ny, cz + off1 * nz, color);
        set_vert(*v++, cx + off2 * nx, cy + off2 * ny, cz + off2 * nz, color);
    }

    RwIm3DVertex* endv = v;
    v = verts;
    F32 ulookup[2] = { 0.0f, 1.0f };
    switch (cfg->flags & 0x3) {
    case 0: {
        S32 back = first.global_index() & 1;
        for (; v != endv; back ^= 1) {
            F32 u = ulookup[back];
            set_vert_uv(*v++, u, 0.0f);
            set_vert_uv(*v++, u, 1.0f);
        }
        break;
    }
    case 1: {
        F32 du = -texture_increment_scale / size;
        F32 u = 1.0f - texture_offset;
        while (v != endv) {
            set_vert_uv(*v++, u, 0.0f);
            set_vert_uv(*v++, u, 1.0f);
            u += du;
        }
        break;
    }
    case 2: {
        S32 back = first.global_index() & 1;
        for (; v != endv; back ^= 1) {
            F32 u = ulookup[back];
            set_vert_uv(*v++, 0.0f, u);
            set_vert_uv(*v++, 1.0f, u);
        }
        break;
    }
    case 3: {
        F32 du = -texture_increment_scale / size;
        F32 u = 1.0f - texture_offset;
        while (v != endv) {
            set_vert_uv(*v++, 0.0f, u);
            set_vert_uv(*v++, 1.0f, u);
            u += du;
        }
    }
    }

    xIMRenderLegacy(verts, v - verts, NULL, rwPRIMTYPETRISTRIP);
}

void xFXRibbon::copy_parameters(xFXRibbon* ribbon) const
{
    ribbon->set_config(cfg);
    ribbon->clear();
}

S32 xFXRibbon::compare_ribbons(const void* e1, const void* e2)
{
    if (e1 == e2) return 0;
    if (!e1) return 1;
    if (!e2) return -1;
    const xFXRibbon* r1 = (*(activity_data* const*)e1)->owner; xASSERT_L(2316, r1 != 0);
    const xFXRibbon* r2 = (*(activity_data* const*)e2)->owner; xASSERT_L(2317, r2 != 0);
    if (r1->cfg->raster < r2->cfg->raster) return -1;
    if (r1->cfg->raster > r2->cfg->raster) return 1;
    if (r1->cfg->blend_src < r2->cfg->blend_src) return -1;
    if (r1->cfg->blend_src > r2->cfg->blend_src) return 1;
    if (r1->cfg->blend_dst < r2->cfg->blend_dst) return -1;
    if (r1->cfg->blend_dst > r2->cfg->blend_dst) return 1;
    return 0;
}

void xFXRibbon::sort_active_ribbons()
{
    if (!need_sort || activities_used <= 0)
        return;
    qsort(activities, activities_used, sizeof(activity_data*), compare_ribbons);
    activity_data** act = activities, ** end_act = act + activities_used;
    for (; act != end_act; act++)
        (*act)->position = act;

    need_sort = false;
}

#if DEBUG || RELEASE
namespace
{
    struct raster_pair
    {
        RwRaster* raster;
        U32 id;
    };

    struct compare_raster
    {
        S32 operator()(RwRaster* raster, const raster_pair& b) { return (S32)raster - (S32)b.raster; }
    };

    raster_pair* raster_map;
    S32 raster_map_size;
}

void xFXRibbon::debug_scene_enter()
{
    xTWEAK_REMOVE("FX|Ribbon");

    const char** texture_labels;
    const U32* texture_ids;
    raster_map_size = zSceneGetTextureTweakMap(texture_labels, texture_ids);
    raster_map = (raster_pair*)xDebugTweak_StaticAlloc(raster_map_size * sizeof(raster_pair));
    for (S32 i = 0; i < raster_map_size; i++) {
        U32 id = texture_ids[i];
        raster_map[i].id = id;
        RwTexture* texture = (id == 0) ? NULL : (RwTexture*)xSTFindAsset(texture_ids[i], NULL);
        raster_map[i].raster = (texture == NULL) ? NULL : RwTextureGetRaster(texture);
    }
    quicksort_int(raster_map, raster_map_size, sizeof(raster_pair), 0);

    static xFXRibbon dummy_ribbon;
    static config_type dummy_cfg;
    dummy_ribbon.create(0);
    load_default_config(dummy_cfg);
    dummy_ribbon.set_config(&dummy_cfg);
    dummy_ribbon.add_tweaks("Dummy", NULL, NULL, NULL);
}

void xFXRibbon::add_tweaks(const char* debug_name, const char* tweak_prefix, tweaked_callback tweaked, void* context)
{
    xASSERT_L(2389, debug != 0);

    if (!debug_name || !*debug_name)
        sprintf(debug->name, "0x%08x", this);
    else
        sprintf(debug->name, "%.31s", debug_name);

    if (!tweak_prefix)
        sprintf(debug->tweak_prefix, "FX|Ribbon|%s|", debug->name);
    else
        strcpy(debug->tweak_prefix, tweak_prefix);

    debug->tweaked = tweaked;
    debug->context = context;

    const char* prefix = debug->tweak_prefix;

    zAUTOTWEAK(prefix, "\001Active Joints", &debug->active_joints, 0, 0, NULL, NULL, 0x2);

    static tweak_callback cb_config = tweak_callback::create_change(on_change_config);
    zAUTOTWEAK(prefix, "life_time", &debug->cfg.life_time, 0.001f, 1000000000.0f, &cb_config, this, 0x2);
    zAUTOTWEAK_SELECT(prefix, "blend_src", &debug->cfg.blend_src, tweak_blend_mode_labels, tweak_blend_mode_values, ARRAY_LENGTH(tweak_blend_mode_labels), &cb_config, this, 0x2);
    zAUTOTWEAK_SELECT(prefix, "blend_dst", &debug->cfg.blend_dst, tweak_blend_mode_labels, tweak_blend_mode_values, ARRAY_LENGTH(tweak_blend_mode_labels), &cb_config, this, 0x2);
    zAUTOTWEAK(prefix, "pivot", &debug->cfg.pivot, 0.0f, 1.0f, &cb_config, this, 0x2);
    zAUTOTWEAK_FLAG(prefix, "texture_stretch", &debug->cfg.flags, 0x1, &cb_config, this, 0x2);
    zAUTOTWEAK_FLAG(prefix, "vmajor", &debug->cfg.flags, 0x2, &cb_config, this, 0x2);
    zAUTOTWEAK_FLAG(prefix, "bright", &debug->cfg.flags, 0x4, &cb_config, this, 0x2);

    static tweak_callback cb_change_texture = tweak_callback::create_change(on_change_texture);
    zAUTOTWEAK_SELECT(prefix, "texture", &debug->texture, all_texture_labels, all_texture_ids, all_texture_count, &cb_change_texture, this, 0x2);

    static tweak_callback cb_curve = tweak_callback::create_change(on_change_curve);
    zAUTOTWEAK(prefix, "Curv\001e Size", &debug->cfg.curve_size, (U32)2, (U32)MAX_CURVE_NODES, &cb_curve, this, 0x2);

    memcpy(debug->curve, cfg->curve, cfg->curve_size * sizeof(curve_node));
    debug->cfg.curve = debug->curve;
    update_curve_tweaks();

    add_spiral_tweaks();
}

void xFXRibbon::debug_init()
{
    debug = (debug_info*)xDebugTweak_StaticAlloc(sizeof(debug_info));

    memset(debug, 0, sizeof(debug_info));

    load_default_config(debug->cfg);
    cfg = &debug->cfg;

    debug->spiral.active = debug->spiral.repeat = false;
    debug->spiral.bind_to_player = true;
    debug->spiral.center.assign(0.0f, 0.0f, 0.0f);
    debug->spiral.offset.assign(0.0f, 0.0f, 0.0f);
    debug->spiral.radius_inner = 0.0f;
    debug->spiral.radius_outer = 1.0f;
    debug->spiral.freq = 5.0f;
    debug->spiral.duration = 2.0f;
    debug->spiral.orient = 0.0001f;
    debug->spiral.random_orient_increase = 0.0f;
}

void xFXRibbon::debug_update(F32 dt)
{
    if (!debug_tweakhelp_enabled()) {
        return;
    }

    debug->active_joints = (act == NULL) ? 0 : act->joints.size();

    debug->spiral.time += dt;
    if (debug->spiral.time >= debug->spiral.duration) debug->spiral.active = false;

    if (!debug->spiral.active && !debug->spiral.repeat) return;

    S32 insert_flags = 0;
    if (!debug->spiral.active) {
        debug->spiral.active = true;
        debug->spiral.time = xfmod(debug->spiral.time, debug->spiral.duration);
    }

    if (debug->spiral.bind_to_player) move_spiral_to_player();

    F32 frac = debug->spiral.time / debug->spiral.duration;
    F32 ang = 2.0f * PI * (frac * debug->spiral.freq);
    F32 radius = debug->spiral.radius_inner + (debug->spiral.radius_outer - debug->spiral.radius_inner) * isin(PI * frac);
    xVec3 norm = { isin(ang), 0.0f, icos(ang) }, loc;
    loc.AddScale(debug->spiral.center, norm, radius);

    if (0.0f != debug->spiral.random_orient_increase) {
        debug->spiral.orient += debug->spiral.random_orient_increase * dt * xurand();
    }

    F32 old_orient = (PI / 180.0f) * debug->spiral.orient;
    F32 orient = xrmod(old_orient);
    F32 so = isin(orient), co = icos(orient);
    xVec3 onorm = { norm.x * co, so, norm.z * co };
    insert(loc, onorm, 1.0f, 1.0f, insert_flags);
    if (!xeq(old_orient / orient, 1.0f, 0.00001f)) debug->spiral.orient = (180.0f / PI) * orient;
}

void xFXRibbon::debug_refresh_config()
{
    if (!debug) {
        return;
    }

    debug->cfg = *cfg;
    cfg = &debug->cfg;

    raster_pair* raster_map_end = raster_map + raster_map_size;
    raster_pair* found = binary_find(raster_map, raster_map_end, cfg->raster, compare_raster());
    if (found != raster_map_end)
        debug->texture = found->id;
    else
        debug->texture = 0;

    debug_update_curve();
}

void xFXRibbon::debug_update_curve()
{
    if (!debug_tweakhelp_enabled()) {
        return;
    }

    if (cfg->curve_size < 2) create_test_curve(2);
    if (!cfg->curve || cfg->curve == debug->curve) {
        debug->cfg.curve = debug->curve;
        return;
    }

    if (debug->cfg.curve_size > MAX_CURVE_NODES)
        debug->cfg.curve_size = MAX_CURVE_NODES;
    memcpy(debug->curve, cfg->curve, cfg->curve_size * sizeof(curve_node));
    debug->cfg.curve = debug->curve;
    update_curve_tweaks();
}

void xFXRibbon::add_spiral_tweaks()
{
    char prefix[128];
    sprintf(prefix, "%s\002Test Spiral|", debug->tweak_prefix);

    static tweak_callback cb_spiral = tweak_callback::create_change(on_start_spiral);
    zAUTOTWEAK_MESSAGE(prefix, "\001Start", "Blah", &cb_spiral, this, 0x2);

    static tweak_callback cb_activate = tweak_callback::create_change(on_activate_spiral);
    zAUTOTWEAK(prefix, "\002repeat", &debug->spiral.repeat, 0, 0, &cb_activate, this, 0x2);
    zAUTOTWEAK(prefix, "\002bind_to_player", &debug->spiral.bind_to_player, 0, 0, NULL, NULL, 0x2);
    zAUTOTWEAK(prefix, "center ", &debug->spiral.center, 0, 0, NULL, NULL, 0x2);
    zAUTOTWEAK(prefix, "offset ", &debug->spiral.offset, 0, 0, NULL, NULL, 0x2);
    zAUTOTWEAK(prefix, "radius_inner", &debug->spiral.radius_inner, 0.0f, 100000.0f, NULL, NULL, 0x2);
    zAUTOTWEAK(prefix, "radius_outer", &debug->spiral.radius_outer, 0.0f, 100000.0f, NULL, NULL, 0x2);
    zAUTOTWEAK(prefix, "freq", &debug->spiral.freq, 0.0f, 1000000000.0f, NULL, NULL, 0x2);
    zAUTOTWEAK(prefix, "duration", &debug->spiral.duration, 0.0f, 1000000000.0f, NULL, NULL, 0x2);
    zAUTOTWEAK(prefix, "orient", &debug->spiral.orient, -1000000000.0f, 1000000000.0f, NULL, NULL, 0x2);
    zAUTOTWEAK(prefix, "random_orient_increase", &debug->spiral.random_orient_increase, -1000000000.0f, 1000000000.0f, NULL, NULL, 0x2);
}

void xFXRibbon::update_curve_tweaks()
{
    if (!debug_tweakhelp_enabled()) {
        return;
    }

    char prefix[128];
    xASSERT_L(2560, strlen(debug->tweak_prefix) + strlen("Curve|") < 128);
    sprintf(prefix, "%sCurve|", debug->tweak_prefix);
    char* tail = prefix + strlen(prefix);
    curve_node* curve = debug->curve;

    for (S32 i = 0; i < MAX_CURVE_NODES; i++) {
        sprintf(tail, "%02d", i);
        if (i < cfg->curve_size) {
            if (xDebugGetTweak(prefix)) continue;

            strcat(tail, "|");

            static tweak_callback cb_node_time = tweak_callback::create_change(on_change_node_time);
            static tweak_callback cb_node_value = tweak_callback::create_change(on_change_node_value);
            zAUTOTWEAK(prefix, "time", &curve[i].time, 0.0f, 1.0f, &cb_node_time, this, 0x2);
            zAUTOTWEAK(prefix, "color ", &curve[i].color, 0, 0, &cb_node_value, this, 0x2);
            zAUTOTWEAK(prefix, "scale", &curve[i].scale, 0.0f, 100000.0f, &cb_node_value, this, 0x2);
        } else
            xTWEAK_REMOVE(prefix);
    }
}

bool xFXRibbon::debug_need_update() const
{
    return debug && (debug->spiral.active || debug->spiral.repeat);
}

bool xFXRibbon::debug_tweakhelp_enabled() const { return debug != NULL; }

void xFXRibbon::create_test_curve(S32 size)
{
    if (size < 2) size = 2;
    else if (size > MAX_CURVE_NODES) size = MAX_CURVE_NODES;

    debug->cfg.curve_size = size;
    curve_node* curve = debug->curve;
    debug->cfg.curve = curve;

    for (S32 i = 0; i < size; i++) {
        curve[i].time = (F32)i / (size - 1);
        if (curve[i].scale < 0.001f || curve[i].scale > 10.0f) {
            curve[i].color = g_WHITE;
            curve[i].scale = 0.1f;
        }
    }

    update_curve_tweaks();
}

void xFXRibbon::move_spiral_to_player()
{
    const xMat4x3& mat = *(xMat4x3*)xglobals->players[0]->model->Mat;
    debug->spiral.center.Add(mat.pos, debug->spiral.offset);
}

void xFXRibbon::on_start_spiral(const tweak_info& info)
{
    xFXRibbon& owner = *(xFXRibbon*)info.context;
    if (owner.cfg->curve_size < 2) owner.create_test_curve(2);
    owner.debug->spiral.active = true;
    owner.debug->spiral.time = 0.0f;
    owner.activate();
}

void xFXRibbon::on_activate_spiral(const tweak_info& info)
{
    xFXRibbon& owner = *(xFXRibbon*)info.context;
    if (owner.need_update()) owner.activate();
    else owner.deactivate();
}

void xFXRibbon::on_change_curve(const tweak_info& info)
{
    xFXRibbon& owner = *(xFXRibbon*)info.context;
    owner.create_test_curve(owner.cfg->curve_size);
    if (owner.debug->tweaked)
        owner.debug->tweaked(owner, owner.debug->context);
}

void xFXRibbon::on_change_config(const tweak_info& info)
{
    xFXRibbon& owner = *(xFXRibbon*)info.context;
    owner.set_config(owner.cfg);
    if (owner.debug->tweaked)
        owner.debug->tweaked(owner, owner.debug->context);
}

void xFXRibbon::on_change_node_time(const tweak_info& info)
{
    xFXRibbon& owner = *(xFXRibbon*)info.context;
    char* found = strstr(info.name.text, "Curve|");
    if (!found) return;

    S32 i = atoi(found + strlen("Curve|"));
    if (i >= owner.cfg->curve_size) return;

    curve_node* curve = owner.debug->curve;
    S32 curve_size = owner.cfg->curve_size;
    if (i > 0 && curve[i].time < curve[i - 1].time + 0.001f)
        curve[i].time = curve[i - 1].time + 0.001f;
    if (i + 1 < curve_size && curve[i].time > curve[i + 1].time - 0.001f)
        curve[i].time = curve[i + 1].time - 0.001f;
    if (owner.debug->tweaked)
        owner.debug->tweaked(owner, owner.debug->context);
}

void xFXRibbon::on_change_node_value(const tweak_info& info)
{
    xFXRibbon& owner = *(xFXRibbon*)info.context;
    if (owner.debug->tweaked)
        owner.debug->tweaked(owner, owner.debug->context);
}

void xFXRibbon::on_change_texture(const tweak_info& info)
{
    xFXRibbon& owner = *(xFXRibbon*)info.context;
    owner.debug->cfg.raster = get_raster(owner.debug->texture);
    if (owner.debug->tweaked)
        owner.debug->tweaked(owner, owner.debug->context);
}
#endif

void xFXRibbon::scene_enter()
{
    joint_alloc.init(sizeof(joint_data), 32, 32, eMemMgrTag_FXRibbon);

    activities_used = 0;

    activity_data** act = activities, ** end_act = act + MAX_ACTIVITIES;
    for (; act != end_act; act++) {
        *act = new (eMemStaticType_0, eMemMgrTag_FXRibbon, 0) activity_data;
        (*act)->position = act;
        (*act)->joints.init(joint_alloc);
    }

    debug_scene_enter();
}

void xFXRibbon::update_all(F32 dt)
{
    activity_data** act = activities, ** end_act = act + activities_used;
    for (; act != end_act; act++) {
        xFXRibbon* ribbon = (*act)->owner;
        ribbon->update(dt);
    }
}

void xFXRibbon::render_all()
{
    if (activities_used <= 0) return;

    sort_active_ribbons();

    zRenderState(eRenderStateRibbon);

    RwRaster* raster = (RwRaster*)-1;
    U32 blend_src = rwBLENDSRCALPHA, blend_dst = rwBLENDINVSRCALPHA;
    activity_data** act = activities, ** end_act = act + activities_used;
    for (; act != end_act; act++) {
        xFXRibbon* ribbon = (*act)->owner;
        if (ribbon->cfg->blend_dst != blend_dst || ribbon->cfg->blend_src != blend_src) {
            blend_src = ribbon->cfg->blend_src;
            blend_dst = ribbon->cfg->blend_dst;
            xRenderStateBlendModesSet((RwBlendFunction)blend_src, (RwBlendFunction)blend_dst);
        }

        if (ribbon->cfg->raster != raster) {
            raster = ribbon->cfg->raster;
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
        }

        ribbon->render();
    }

    if (blend_src != rwBLENDSRCALPHA || blend_dst != rwBLENDINVSRCALPHA) {
        xRenderStateBlendModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);
    }

    xFXHighDynamicRangeBrighten(false, false);
}
