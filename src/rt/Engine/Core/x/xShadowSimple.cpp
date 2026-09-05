#include "xShadowSimple.h"

#include "xClumpColl.h"
#include "xEnt.h"
#include "xEnv.h"
#include "xGlobals.h"
#include "xIM.h"
#include "xJSP.h"
#include "xRenderState.h"
#include "zGrid.h"
#include "zScene.h"

#include <string.h>

struct xShadowSimpleQueue
{
    xShadowSimpleCache* cache;
    U32 priority;
    const xMat4x3* modelMatrix;
    S32 hasMatCopy;
    F32 radius;
    F32 height;
    F32 ecc;
};

struct zSimpleShadowTableHeader
{
    U32 num;
};

struct zSimpleShadow
{
    U32 modelID;
    U32 shadowID;
    U32 flags;
};

#define MAX_SHAD_QUADS 64
#define MAX_SHAD_VERTS (MAX_SHAD_QUADS * 6)
#define MAX_SHAD_RASTERS 64

static xShadowSimpleQueue sCollQueue[2];
static xMat4x3 sCollModelMatrices[2];
static RxRenderStateVector xrsv;
static RwIm3DVertex sShadVert[MAX_SHAD_VERTS];
static RwRaster* sShadRasters[MAX_SHAD_RASTERS];
static RwRaster* sShadRaster;
static U32 sShadVertCount;
static RwMatrix* sModelMat;

static RpCollisionTriangle* shadowRayCB(RpIntersection*, RpWorldSector*, RpCollisionTriangle* tri, RwReal dist, void* data)
{
    xVec3 xformnorm;
    xVec3* norm;
    F32* testdist;
    xShadowSimpleCache* cache = (xShadowSimpleCache*)data;

    if (sModelMat) {
        testdist = &cache->shadowHeight;
        xMat3x3RMulVec(&xformnorm, (xMat3x3*)sModelMat, (xVec3*)&tri->normal);
        xVec3Normalize(&xformnorm, &xformnorm);
        norm = &xformnorm;
    } else {
        testdist = &cache->envHeight;
        norm = (xVec3*)&tri->normal;
    }

    // 0.0871557f = cos(85 degrees)
    if (dist >= *testdist || norm->y < 0.0871557f) {
        return tri;
    }

    *testdist = dist;

    cache->poly.vert[0] = *(xVec3*)tri->vertices[0];
    cache->poly.vert[1] = *(xVec3*)tri->vertices[1];
    cache->poly.vert[2] = *(xVec3*)tri->vertices[2];
    cache->poly.norm = *(xVec3*)&tri->normal;

    return tri;
}

static RpCollisionTriangle* shadowRayModelCB(RpIntersection* isx, RpCollisionTriangle* tri, RwReal dist, void* data)
{
    return shadowRayCB(isx, NULL, tri, dist, data);
}

struct ShadowSimpleCBParam
{
    xShadowSimpleCache* cache;
    RpIntersection* isx;
};

static S32 shadowRayEntCB(xEnt* ent, void* cbdata)
{
    if (!(ent->baseFlags & k_XBASE_RECEIVES_SHADOWS)) {
        return 1;
    }

    ShadowSimpleCBParam* cbparam = (ShadowSimpleCBParam*)cbdata;

    if (cbparam->cache->collSkipsItem == ent) {
        return 1;
    }

    xModelInstance* m = ent->model;

    if (iModelNumBones(m->Data) > 1) {
        return 1;
    }

    if (xModelIsPreinstanced(ent->model)) {
        return 1;
    }

    F32 oldHeight = cbparam->cache->shadowHeight;

    RwFrame* frame = RpAtomicGetFrame(m->Data);
    RwFrameTransform(frame, m->Mat, rwCOMBINEREPLACE);

    sModelMat = m->Mat;
    RpAtomicForAllIntersections(m->Data, cbparam->isx, shadowRayModelCB, cbparam->cache);
    sModelMat = NULL;

    if (cbparam->cache->shadowHeight != oldHeight) {
        cbparam->cache->castOnEnt = ent;
    }

    return 1;
}

static void xShadowSimple_SceneCollide(xShadowSimpleCache* cache, const xVec3* pos, F32 depth)
{
    xASSERT(187, cache);

    cache->envHeight = HUGE;
    cache->shadowHeight = HUGE;
    cache->castOnEnt = NULL;
    cache->collPriority = 0;

    xEnv* env = xglobals->sceneCur->env;

    RpIntersection isx;
    isx.type = rpINTERSECTLINE;
    isx.t.line.start.x = pos->x;
    isx.t.line.start.y = 0.1f + pos->y;
    isx.t.line.start.z = pos->z;
    isx.t.line.end.x = pos->x;
    isx.t.line.end.y = pos->y - depth;
    isx.t.line.end.z = pos->z;

    for (S32 i = 0; i < env->geom->jsp_count; i++) {
        if (xEnvIsJSPActive(env, i)) {
            xClumpColl_ForAllIntersections(env->geom->jsp_list[i]->colltree, &isx, shadowRayCB, cache);
        }
    }

    if (cache->envHeight != HUGE) {
        cache->envHeight = cache->envHeight * (isx.t.line.end.y - isx.t.line.start.y) + isx.t.line.start.y;
        isx.t.line.end.y = cache->envHeight;
    }

    if (!(cache->flags & 0x10)) {
        ShadowSimpleCBParam cbparam;

        xQCData qcd;
        xQuickCullForLine(&qcd, (xLine3*)&isx.t.line);

        cbparam.cache = cache;
        cbparam.isx = &isx;

        xGridCheckPosition(&colls_grid, (xVec3*)&isx.t.line.start, &qcd, shadowRayEntCB, &cbparam);
        xGridCheckPosition(&colls_oso_grid, (xVec3*)&isx.t.line.start, &qcd, shadowRayEntCB, &cbparam);
    }

    if (cache->shadowHeight != HUGE) {
        cache->shadowHeight = cache->shadowHeight * (isx.t.line.end.y - isx.t.line.start.y) + isx.t.line.start.y;
    } else {
        cache->shadowHeight = cache->envHeight;
    }

    if (cache->shadowHeight != HUGE && !cache->castOnEnt) {
        cache->dydx = -cache->poly.norm.x / cache->poly.norm.y;
        cache->dydz = -cache->poly.norm.z / cache->poly.norm.y;
    }
}

static void xShadowSimple_CalcCorners(xShadowSimpleCache* cache, const xMat4x3* mat, F32 radius, F32 ecc)
{
    xASSERT(258, cache);
    xASSERT(259, mat);

    if (cache->shadowHeight == HUGE) {
        return;
    }

    if (cache->castOnEnt) {
        xVec3 tempnorm;
        xMat3x3RMulVec(&tempnorm, (xMat3x3*)cache->castOnEnt->model->Mat, &cache->poly.norm);
        xVec3Normalize(&tempnorm, &tempnorm);

        if (xabs(tempnorm.y) <= EPSILON) {
            cache->dydx = -tempnorm.x;
            cache->dydz = -tempnorm.z;
        } else {
            cache->dydx = -tempnorm.x / tempnorm.y;
            cache->dydz = -tempnorm.z / tempnorm.y;
        }
    }

    xVec3 shadowRight, shadowAt;
    if (cache->flags & 0x2) {
        shadowRight.x = mat->right.x * radius * ecc;
        shadowRight.z = mat->right.z * radius * ecc;
        shadowRight.y = shadowRight.x * cache->dydx + shadowRight.z * cache->dydz;
        shadowAt.x = mat->at.x * radius;
        shadowAt.z = mat->at.z * radius;
        shadowAt.y = shadowAt.x * cache->dydx + shadowAt.z * cache->dydz;
    } else {
        shadowRight.x = radius * ecc;
        shadowRight.z = 0.0f;
        shadowRight.y = shadowRight.x * cache->dydx;
        shadowAt.x = 0.0f;
        shadowAt.z = radius;
        shadowAt.y = shadowAt.z * cache->dydz;
    }

    cache->corner[0].x = mat->pos.x + shadowRight.x;
    cache->corner[0].y = cache->shadowHeight + shadowRight.y + 0.02f;
    cache->corner[0].z = mat->pos.z + shadowRight.z;
    cache->corner[1].x = mat->pos.x + shadowAt.x;
    cache->corner[1].y = cache->shadowHeight + shadowAt.y + 0.02f;
    cache->corner[1].z = mat->pos.z + shadowAt.z;
    cache->corner[2].x = mat->pos.x - shadowAt.x;
    cache->corner[2].y = cache->shadowHeight - shadowAt.y + 0.02f;
    cache->corner[2].z = mat->pos.z - shadowAt.z;
    cache->corner[3].x = mat->pos.x - shadowRight.x;
    cache->corner[3].y = cache->shadowHeight - shadowRight.y + 0.02f;
    cache->corner[3].z = mat->pos.z - shadowRight.z;
}

static void xShadowSimple_AddVerts(xShadowSimpleCache* cache)
{
    if (cache->shadowHeight == HUGE || sShadVertCount >= MAX_SHAD_VERTS) {
        return;
    }

    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 0], cache->corner[0].x, cache->corner[0].y, cache->corner[0].z);
    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 1], cache->corner[1].x, cache->corner[1].y, cache->corner[1].z);
    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 2], cache->corner[2].x, cache->corner[2].y, cache->corner[2].z);
    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 3], cache->corner[1].x, cache->corner[1].y, cache->corner[1].z);
    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 4], cache->corner[2].x, cache->corner[2].y, cache->corner[2].z);
    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 5], cache->corner[3].x, cache->corner[3].y, cache->corner[3].z);

    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 0], 0, 0, 0, cache->alpha);
    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 1], 0, 0, 0, cache->alpha);
    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 2], 0, 0, 0, cache->alpha);
    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 3], 0, 0, 0, cache->alpha);
    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 4], 0, 0, 0, cache->alpha);
    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 5], 0, 0, 0, cache->alpha);

    sShadRasters[sShadVertCount / 6] = cache->ptr_raster;
    sShadVertCount += 6;
}

void xShadowSimple_Init()
{
    memset(sCollQueue, 0, sizeof(sCollQueue));
    memset(sCollModelMatrices, 0, sizeof(sCollModelMatrices));

    // 0x1A2A0413 = xStrHash("shadow_round")
    RwTexture* tex = (RwTexture*)xSTFindAsset(0x1A2A0413, NULL);
    if (tex) {
        sShadRaster = RwTextureGetRaster(tex);
    } else {
        sShadRaster = NULL;
    }

    xASSERTM(376, sShadRaster != 0L, "%s", "who deleted the default shadow texture 'shadow_round' from boot.hip?!?");

    memset(sShadVert, 0, sizeof(sShadVert));
    for (U32 i = 0; i < MAX_SHAD_QUADS; i++) {
        sShadVert[i * 6 + 1].u = 1.0f;
        sShadVert[i * 6 + 2].v = 1.0f;
        sShadVert[i * 6 + 3].u = 1.0f;
        sShadVert[i * 6 + 4].v = 1.0f;
        sShadVert[i * 6 + 5].u = 1.0f;
        sShadVert[i * 6 + 5].v = 1.0f;

        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 0], 0.0f, 1.0f, 0.0f);
        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 1], 0.0f, 1.0f, 0.0f);
        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 2], 0.0f, 1.0f, 0.0f);
        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 3], 0.0f, 1.0f, 0.0f);
        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 4], 0.0f, 1.0f, 0.0f);
        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 5], 0.0f, 1.0f, 0.0f);
    }
}

void xShadowSimple_Reset()
{
    memset(sCollQueue, 0, sizeof(sCollQueue));
    memset(sCollModelMatrices, 0, sizeof(sCollModelMatrices));
}

void xShadowSimple_CacheInit(xShadowSimpleCache* cache, xEnt* ent, U8 alpha)
{
    memset(cache, 0, sizeof(xShadowSimpleCache));

    cache->envHeight = HUGE;
    cache->shadowHeight = HUGE;
    cache->flags = 0x1;
    cache->alpha = alpha;
    cache->collSkipsItem = ent;

    if (!ent->model || ent->model->shadowID != 0xDEADBEEF) {
        return;
    }

    S32 i, n;
    U32 j;
    zSimpleShadowTableHeader* sst;
    RwRaster* raster = NULL;
    U32 flags = 0;

    n = xSTAssetCountByType('SHDW');
    for (i = 0; i < n; i++) {
        U32 size;
        sst = (zSimpleShadowTableHeader*)xSTFindAssetByType('SHDW', i, &size);

        xASSERTM(435, size == 4+sst->num*sizeof(zSimpleShadow), "zSimpleShadow def mismatch!");

        for (j = 0; j < sst->num; j++) {
            zSimpleShadow* table = (zSimpleShadow*)(sst + 1);
            if (table[j].modelID == ent->model->modelID) {
                RwTexture* tex = (RwTexture*)xSTFindAsset(table[j].shadowID, NULL);
                if (tex) {
                    raster = RwTextureGetRaster(tex);
                    flags = table[j].flags;
                } else {
                    raster = (RwRaster*)0xDEADBEEF;
                }
            }

            if (raster) {
                break;
            }
        }

        if (raster) {
            break;
        }
    }
    
    if (!raster || raster == (RwRaster*)0xDEADBEEF) {
        raster = sShadRaster;
    }

    cache->ptr_raster = raster;
    cache->flags |= (U16)flags;

    ent->model->shadowID = (U32)raster;
}

void xShadowSimple_CacheInit(xShadowSimpleCache* cache, U8 alpha)
{
    memset(cache, 0, sizeof(xShadowSimpleCache));

    cache->envHeight = HUGE;
    cache->shadowHeight = HUGE;
    cache->flags = 0x1;
    cache->alpha = alpha;
    cache->collSkipsItem = NULL;
    cache->ptr_raster = sShadRaster;
}

void xShadowSimple_Add(xShadowSimpleCache* cache, xEnt* ent, F32 radius, F32 ecc)
{
    xShadowSimple_Add_Expert(cache, (xMat4x3*)ent->model->Mat, cache->alpha, radius, 10.0f, ecc, false);
}

void xShadowSimple_Add(xShadowSimpleCache* cache, xMat4x3* entMat, F32 radius, F32 ecc)
{
    xShadowSimple_Add_Expert(cache, entMat, cache->alpha, radius, 10.0f, ecc, true);
}

void xShadowSimple_Render()
{
    for (S32 qnum = 0; qnum < 2; qnum++) {
        if (sCollQueue[qnum].cache) {
            xShadowSimpleCache* cache = sCollQueue[qnum].cache;

            xShadowSimple_SceneCollide(cache, &sCollQueue[qnum].modelMatrix->pos, sCollQueue[qnum].height);

            cache->pos = sCollQueue[qnum].modelMatrix->pos;
            cache->at = sCollQueue[qnum].modelMatrix->at;

            xShadowSimple_CalcCorners(cache, sCollQueue[qnum].modelMatrix, sCollQueue[qnum].radius, sCollQueue[qnum].ecc);
            xShadowSimple_AddVerts(cache);
        }

        sCollQueue[qnum].cache = NULL;
    }

    if (sShadVertCount == 0) {
        return;
    }

    RxRenderStateVectorLoadDriverState(&xrsv);

    xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA, FALSE, TRUE);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
    
    for (U32 i = 0, j; i < sShadVertCount; i = j) {
        RwRaster* raster = sShadRasters[i / 6];

        j = i;
        while (j < sShadVertCount && sShadRasters[j / 6] == raster) {
            j += 6;
        }

        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)raster);
        xIMRenderLegacy(sShadVert + i, j - i, NULL, rwPRIMTYPETRILIST);
    }
    
    xRenderStateBlendAndZModesSet(xrsv.SrcBlend, xrsv.DestBlend, (xrsv.Flags & rxRENDERSTATEFLAG_ZWRITEENABLE) ? TRUE : FALSE, TRUE);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)xrsv.ShadeMode);

    sShadVertCount = 0;
}

inline void XSSC_Add_NudgeItAndFudgeIt(xShadowSimpleCache* cache, const xMat4x3* mat_owner)
{
    xVec3* vert;
    xVec3 xformvert[3];
    xVec3 xformnorm;

    if (cache->castOnEnt) {
        xEnt* castOnEnt = cache->castOnEnt;

        xMat4x3ToWorld(&xformvert[0], (xMat4x3*)castOnEnt->model->Mat, &cache->poly.vert[0]);
        xMat4x3ToWorld(&xformvert[1], (xMat4x3*)castOnEnt->model->Mat, &cache->poly.vert[1]);
        xMat4x3ToWorld(&xformvert[2], (xMat4x3*)castOnEnt->model->Mat, &cache->poly.vert[2]);

        xMat3x3RMulVec(&xformnorm, (xMat4x3*)castOnEnt->model->Mat, &cache->poly.norm);
        xVec3Normalize(&xformnorm, &xformnorm);

        // BUG: Should be xabs(xformnorm.y) > EPSILON
        if (xabs(xformnorm.y > EPSILON)) {
            cache->dydx = -xformnorm.x / xformnorm.y;
            cache->dydz = -xformnorm.z / xformnorm.y;
        } else {
            F32 poo = (xformnorm.y > 0.0f) ? EPSILON : -EPSILON;
            cache->dydx = -xformnorm.x / poo;
            cache->dydz = -xformnorm.z / poo;
        }

        vert = &xformvert[0];
    } else {
        vert = &cache->poly.vert[0];
        if (vert->x == HUGE) {
            return;
        }
    }

    F32 nx, nz, pdot;
    for (S32 i = 0; i < 3; i++) {
        xVec3* v0 = &vert[i];
        xVec3* v1 = &vert[i == 2 ? 0 : i + 1];
        nx = v0->z - v1->z;
        nz = v1->x - v0->x;
        pdot = nx * (mat_owner->pos.x - v0->x) + nz * (mat_owner->pos.z - v0->z);
        if (pdot > EPSILON) {
            cache->collPriority += 20;
            break;
        }
    }
    
    cache->shadowHeight =
        cache->dydx * (mat_owner->pos.x - vert->x) +
        vert->y +
        cache->dydz * (mat_owner->pos.z - vert->z);
}

inline S32 XSSC_Add_ShoveIntoQueue(xShadowSimpleCache* cache, const xMat4x3* mat_owner, F32 radius, F32 height, F32 ecc, bool copyMat)
{
    for (S32 i = 0; i < 2; i++) {
        if (!sCollQueue[i].cache) {
            sCollQueue[i].cache = cache;
            sCollQueue[i].priority = sCollQueue[i].cache->collPriority;
            sCollQueue[i].hasMatCopy = copyMat;
            if (copyMat) {
                xMat4x3Copy(&sCollModelMatrices[i], mat_owner);
                sCollQueue[i].modelMatrix = &sCollModelMatrices[i];
            } else {
                sCollQueue[i].modelMatrix = mat_owner;
            }
            sCollQueue[i].radius = radius;
            sCollQueue[i].height = height;
            sCollQueue[i].ecc = ecc;
            return 1;
        }

        if (cache->collPriority > sCollQueue[i].cache->collPriority) {
            if (sCollQueue[1].cache) {
                xShadowSimple_CalcCorners(sCollQueue[1].cache, sCollQueue[1].modelMatrix, sCollQueue[1].radius, sCollQueue[1].ecc);
                xShadowSimple_AddVerts(sCollQueue[1].cache);
            }
            for (S32 j = 0; j >= i; j--) {
                sCollQueue[j + 1] = sCollQueue[j];
                if (sCollQueue[j + 1].hasMatCopy) {
                    xMat4x3Copy(&sCollModelMatrices[j + 1], &sCollModelMatrices[j]);
                    sCollQueue[j + 1].modelMatrix = &sCollModelMatrices[j + 1];
                }
            }
            sCollQueue[i].cache = cache;
            sCollQueue[i].priority = sCollQueue[i].cache->collPriority;
            sCollQueue[i].hasMatCopy = copyMat;
            if (copyMat) {
                xMat4x3Copy(&sCollModelMatrices[i], mat_owner);
                sCollQueue[i].modelMatrix = &sCollModelMatrices[i];
            } else {
                sCollQueue[i].modelMatrix = mat_owner;
            }
            sCollQueue[i].radius = radius;
            sCollQueue[i].height = height;
            sCollQueue[i].ecc = ecc;
            return 1;
        }
    }
    
    return 0;
}

void xShadowSimple_Add_Expert(xShadowSimpleCache* cache, const xMat4x3* mat_owner, U8 alpha, F32 radius, F32 height, F32 ecc, bool copyMat)
{
    xASSERT(771, cache);

    if (radius > 10.0f) {
        return;
    }

    cache->alpha = alpha;

    if (radius >= 0.0f) {
        cache->radiusOptional = radius;
    }
    if (radius < 0.0f && cache->radiusOptional >= 0.0f) {
        radius = cache->radiusOptional;
    }

    xASSERT(781, radius>=0.0f);

    if (cache->flags & 0x1) {
        cache->flags &= (U16)~0x1;
        cache->poly.vert[0].x = HUGE;

        xShadowSimple_SceneCollide(cache, &mat_owner->pos, height);

        cache->pos = mat_owner->pos;
        cache->at = mat_owner->at;

        if (cache->shadowHeight != HUGE) {
            xShadowSimple_CalcCorners(cache, mat_owner, radius, ecc);
            xShadowSimple_AddVerts(cache);
        }
    } else {
        U32 ownerMoved;
        if (cache->flags & 0x8) {
            xVec3 delta;
            delta.Sub(cache->pos, mat_owner->pos);
            F32 ds2_moved = delta.length2();
            ownerMoved = (ds2_moved > xsqr(cache->tol_movement));
        } else {
            ownerMoved = (cache->pos.x != mat_owner->pos.x || cache->pos.y != mat_owner->pos.y || cache->pos.z != mat_owner->pos.z);
        }

        if ((cache->flags & 0x8) && (ownerMoved || cache->castOnEnt)) {
            XSSC_Add_NudgeItAndFudgeIt(cache, mat_owner);
        } else if (cache->shadowHeight != HUGE && (ownerMoved || cache->castOnEnt)) {
            XSSC_Add_NudgeItAndFudgeIt(cache, mat_owner);
            if (ownerMoved) {
                cache->envHeight = HUGE;
            }
        }

        if (ownerMoved) {
            cache->collPriority += 6;
        } else if (cache->castOnEnt) {
            cache->collPriority += 3;
        } else {
            cache->collPriority += 2;
        }

        S32 added = XSSC_Add_ShoveIntoQueue(cache, mat_owner, radius, height, ecc, copyMat);
        if (!added) {
            xShadowSimple_CalcCorners(cache, mat_owner, radius, ecc);
            xShadowSimple_AddVerts(cache);
        }
    }
}
