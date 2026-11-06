#include "xModelBucket.h"

#include "xModel.h"
#include "xLight.h"
#include "xGlobals.h"
#include "xCam.h"
#include "xDebug.h"
#include "xDebugTweak.h"
#include "xMemMgr.h"
#include "xMath.h"
#include "xstransvc.h"
#include "xString.h"
#include "xScene.h"
#include "xEnt.h"
#include "xRenderState.h"
#include "xFXHighDynamicRange.h"
#include "xutil.h"
#include "iModel.h"
#include "iCamera.h"
#include "iDraw.h"
#include "iAsync.h"

#include <rpmatfx.h>
#include <rpskin.h>
#include <stdio.h>
#include <stdlib.h>

struct xModelAlphaBucket
{
    RpAtomic* Data;
    xModelInstance* MInst;
    F32 AlphaFade;
    F32 SortValue;
    U32 Layer;
};

struct xModelBadLayer
{
    U32 modelID;
    U32 asserted;
    S32 curLayer;
    S32 modelLayer;
};

static void xModelBucket_SetBlendAndZModes(S32 flags);
static void xModelBucket_ResetBlendAndZModes(S32 flags);

static RpWorld* sBucketDummyWorld;
static RwCamera* sBucketDummyCamera;
static S32 sBucketCurr;
static S32 sBucketCount;
static xModelBucket* sBucketList;
static S32 sBucketClipCullCurr;
static S32 sBucketClipCullCount;
static xModelBucket** sBucketClipCullList;
static S32 sAlphaCurr;
static S32 sAlphaStart;
static S32 sAlphaCount;
static xModelAlphaBucket* sAlphaList;

#if DEBUG || RELEASE
bool sRenderModels = true;
bool sRenderAlpha = true;
bool sRenderClones = true;
#endif

static S32 sBucketOpaqueRenderStart;

namespace {
    class CmpAlphaBucket
    {
    public:
        S32 operator()(const void* _a, const void* _b) const
        {
            const xModelAlphaBucket* a = (const xModelAlphaBucket*)_a;
            const xModelAlphaBucket* b = (const xModelAlphaBucket*)_b;

            if (a->Layer != b->Layer) {
                return a->Layer - b->Layer;
            }

            if (a->SortValue < b->SortValue) {
                return 1;
            }
            if (a->SortValue > b->SortValue) {
                return -1;
            }
            return 0;
        }
    };

    S32 compare_opaque_buckets(const void* e1, const void* e2)
    {
        const xModelBucket& b1 = *(const xModelBucket*)e1, &b2 = *(const xModelBucket*)e2;
        S32 use_layer1 = b1.Pipe.Flags & eALPHASORTDISABLE;
        S32 use_layer2 = b2.Pipe.Flags & eALPHASORTDISABLE;
        if (use_layer1 != use_layer2)
            return use_layer1 - use_layer2;
        if (b1.Pipe.Layer != b2.Pipe.Layer)
            return b1.Pipe.Layer - b2.Pipe.Layer;
        if (b1.Pipe.Flags != b2.Pipe.Flags)
            return b1.Pipe.Flags - b2.Pipe.Flags;
        if (b1.Pipe.AlphaDiscard != b2.Pipe.AlphaDiscard)
            return b1.Pipe.AlphaDiscard - b2.Pipe.AlphaDiscard;
        if (b1.OriginalData != b2.OriginalData) {
            return (U32)b1.OriginalData - (U32)b2.OriginalData;
        }

        return b2.ClipFlags - b1.ClipFlags;
    }
}

#if DEBUG || RELEASE
inline S32 DBG_SkipCloneHelperComment(U32) { static U32 aid_skipComment[] = { 0 }; return 1; }

U32 xModelBucket_HackPipeFlags(RpAtomic* data, U32 pipeFlags, U32 aid)
{
    if ((pipeFlags & ePIPELINE_MASK) == ePIPELINE_CLONE) {
        if (RpMatFXAtomicQueryEffects(data)) {
            if (xglobals->fromLauncher) xASSERT_FMT_L(191, 0, "ART BUG:  Model 0x%08x\n\"%s\" set to CLONE, but has env map!\nFix setting in pipespec.", aid, xSTAssetName(aid));
            pipeFlags = (pipeFlags & ~ePIPELINE_MASK) | ePIPELINE_BOTH;
        }

        if (RpSkinGeometryGetSkin(data->geometry)) {
            if (xglobals->fromLauncher) xASSERT_FMT_L(196, 0, "ART BUG:  Model 0x%08x\n\"%s\" set to CLONE, but has skinning!\nFix setting in pipespec.", aid, xSTAssetName(aid));
            pipeFlags = (pipeFlags & ~ePIPELINE_MASK) | ePIPELINE_BOTH;
        }
    }
    else if (!(pipeFlags & eMATFX_MASK) &&
               (pipeFlags & eLIGHT_MASK) != ePIPELIGHT_KITPRELIGHT &&
               !RpMatFXAtomicQueryEffects(data) &&
               !RpSkinGeometryGetSkin(data->geometry)) {
        S32 rc = DBG_SkipCloneHelperComment(aid);
        if (!rc) {
            printf("CLONABLE?   Model 0x%08X  \"%s\"\n", aid, xSTAssetName(aid));
        }
    }

    return pipeFlags;
}
#endif

void xModelBucket_PreCountReset()
{
    sBucketCurr = 0;
    sBucketCount = 0;
    sBucketList = NULL;
    sBucketClipCullCurr = 0;
    sBucketClipCullCount = 0;
    sBucketClipCullList = NULL;
    sAlphaCurr = 0;
    sAlphaCount = 0;
    sAlphaList = NULL;
    sAlphaStart = 0;
}

void xModelBucket_PreCountBucket(RpAtomic* data, xModelPipe& pipe, U32 subObjects)
{
    U32 pipeFlags = (pipe.Flags & ~ePIPELINE_MASK) | ePIPELINE_CLIP;
    S32 numbuckets;

    switch (pipeFlags & ePIPELINE_MASK) {
    case ePIPELINE_CULL:
        numbuckets = 1;
        break;
    case ePIPELINE_CLIP:
        numbuckets = 1;
        break;
    case ePIPELINE_BOTH:
    case ePIPELINE_CLONE:
        numbuckets = 2;
        break;
    case ePIPELINE_DEFAULT:
    default:
        if (iModelNumBones(data)) {
            numbuckets = 1;
        } else {
            numbuckets = 2;
        }
    }

    S32 modelcount = 0;
    do {
        if (subObjects & 1) {
            modelcount++;
        }

        subObjects >>= 1;
        data = iModelFile_RWMultiAtomic(data);
    } while (data);

    sBucketCount += modelcount * numbuckets;
    sBucketClipCullCount += modelcount * 2;
}

void xModelBucket_PreCountAlloc(S32 maxAlphaModels)
{
    sAlphaCount = maxAlphaModels;
    sBucketList = (xModelBucket*)xMEMALLOC_L(sBucketCount * sizeof(xModelBucket), 0, eMemMgrTag_Model, 0, 291);
    sBucketClipCullList = (xModelBucket**)xMEMALLOC_L(sBucketClipCullCount * sizeof(xModelBucket*), 0, eMemMgrTag_Model, 0, 292);
    sAlphaList = (xModelAlphaBucket*)xMEMALLOC_L(sAlphaCount * sizeof(xModelAlphaBucket), 0, eMemMgrTag_Model, 0, 293);

    xASSERT_L(296, sBucketDummyWorld == 0);
    xASSERT_L(297, sBucketDummyCamera == 0);

    RwBBox bbox;
    bbox.sup.x = 100000.0f;
    bbox.sup.y = 100000.0f;
    bbox.sup.z = 100000.0f;
    bbox.inf.x = -100000.0f;
    bbox.inf.y = -100000.0f;
    bbox.inf.z = -100000.0f;
    iAsyncLockRW();
    sBucketDummyWorld = RpWorldCreate(&bbox);
    iAsyncUnlockRW();

    sBucketDummyCamera = iCameraCreate(0, 0, 0);

    iAsyncLockRW();
    RpWorldAddCamera(sBucketDummyWorld, sBucketDummyCamera);
    iAsyncUnlockRW();
}

static RpMaterial* s_pMaterial;

static RpMaterial* GrabMaterial(RpMaterial* material, void*)
{
    if (RpMatFXMaterialGetEffects(material) & rpMATFXEFFECTENVMAP)
        s_pMaterial = material;
    return material;
}

static RpAtomic* GimmeAtomicCallback(RpAtomic* atomic, void* tmp)
{
    *(RpAtomic**)tmp = atomic;
    return atomic;
}

void FullAtomicDupe(RpAtomic* atomic, S32 count, RpAtomic** output)
{
    S32 i;
    RwStream* stream;
    RwMemory rwmemA = { NULL, 0 };
    RpAtomic* tempAtom;

#if DEBUG
    U32 origFlags = atomic->geometry->flags;
    bool hasEffects = false;
    F32 origShineValue = 0.0f;

    if (RpMatFXAtomicQueryEffects(atomic)) {
        hasEffects = true;
        s_pMaterial = NULL;
        RpGeometryForAllMaterials(atomic->geometry, GrabMaterial, NULL);
        if (s_pMaterial) {
            origShineValue = RpMatFXMaterialGetEnvMapCoefficient(s_pMaterial);
        }
    }
#endif

    xASSERT_L(370, atomic->geometry->triangles);

    iAsyncLockRW();
    stream = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMWRITE, &rwmemA);
    xASSERT_L(375, stream);
    if (stream) {
        RpClump* clump = RpAtomicGetClump(atomic);
        xASSERT_L(382, clump);
        RwLinkList backupClump_atomicList = clump->atomicList;
        RwLLLink backupAtomic_inClumpLink = atomic->inClumpLink;
        clump->atomicList.link.next = &atomic->inClumpLink;
        clump->atomicList.link.prev = &atomic->inClumpLink;
        atomic->inClumpLink.next = &clump->atomicList.link;
        atomic->inClumpLink.prev = &clump->atomicList.link;
        RpClumpStreamWrite(clump, stream);
        clump->atomicList = backupClump_atomicList;
        atomic->inClumpLink = backupAtomic_inClumpLink;
        RwStreamClose(stream, &rwmemA);
    }
    iAsyncUnlockRW();

    for (i = 0; i < count; i++) {
        iAsyncLockRW();
        stream = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &rwmemA);
        xASSERT_L(407, stream);
        RwBool result = RwStreamFindChunk(stream, rwID_CLUMP, NULL, NULL);
        xASSERT_L(412, result);
        RpClump* tempClump = RpClumpStreamRead(stream);
        xASSERT_FMT_L(415, tempClump, "tempClump is NULL\nRan out of memory for model data...  Crash is likely.\nNeed to remove models, compress anims\nor add textures to texspec.\n");
        RwStreamClose(stream, &rwmemA);
        RpClumpForAllAtomics(tempClump, GimmeAtomicCallback, &tempAtom);
        xASSERT_L(419, tempAtom);
        iAsyncUnlockRW();

        iAsyncLockRW();
        RwMemory rwmemB = { NULL, 0 };
        stream = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMWRITE, &rwmemB);
        xASSERT_L(473, stream);
        if (stream) {
            RpClumpStreamWrite(tempClump, stream);
            RwStreamClose(stream, &rwmemB);
        }
        stream = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &rwmemB);
        xASSERT_L(483, stream);
        if (stream) {
            result = RwStreamFindChunk(stream, rwID_CLUMP, NULL, NULL);
            xASSERT_L(500, result);
            RpClump* outClump = RpClumpStreamRead(stream);
            RpClumpForAllAtomics(outClump, GimmeAtomicCallback, &output[i]);
            RwStreamClose(stream, &rwmemB);
        } else {
            output[i] = NULL;
        }
        RwFree(rwmemB.start);
        _rwFrameSyncDirty();
        iModelUnload(tempAtom);
        xASSERT_FMT_L(537, output[i], "output[%d] is NULL\nRan out of memory for model data...  Crash is likely.\nNeed to remove models, compress anims\nor add textures to texspec.\n", i, i + 1, count);
        if (output[i]) {
            RpHAnimHierarchy* pHier = iModelGetHierarchy(output[i]);
            RpSkin* pSkin = RpSkinGeometryGetSkin(output[i]->geometry);
            if (pHier && pSkin) {
                RpSkinAtomicSetHAnimHierarchy(output[i], pHier);
            }
        }
        iAsyncUnlockRW();
    }

    iAsyncLockRW();
    RwFree(rwmemA.start);

#if DEBUG
    for (i = 0; i < count; i++) {
        xASSERT_L(575, hasEffects == (RpMatFXAtomicQueryEffects(output[i]) != 0));
        xASSERT_L(576, origFlags == (output[i]->geometry->flags & rpGEOMETRYFLAGSMASK));
        s_pMaterial = NULL;
        RpGeometryForAllMaterials(output[i]->geometry, GrabMaterial, NULL);
        if (hasEffects && s_pMaterial)
            xASSERT_L(586, origShineValue == RpMatFXMaterialGetEnvMapCoefficient(s_pMaterial));
    }
#endif

    iAsyncUnlockRW();
}

void xModelBucket_SetMaterialFX(RpAtomic* model, xModelPipe& pipe)
{
    if (pipe.Flags & eMATFX_MASK) {
        while (model) {
            iAsyncLockRW();

            RpMatFXAtomicEnableEffects(model);
            RpGeometry* geom = RpAtomicGetGeometry(model);

            bool skinned = RpSkinGeometryGetSkin(geom) != NULL;
            if (skinned) {
                RpSkinAtomicSetType(model, rpSKINTYPEMATFX);
            }

            S32 numMaterials = RpGeometryGetNumMaterials(geom);
            for (S32 i = 0; i < numMaterials; i++) {
                RpMaterial* material = RpGeometryGetMaterial(geom, i);

                if (pipe.Flags & ePIPEMATFX_UV2PANIM) {
                    RpMatFXMaterialSetEffects(material, rpMATFXEFFECTDUALUVTRANSFORM);
                } else {
                    RpMatFXMaterialSetEffects(material, rpMATFXEFFECTUVTRANSFORM);
                }
            }
            iAsyncUnlockRW();

            model = iModelFile_RWMultiAtomic(model);
        }
    }
}

void xModelBucket_InsertBucket(RpAtomic* data, xModelPipe& pipe, U32 subObjects)
{
    xModelBucket_SetMaterialFX(data, pipe);

    U32 pipeFlags = (pipe.Flags & ~ePIPELINE_MASK) | ePIPELINE_CLIP;
    S32 numbuckets;
    S32 pipeSetting[2];

    switch (pipeFlags & ePIPELINE_MASK) {
    case ePIPELINE_CULL:
        numbuckets = 1;
        pipeSetting[0] = 1;
        break;
    case ePIPELINE_CLIP:
        numbuckets = 1;
        pipeSetting[0] = 2;
        break;
    case ePIPELINE_BOTH:
    case ePIPELINE_CLONE:
        numbuckets = 2;
        pipeSetting[0] = 2;
        pipeSetting[1] = 1;
        break;
    case ePIPELINE_DEFAULT:
    default:
        if (iModelNumBones(data)) {
            numbuckets = 1;
            pipeSetting[0] = 1;
        } else {
            numbuckets = 2;
            pipeSetting[0] = 2;
            pipeSetting[1] = 1;
        }
    }

    RpAtomic* extraData = NULL;

    do {
        if (subObjects & 1) {
            xASSERT_L(775, sBucketCurr + numbuckets <= sBucketCount);
            xASSERT_L(776, sBucketClipCullCurr + 2 <= sBucketClipCullCount);

            sBucketClipCullList[sBucketClipCullCurr] = &sBucketList[sBucketCurr];
            sBucketClipCullList[sBucketClipCullCurr + 1] = &sBucketList[sBucketCurr];
            sBucketList[sBucketCurr].Data = data;
            sBucketList[sBucketCurr].OriginalData = data;
            sBucketList[sBucketCurr].BackRef = &sBucketClipCullList[sBucketClipCullCurr + 1];
            sBucketList[sBucketCurr].ClipFlags = pipeSetting[0];
            sBucketList[sBucketCurr].Pipe = pipe;
            if (numbuckets == 2) {
                FullAtomicDupe(data, 1, &extraData);
                sBucketList[sBucketCurr + 1].Data = extraData;
                sBucketList[sBucketCurr + 1].OriginalData = data;
                sBucketList[sBucketCurr + 1].BackRef = &sBucketClipCullList[sBucketClipCullCurr];
                sBucketList[sBucketCurr + 1].ClipFlags = pipeSetting[1];
                sBucketList[sBucketCurr + 1].Pipe = pipe;
                sBucketClipCullList[sBucketClipCullCurr] = &sBucketList[sBucketCurr + 1];
                extraData = NULL;
            }

            sBucketCurr += numbuckets;
            sBucketClipCullCurr += 2;
        }

        subObjects >>= 1;
        data = iModelFile_RWMultiAtomic(data);
    } while (data);
}

void xModelBucket_Init()
{
    xASSERT_L(815, sBucketCurr == sBucketCount);
    xASSERT_L(816, sBucketClipCullCurr == sBucketClipCullCount);

    if (sBucketDummyCamera) {
        iAsyncLockRW();
        RpWorldRemoveCamera(sBucketDummyWorld, sBucketDummyCamera);
        iAsyncUnlockRW();
        iCameraDestroy(sBucketDummyCamera);
        iAsyncLockRW();
        RpWorldDestroy(sBucketDummyWorld);
        iAsyncUnlockRW();
        sBucketDummyCamera = NULL;
        sBucketDummyWorld = NULL;
    }

    qsort(sBucketList, sBucketCount, sizeof(xModelBucket), compare_opaque_buckets);

    for (S32 i = 0; i < sBucketCount; i++) {
        xModelBucket* bucket = &sBucketList[i];
        xModelBucket** backref = bucket->BackRef;

        if (backref > sBucketClipCullList && backref[0] == backref[-1]) {
            backref[-1] = bucket;
        }

        backref[0] = bucket;

        bucket->List = NULL;
    }

    xTWEAK_BOOL("Debugging Options|Performance|Render Models", &sRenderModels, NULL, NULL, 0);
    xTWEAK_BOOL("Debugging Options|Performance|Render Alpha", &sRenderAlpha, NULL, NULL, 0);
    xTWEAK_BOOL("Debugging Options|Performance|Render Clones", &sRenderClones, NULL, NULL, 0);
}

xModelBucket** xModelBucket_GetBuckets(RpAtomic* data)
{
    for (S32 i = 0; i < sBucketClipCullCount; i += 2) {
        if (sBucketClipCullList[i]->OriginalData == data) {
            return &sBucketClipCullList[i];
        }
    }
    return NULL;
}

void xModelBucket_Begin()
{
    for (S32 i = 0; i < sBucketCount; i++) {
        sBucketList[i].List = NULL;
    }

    xModelBucketEnabled = TRUE;
}

#if DEBUG || RELEASE
RpAtomic* gxModelBucketProfileModel;
U32 gxModelBucketProfileCount;
#endif

void xModelBucket_Add(xModelInstance* minst, bool)
{
    if ((minst->Flags & 0x401) != 0x1) {
        return;
    }

#if DEBUG || RELEASE
    if (gxModelBucketProfileModel == minst->Data) {
        gxModelBucketProfileCount++;
        return;
    }

    if (!sRenderModels) {
        return;
    }

    if (!(RpAtomicGetFlags(minst->Data) & rpATOMICRENDER)) {
        return;
    }
#endif

    RpAtomic* model = minst->Data;

#if DEBUG || RELEASE
    if (gScreenDump != exSD_MASSIVE)
#endif
    {
        if (xModelCullSingle(minst)) {
            return;
        }
    }

    xMat4x3* cammat = &xglobals->cam->mat;
    F32 camdist2 = xsqr(model->worldBoundingSphere.center.x - cammat->pos.x) +
                   xsqr(model->worldBoundingSphere.center.y - cammat->pos.y) +
                   xsqr(model->worldBoundingSphere.center.z - cammat->pos.z);

    if (camdist2 >= xsqr(minst->FadeEnd)) {
        return;
    }

    xModelBucket* bucket;
    F32 camdot = cammat->at.x * (model->worldBoundingSphere.center.x - cammat->pos.x) +
                 cammat->at.y * (model->worldBoundingSphere.center.y - cammat->pos.y) +
                 cammat->at.z * (model->worldBoundingSphere.center.z - cammat->pos.z);

    if (camdot < 1.5f * model->worldBoundingSphere.radius) {
        bucket = minst->Bucket[1];
    } else {
        bucket = minst->Bucket[0];
    }

    F32 alphaFade = 1.0f;
    if (camdist2 > xsqr(minst->FadeStart)) {
        alphaFade = (minst->FadeEnd - xsqrt(camdist2)) / (minst->FadeEnd - minst->FadeStart);
        if (alphaFade <= 0.0f) {
            return;
        }

        alphaFade = MIN(alphaFade, 1.0f);
    }

    bool alphaFlags = (minst->Pipe.Flags & (eSRCBLEND_MASK | eDESTBLEND_MASK)) != 0;
    bool alphaLOD = 1.0f != alphaFade;
    bool alphaModel = 1.0f != minst->Alpha;

    if (minst->Pipe.Flags & eALPHASORTDISABLE) {
        alphaFlags = false;
        alphaModel = false;
    }
    else if ((minst->Pipe.Flags & eFORCEOPAQUE_MASK) == ePIPEFORCEOPAQUE_BLEND)
        alphaFlags = false;
    else if ((minst->Pipe.Flags & eFORCEOPAQUE_MASK) == ePIPEFORCEOPAQUE_ALWAYS)
        alphaFlags = alphaLOD = alphaModel = false;

    bool alphaProg = alphaLOD || alphaModel;

    if (alphaFlags || alphaProg) {
        if (sAlphaCurr < sAlphaCount) {
            sAlphaList[sAlphaCurr].Data = bucket->Data;
            sAlphaList[sAlphaCurr].MInst = minst;
            sAlphaList[sAlphaCurr].AlphaFade = alphaFade;
            sAlphaList[sAlphaCurr].SortValue = camdot;
            sAlphaList[sAlphaCurr].SortValue += minst->sortBias / 32.0f;
            sAlphaList[sAlphaCurr].Layer = minst->Pipe.Layer;
            sAlphaCurr++;
        } else {
            DBprintf(DBML_ERR, "ALPHA BUFFER OVERFLOWED...\n");
        }
    } else {
        xASSERT_L(1097, minst != bucket->List);
        minst->BucketNext = bucket->List;
        xASSERT_L(1113, minst != minst->BucketNext);
        bucket->List = minst;
    }
}

void xModelBucket_AddAlphaFade(xModelInstance* minst, F32 alphaFade, bool)
{
    if (alphaFade <= 0.0f) {
        return;
    }

    if ((minst->Flags & 0x401) != 0x1) {
        return;
    }

#if DEBUG || RELEASE
    if (gxModelBucketProfileModel == minst->Data) {
        gxModelBucketProfileCount++;
        return;
    }

    if (!sRenderModels) {
        return;
    }

    if (!(RpAtomicGetFlags(minst->Data) & rpATOMICRENDER)) {
        return;
    }
#endif

    RpAtomic* model = minst->Data;

#if DEBUG || RELEASE
    if (gScreenDump != exSD_MASSIVE)
#endif
    {
        if (xModelCullSingle(minst)) {
            return;
        }
    }

    xMat4x3* cammat = &xglobals->cam->mat;
    F32 camdist2 = xsqr(model->worldBoundingSphere.center.x - cammat->pos.x) +
                   xsqr(model->worldBoundingSphere.center.y - cammat->pos.y) +
                   xsqr(model->worldBoundingSphere.center.z - cammat->pos.z);

    if (camdist2 >= xsqr(minst->FadeEnd)) {
        return;
    }

    xModelBucket* bucket;
    F32 camdot = cammat->at.x * (model->worldBoundingSphere.center.x - cammat->pos.x) +
                 cammat->at.y * (model->worldBoundingSphere.center.y - cammat->pos.y) +
                 cammat->at.z * (model->worldBoundingSphere.center.z - cammat->pos.z);

    if (camdot < 1.5f * model->worldBoundingSphere.radius) {
        bucket = minst->Bucket[1];
    } else {
        bucket = minst->Bucket[0];
    }

    bool alphaFlags = (minst->Pipe.Flags & (eSRCBLEND_MASK | eDESTBLEND_MASK)) != 0;
    bool alphaLOD = 1.0f != alphaFade;
    bool alphaModel = 1.0f != minst->Alpha;

    if (minst->Pipe.Flags & eALPHASORTDISABLE) {
        alphaFlags = false;
        alphaModel = false;
    }
    else if ((minst->Pipe.Flags & eFORCEOPAQUE_MASK) == ePIPEFORCEOPAQUE_BLEND)
        alphaFlags = false;
    else if ((minst->Pipe.Flags & eFORCEOPAQUE_MASK) == ePIPEFORCEOPAQUE_ALWAYS)
        alphaFlags = alphaLOD = alphaModel = false;

    bool alphaProg = alphaLOD || alphaModel;

    if (alphaFlags || alphaProg) {
        if (sAlphaCurr < sAlphaCount) {
            sAlphaList[sAlphaCurr].Data = bucket->Data;
            sAlphaList[sAlphaCurr].MInst = minst;
            sAlphaList[sAlphaCurr].AlphaFade = alphaFade;
            sAlphaList[sAlphaCurr].SortValue = camdot;
            sAlphaList[sAlphaCurr].SortValue += minst->sortBias / 32.0f;
            sAlphaList[sAlphaCurr].Layer = minst->Pipe.Layer;
            sAlphaCurr++;
        } else {
            DBprintf(DBML_ERR, "ALPHA BUFFER OVERFLOWED...\n");
        }
    } else {
        xASSERT_L(1266, minst != bucket->List);
        minst->BucketNext = bucket->List;
        xASSERT_L(1270, minst != minst->BucketNext);
        bucket->List = minst;
    }
}

void xModelBucket_RenderOpaqueBegin()
{
    sBucketOpaqueRenderStart = 0;
}

void xModelBucket_RenderOpaque(S32 maxLayer)
{
    S32 i;

    xModelBucketEnabled = FALSE;

    for (i = sBucketOpaqueRenderStart; i < sBucketCount; i++) {
#if DEBUG || RELEASE
        if (!sRenderClones) {
            continue;
        }
#endif

        xModelBucket* bucket = &sBucketList[i];
        RpAtomic* data = bucket->Data;
        xModelInstance* minst = bucket->List;

        if ((bucket->Pipe.Flags & eALPHASORTDISABLE) && bucket->Pipe.Layer > maxLayer) {
            break;
        }

        while (minst) {
#if DEBUG || RELEASE
            if (sRenderModels) {
#endif
#if DEBUG
                static U32 check_model = 0;
                if (check_model) {
                    const char* model_name = xSTAssetNameFromPointer(minst->Data);
                    if (model_name && xStrHash(model_name) == check_model) {
                        S32 asdf = 1234;
                    }
                }

                static U32 check_ent = 0;
                if (check_ent) {
                    if (minst->Object && ((xBase*)minst->Object)->id == check_ent) {
                        S32 asdf = 1234;
                    }
                }
#endif

                if (minst->Surf) {
                    xEntSetupPipeline(minst->Surf, data);
                }

                RpAtomic* oldmodel = minst->Data;
                minst->Data = data;

                S32 flags = minst->Pipe.Flags;

                if ((flags & eCULL_MASK) == ePIPECULL_FRONTONLY) {
                    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
                }

                if (flags & eFOGDISABLE) {
                    iCameraSetFog(false);
                }
                xModelBucket_SetBlendAndZModes(flags);

                if (flags & eBRIGHT) {
                    xFXHighDynamicRangeBrighten(true, false);
                }
                if (flags & eZBIAS) {
                    xRenderStateSetZBias(1.0f);
                }

                xModelRenderSingle(minst);

                if (flags & eFOGDISABLE) {
                    iCameraSetFog(true);
                }

                if ((flags & eCULL_MASK) == ePIPECULL_FRONTONLY) {
                    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
                }

                xModelBucket_ResetBlendAndZModes(flags);

                if (flags & eBRIGHT) {
                    xFXHighDynamicRangeBrighten(false, false);
                }
                if (flags & eZBIAS) {
                    xRenderStateResetZBias();
                }

                minst->Data = oldmodel;
#if DEBUG || RELEASE
            }
#endif

            if (minst == minst->BucketNext) {
                xFAIL_FMT_L(1573, "%s", "BucketNext is the same as minst.!!\nModel may be added twice. Please look at:\nminst->BucketNext = bucket->List;\nbucket->List = minst;\nin xModelBucket_Add");
            }
            minst = minst->BucketNext;
        }

        bucket->List = NULL;
    }

    sBucketOpaqueRenderStart = i;
}

void xModelBucket_RenderAlphaBegin()
{
    xModelBucketEnabled = FALSE;
    sAlphaStart = 0;
    xModelAlphaBucketEnabled = FALSE;

#if DEBUG || RELEASE
    if (!sRenderAlpha)
        sAlphaStart = sAlphaCurr;
    else
#endif
    if (sAlphaCurr != 0) {
        quicksort(sAlphaList, sAlphaCurr, sizeof(xModelAlphaBucket), CmpAlphaBucket());
    }
}

#if DEBUG || RELEASE
void ValidateRenderStateExpectations(const xModelInstance* model, U32 pipeFlags)
{
    static S32 enableValidation = FALSE;
    if (!enableValidation) {
        return;
    }

    static const xModelInstance* lastModel = NULL;
    if (!model) {
        lastModel = NULL;
        if (!pipeFlags) {
            return;
        }
    }

    S32 isProblem = 0;
    isProblem += xRenderStateCheckBlendModes((RwBlendFunction)((pipeFlags >> eSRCBLEND_SHIFT) & 0xf), (RwBlendFunction)((pipeFlags >> eDESTBLEND_SHIFT) & 0xf));

    if ((pipeFlags & eZBUFFER_MASK) == ePIPEZBUFFER_DISABLE)
        isProblem += xRenderStateCheckState(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
    else
        isProblem += xRenderStateCheckState(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);

    if ((pipeFlags & eCULL_MASK) == ePIPECULL_FRONTONLY)
        isProblem += xRenderStateCheckState(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
    else if ((pipeFlags & eCULL_MASK) <= ePIPECULL_DOUBLESIDED)
        isProblem += xRenderStateCheckState(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
    else
        xASSERT_L(1726, !(pipeFlags&&eCULL_MASK));

    if (pipeFlags & eFOGDISABLE)
        isProblem += xRenderStateCheckState(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    else
        isProblem += xRenderStateCheckState(rwRENDERSTATEFOGENABLE, (void*)TRUE);

    if (pipeFlags & eBRIGHT) {
        xASSERT_L(1753, HDR_brightening);
        if (!HDR_brightening)
            isProblem++;
    } else {
        xASSERT_L(1758, !HDR_brightening);
        if (HDR_brightening) {
            isProblem++;
        }
    }

    if (isProblem) {
        iprintf("Minimalism Render state failure\n{\n");
        iprintf("\t%-12s: 0x%08X\n", "PipeFlags", pipeFlags);
        xUtil_spit_flagbits(pipeFlags, 0);

        if (model) {
            PKRAssetTOCInfo info_curr;
            en_pkrfindmyass quality = xSTRandMemToAsset(&info_curr, model->Data);
            iprintf("\t-- Current Render --\n");
            if (quality == PKR_FIND_FAILED) {
                iprintf("\t%-12s: ?--?\n", "AID");
                iprintf("\t%-12s: ?--?\n", "Name");
            } else {
                iprintf("\t%-12s: 0x%08X (q=%d)\n", "AID", info_curr.aid, quality);
                iprintf("\t%-12s: %s\n", "Name", xSceneID2Name((xScene*)xglobals->sceneCur, info_curr.aid));
            }
            iprintf("\t%-12s: 0x%p\n", "Model", model);
            iprintf("\t%-12s: 0x%p\n", "Atomic", model->Data);
        }

        if (lastModel) {
            PKRAssetTOCInfo info_last;
            en_pkrfindmyass quality = xSTRandMemToAsset(&info_last, lastModel->Data);
            iprintf("\t -- Previous Render --\n");
            if (quality == PKR_FIND_FAILED) {
                iprintf("\t%-12s: ?--?\n", "AID");
                iprintf("\t%-12s: ?--?\n", "Name");
            } else {
                iprintf("\t%-12s: 0x%08X (q=%d)\n", "AID", info_last.aid, quality);
                iprintf("\t%-12s: %s\n", "Name", xSceneID2Name((xScene*)xglobals->sceneCur, info_last.aid));
            }
            iprintf("\t%-12s: 0x%p\n", "Model", model);
            iprintf("\t%-12s: 0x%p\n", "Atomic", model->Data);
        }

        iprintf("}\n");
    }

    lastModel = model;
}
#else
inline void ValidateRenderStateExpectations(const xModelInstance*, U32)
{
}
#endif

static void xModelBucket_RenderAlphaLayerInternal(S32 maxLayer)
{
    U32 pipeMask, curPipeFlags, lastPipeFlags;
    U8 curAlphaDiscard, lastAlphaDiscard;
    RwFogType oldfogtype;
    S32 layer;
    xModelInstance* minst;

    pipeMask = 0xFFF3FF38;

    lastPipeFlags = (ePIPEBLEND_SRCALPHA << eSRCBLEND_SHIFT) | (ePIPEBLEND_INVSRCALPHA << eDESTBLEND_SHIFT);
    lastAlphaDiscard = 0;

#if DEBUG
    xRenderStateCheckBlendModes((RwBlendFunction)((lastPipeFlags >> eSRCBLEND_SHIFT) & 0xf), (RwBlendFunction)((lastPipeFlags >> eDESTBLEND_SHIFT) & 0xf));
#endif

    oldfogtype = xglobals->fog.type;
    if (!oldfogtype) {
        pipeMask &= ~eFOGDISABLE;
    }

    ValidateRenderStateExpectations(NULL, 0);

    while (sAlphaStart < sAlphaCurr) {
        layer = sAlphaList[sAlphaStart].Layer;
        if (layer > maxLayer) {
            break;
        }

        minst = sAlphaList[sAlphaStart].MInst;

#if DEBUG
        static U32 check_model = 0;
        if (check_model) {
            const char* model_name = xSTAssetNameFromPointer(minst->Data);
            if (model_name && xStrHash(model_name) == check_model) {
                S32 asdf = 1234;
            }
        }

        static U32 check_ent = 0;
        if (check_ent) {
            if (minst->Object && ((xBase*)minst->Object)->id == check_ent) {
                S32 asdf = 1234;
            }
        }
#endif

        RpAtomic* oldmodel = minst->Data;
        minst->Data = sAlphaList[sAlphaStart].Data;

        if (minst->Surf) {
            xEntSetupPipeline(minst->Surf, minst->Data);
        }

        curPipeFlags = minst->Pipe.Flags;
        if (!(curPipeFlags & eSRCBLEND_MASK)) {
            curPipeFlags |= ePIPEBLEND_ONE << eSRCBLEND_SHIFT;
        }
        if (!(curPipeFlags & eDESTBLEND_MASK)) {
            curPipeFlags |= ePIPEBLEND_ZERO << eDESTBLEND_SHIFT;
        }

        curPipeFlags &= pipeMask;

        curAlphaDiscard = minst->Pipe.AlphaDiscard;
        F32 oldAlpha = minst->Alpha;
        F32 fade = sAlphaList[sAlphaStart].AlphaFade;
        minst->Alpha *= fade;

#if DEBUG
        xRenderStateCheckBlendModes((RwBlendFunction)((lastPipeFlags >> eSRCBLEND_SHIFT) & 0xf), (RwBlendFunction)((lastPipeFlags >> eDESTBLEND_SHIFT) & 0xf));
#endif

        U32 xorPipeFlags = curPipeFlags ^ lastPipeFlags;
        bool fadeChanged = false;

        if (xorPipeFlags || fadeChanged) {
            RwBlendFunction srcBlend = (RwBlendFunction)((curPipeFlags >> eSRCBLEND_SHIFT) & 0xf);
            RwBlendFunction destBlend = (RwBlendFunction)((curPipeFlags >> eDESTBLEND_SHIFT) & 0xf);
            srcBlend = srcBlend ? srcBlend : (RwBlendFunction)((lastPipeFlags >> eSRCBLEND_SHIFT) & 0xf);
            destBlend = destBlend ? destBlend : (RwBlendFunction)((lastPipeFlags >> eDESTBLEND_SHIFT) & 0xf);

            if (xorPipeFlags & eSRCBLEND_MASK) {
                xASSERT_FMT_L(2034, ((curPipeFlags >> eSRCBLEND_SHIFT) & 0xf) != ePIPEBLEND_SRCCOLOR, "Model %s: SRCBLEND=SRCCOLOR not supported.  Use SRCBLEND=SRCALPHA.", xSTAssetNameFromPointer(minst->Data));
            }

            if (xorPipeFlags & eDESTBLEND_MASK) {
                xASSERT_FMT_L(2041, ((curPipeFlags >> eDESTBLEND_SHIFT) & 0xf) != ePIPEBLEND_SRCCOLOR, "Model %s: DESTBLEND=SRCCOLOR not supported.  Use DESTBLEND=SRCALPHA.", xSTAssetNameFromPointer(minst->Data));
            }

            if ((xorPipeFlags & eSRCBLEND_MASK) || (xorPipeFlags & eDESTBLEND_MASK)) {
                if (xorPipeFlags & eZBUFFER_MASK) {
                    if ((curPipeFlags & eZBUFFER_MASK) == ePIPEZBUFFER_DISABLE) {
                        xRenderStateBlendAndZModesSet(srcBlend, destBlend, FALSE, TRUE);
                    } else {
                        xRenderStateBlendAndZModesSet(srcBlend, destBlend, TRUE, TRUE);
                    }
                } else {
                    xRenderStateBlendModesSet(srcBlend, destBlend);
                }
            } else {
                if (xorPipeFlags & eZBUFFER_MASK) {
                    if (curPipeFlags & ePIPEZBUFFER_DISABLE) {
                        xRenderStateZModesSet(FALSE, TRUE);
                    } else {
                        xRenderStateZModesSet(TRUE, TRUE);
                    }
                }
            }

            if (xorPipeFlags & eCULL_MASK) {
                if ((curPipeFlags & eCULL_MASK) == ePIPECULL_FRONTONLY) {
                    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
                }
                else if ((curPipeFlags & eCULL_MASK) <= ePIPECULL_DOUBLESIDED) {
                    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
                }
            }

            if (xorPipeFlags & eFOGDISABLE) {
                if (curPipeFlags & eFOGDISABLE) {
                    iCameraSetFog(false);
                } else {
                    iCameraSetFog(true);
                }
            }

            if (xorPipeFlags & eBRIGHT) {
                xFXHighDynamicRangeBrighten((curPipeFlags & eBRIGHT) != 0, false);
            }

            lastPipeFlags = curPipeFlags;
        }

        if (curAlphaDiscard) {
            RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_GEQUAL, curAlphaDiscard);
            GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_GEQUAL, curAlphaDiscard);
            _rwDlRenderStateSetZCompLoc(FALSE);
            GXSetZCompLoc(GX_FALSE);
        } else {
            RwGameCubeSetAlphaCompare(GX_GEQUAL, 0, GX_AOP_AND, GX_ALWAYS, 0);
            GXSetAlphaCompare(GX_GEQUAL, 0, GX_AOP_AND, GX_ALWAYS, 0);
            _rwDlRenderStateSetZCompLoc(TRUE);
            GXSetZCompLoc(GX_TRUE);
        }
        lastAlphaDiscard = curAlphaDiscard;

        if (curPipeFlags & eBRIGHT) {
            xASSERT_L(2174, minst->Data);
            xRenderFixUntexturedBegin(minst->Data);
        }

        if (curPipeFlags & eZBIAS) {
            xRenderStateSetZBias(1.0f);
        }

        if ((curPipeFlags & eCULL_MASK) == ePIPECULL_BACKTHENFRONT) {
            RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLFRONT);
            xModelRenderSingle(minst);
            RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
            xModelRenderSingle(minst);
        }
        else if ((curPipeFlags & eZBUFFER_MASK) == ePIPEZBUFFER_ZFIRST) {
            iDrawSetFBMSK(0xFFFFFFFF, 0x00FFFFFF, true);
            xModelRenderSingle(minst);
            iDrawSetFBMSK(0, 0x00FFFFFF, true);
            xModelRenderSingle(minst);
        } else {
            xModelRenderSingle(minst);
        }

        if (curPipeFlags & eBRIGHT) {
            xRenderFixUntexturedEnd(minst->Data);
        }

        if (curPipeFlags & eZBIAS) {
            xRenderStateResetZBias();
        }

        minst->Alpha = oldAlpha;

        if (minst->Surf) {
            xEntRestorePipeline(minst->Surf, minst->Data);
        }

        minst->Data = oldmodel;

        sAlphaStart++;
    }

    if (lastPipeFlags != ((ePIPEBLEND_SRCALPHA << eSRCBLEND_SHIFT) | (ePIPEBLEND_INVSRCALPHA << eDESTBLEND_SHIFT))) {
        if ((lastPipeFlags & eZBUFFER_MASK) == ePIPEZBUFFER_DISABLE) {
            xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA, TRUE, TRUE);
        } else {
            xRenderStateBlendModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);
        }

        if ((lastPipeFlags & eCULL_MASK) == ePIPECULL_FRONTONLY || (lastPipeFlags & eCULL_MASK) == ePIPECULL_BACKTHENFRONT) {
            RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
        }

        if (lastPipeFlags & eFOGDISABLE) {
            iCameraSetFog(true);
        }

        if (lastPipeFlags & eBRIGHT) {
            xFXHighDynamicRangeBrighten(false, false);
        }
    }

    RwGameCubeSetAlphaCompare(GX_GEQUAL, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetAlphaCompare(GX_GEQUAL, 0, GX_AOP_AND, GX_ALWAYS, 0);
    _rwDlRenderStateSetZCompLoc(TRUE);
    GXSetZCompLoc(GX_TRUE);
}

#if DEBUG || RELEASE
#define MAX_MISSING_MODELS 16

namespace {
    xModelBadLayer xModelBucket_BadSetModels[MAX_MISSING_MODELS];
    S32 currentCountBadSetModels;

    void xModelBucket_CheckModel(U32 modelID, S32 curLayer, S32 modelLayer)
    {
        for (S32 i = 0; i < currentCountBadSetModels; i++) {
            xModelBadLayer& model = xModelBucket_BadSetModels[i];
            if (model.modelID == modelID) {
                return;
            }
        }

        if (currentCountBadSetModels < MAX_MISSING_MODELS) {
            xModelBadLayer& tempModel = xModelBucket_BadSetModels[currentCountBadSetModels];
            if (!modelLayer || !curLayer) {
                tempModel.modelID = modelID;
                tempModel.curLayer = curLayer;
                tempModel.modelLayer = modelLayer;
                currentCountBadSetModels++;
            }
        }
    }

    void xModelBucket_DoGCAssertsInternal()
    {
        xASSERT_FMT_L(2388, (currentCountBadSetModels < MAX_MISSING_MODELS), "You've just reached the limit(%d) for badly set models ! \nFix the pipespec file !", MAX_MISSING_MODELS);
        for (S32 i = 0; i < currentCountBadSetModels; i++) {
            xModelBadLayer& model = xModelBucket_BadSetModels[i];
            if (!model.asserted) {
                xFAIL_FMT_L(2394, "Model 0x%08X just broke the alpha bucket list, \nmost likely because is not in any pipespec file or is missing a LAYER setting (but should have it). \nIf you see only 0s in the id then ask programmer for help !\nYou will see the assert for this model only once in this scene.", model.modelID);
                model.asserted = TRUE;
            }
        }
    }

    void xModelBucket_ClearModelsArray()
    {
        for (S32 i = 0; i < MAX_MISSING_MODELS; i++) {
            xModelBadLayer& model = xModelBucket_BadSetModels[i];
            model.modelID = 0;
            model.asserted = FALSE;
            model.curLayer = 0;
            model.modelLayer = 0;
        }
    }
}

void xModelBucket_InitBadSetModelsCheck()
{
    currentCountBadSetModels = 0;
    xModelBucket_ClearModelsArray();
}

void xModelBucket_DoGCAsserts()
{
    xModelBucket_DoGCAssertsInternal();
}
#endif

void xModelBucket_RenderAlphaLayer(S32 maxLayer)
{
    S32 lastLayer = 0;
    while (sAlphaStart < sAlphaCurr) {
        S32 layer = sAlphaList[sAlphaStart].Layer;
        if (layer > maxLayer)
            layer = maxLayer;
        if (layer == lastLayer && maxLayer != 31) {
#if DEBUG || RELEASE
            xModelBucket_CheckModel(sAlphaList[sAlphaStart].MInst->modelID, maxLayer, layer);
#endif
            break;
        }
        xModelBucket_RenderOpaque(layer);
        xModelBucket_RenderAlphaLayerInternal(layer);
        lastLayer = layer;
    }
    if (lastLayer < maxLayer)
        xModelBucket_RenderOpaque(maxLayer);
}

bool xModelBucket_RenderAny(S32 maxLayer)
{
    if (sAlphaStart < sAlphaCurr && (S32)sAlphaList[sAlphaStart].Layer <= maxLayer)
        return true;
    return sBucketOpaqueRenderStart < sBucketCount && sBucketList[sBucketOpaqueRenderStart].Pipe.Layer <= maxLayer;
}

void xModelBucket_RenderAlphaEnd()
{
    sAlphaCurr = 0;
    xModelAlphaBucketEnabled = TRUE;
}

void xModelBucket_Deinit()
{
    for (S32 i = 0; i < sBucketCount; i++) {
        if (sBucketList[i].Data != sBucketList[i].OriginalData) {
            _rwFrameSyncDirty();
            iModelUnload(sBucketList[i].Data);
        }
    }

    xModelBucket_PreCountReset();
}

static void xModelBucket_SetBlendAndZModes(S32 flags)
{
    if ((flags & eSRCBLEND_MASK) || (flags & eDESTBLEND_MASK)) {
        RwBlendFunction srcBlend = (RwBlendFunction)((flags >> eSRCBLEND_SHIFT) & 0xf);
        srcBlend = srcBlend ? srcBlend : rwBLENDSRCALPHA;
        RwBlendFunction destBlend = (RwBlendFunction)((flags >> eDESTBLEND_SHIFT) & 0xf);
        destBlend = destBlend ? destBlend : rwBLENDINVSRCALPHA;

        if ((flags & eZBUFFER_MASK) == ePIPEZBUFFER_DISABLE) {
            xRenderStateBlendAndZModesSet(srcBlend, destBlend, FALSE, TRUE);
        } else {
            xRenderStateBlendModesSet(srcBlend, destBlend);
        }
    } else {
        if ((flags & eZBUFFER_MASK) == ePIPEZBUFFER_DISABLE) {
            xRenderStateZModesSet(FALSE, TRUE);
        }
    }
}

static void xModelBucket_ResetBlendAndZModes(S32 flags)
{
    if ((flags & eSRCBLEND_MASK) || (flags & eDESTBLEND_MASK)) {
        if ((flags & eZBUFFER_MASK) == ePIPEZBUFFER_DISABLE) {
            xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA, TRUE, TRUE);
        } else {
            xRenderStateBlendModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);
        }
    } else {
        if ((flags & eZBUFFER_MASK) == ePIPEZBUFFER_DISABLE) {
            xRenderStateZModesSet(TRUE, TRUE);
        }
    }
}
