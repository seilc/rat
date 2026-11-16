#include "xCommon.h"
#include "zFrostEffect.h"

#include "xDecal.h"
#include "xEnt.h"
#include "xFont.h"
#include "xModel.h"
#include "xRand.h"
#include "xstransvc.h"
#include "xutil.h"
#include "zHashes.h"

#include "decomp.h"

#define MAX_ICICLES 128

namespace
{
    xTWEAKABLE F32 MAX_DECAL_CAST_DIST = 5.0f;
    xTWEAKABLE F32 MAX_DECAL_CAST_DIST_HALF = 0.75f * MAX_DECAL_CAST_DIST;
    xTWEAKABLE F32 FROST_DECAL_LIFETIME = 5.0f;
    xTWEAKABLE F32 FROST_EFFECT_EMIT_RATE = 0.05f;
    xTWEAKABLE F32 DECAL_MAX_SIZE = 2.0f;
    xTWEAKABLE F32 DECAL_MIN_SIZE = 0.5f;

    F32 sEmitTimer = 1e38f;
}

static zFrostEffect sFrostEmitter;

void zFrostEffect::ApplyFrost(xRay3& ray, F32 emitRate, F32 emitDistance)
{
    sFrostEmitter._ApplyFrost(ray, emitRate, emitDistance);
}

// UNUSED: needed to match (code from RotU GC)
void zFrostEffect::_Init()
{
    static const xDecal::curve_node decalCurve[5] = {
        { 0.0f, { 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f } },
        { 0.04f, { 1.0f, 1.0f, 1.0f, 0.8f, 1.0f, 1.0f } },
        { 0.64f, { 1.0f, 1.0f, 1.0f, 0.8f, 1.0f, 1.0f } },
        { 0.88f, { 1.0f, 1.0f, 1.0f, 0.27f, 1.0f, 1.0f } },
        { 1.0f, { 1.0f, 1.0f, 1.0f, 0.0f, 0.7f, 0.7f } },
    };

    mDecalResponseCurve.reset(6, decalCurve, ARRAY_LENGTH(decalCurve));

    mDecalEmitContext.owner = 'FRST';
    mDecalEmitContext.flags = xDecal::emit_context::FLAG_ANIM_COLOR | xDecal::emit_context::FLAG_ANIM_UV;
    mDecalEmitContext.priority = 0;
    mDecalEmitContext.blend_mode = xDecal::BLEND_MODE_AVERAGE;
    mDecalEmitContext.uv[0].assign(0.0f, 0.0f);
    mDecalEmitContext.uv[1].assign(1.0f, 1.0f);
    mDecalEmitContext.curve = &mDecalResponseCurve;
    mDecalEmitContext.texture = (RwTexture*)xSTFindAsset(HASH_frost_decal, NULL);
    mDecalEmitContext.emitAtomicCB = zFrostIcicle::DecalCreateAtomicCB;
    mDecalEmitContext.emitVertListCB = zFrostIcicle::DecalCreateVertListCB;
    mDecalEmitContext.destAtomicCB = NULL;
    mDecalEmitContext.context = NULL;

    zFrostIcicle::Init();
    InitDebugTweak();
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE(
    "FX|Frost Effect",
    "%s|Decal|Max Frost Decals",
    "%s|Decal|Max Casting Distance",
    "%s|Decal|Lifetime",
    "%s|Decal|Max Size",
    "%s|Decal|Min Size",
    "%s|Emit Rate"
)
#else
// UNUSED: needed to match (code from RotU GC)
void zFrostEffect::InitDebugTweak()
{
}
#endif

void zFrostEffect::_ApplyFrost(xRay3& ray, F32 emitRate, F32 emitDistance)
{
    if (emitRate < 0.0f) {
        emitRate = FROST_EFFECT_EMIT_RATE;
    }

    if (sEmitTimer < emitRate) {
        return;
    }

    sEmitTimer = 0.0f;
    if (!emitDistance) {
        emitDistance = MAX_DECAL_CAST_DIST;
    }

    xASSERT_FMT_L(187, mDecalEmitContext.texture, "Could not find texture 0x%X for the frost decal.", HASH_frost_decal);

    F32 decalSize = xrand_RandomFloatRange(DECAL_MIN_SIZE, DECAL_MAX_SIZE);
    mDecalEmitContext.size.assign(decalSize, decalSize);
    mDecalEmitContext.max_dist = emitDistance;
    mDecalEmitContext.life = FROST_DECAL_LIFETIME;

    xMat4x3 mat;
    xVec3 vec = ray.dir;
    vec.negate();
    xMat3x3LookVec3(mat, vec);

    xMat3x3 rotZ;
    xMat3x3RotZ(&rotZ, 2.0f * PI * xrand_RandomFloat());

    xMat4x3 rotMat;
    xMat4x3RMul3x3(&rotMat, &rotZ, &mat);

    rotMat.pos.AddScale(ray.origin, ray.dir, ray.max_t + 0.5f * emitDistance);

    xDecal::emit(mDecalEmitContext, rotMat, k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_ENV);
}

// UNUSED: needed to match (code from RotU GC)
namespace
{
    xTWEAKABLE zFrostIcicle::config defaultIceSettings = { 5.0f, 5.0f, 0.5f, 0.25f, 0.5f, 2.0f };

    F32 areaCounter;
    S32 numIcicles;

    struct icicle
    {
        xModelInstance* model;
        const xMat4x3* driver;
        const xEnt* driver_ent;
        void* initModelData;
        F32 lifetime;
        F32 scale;
        const zFrostIcicle::config* iceConfig;

        void Copy(icicle& ice)
        {
            driver = ice.driver;
            driver_ent = ice.driver_ent;
            initModelData = ice.initModelData;
            lifetime = ice.lifetime;
            scale = ice.scale;
            iceConfig = ice.iceConfig;

            xModelInstance* temp = model;
            model = ice.model;
            ice.model = temp;
        }
    };

    icicle icicleList[MAX_ICICLES];
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("cfg")
#endif

// UNUSED: needed to match (code from RotU GC)
void zFrostIcicle::Init()
{
    RpAtomic* imodel = (RpAtomic*)xSTFindAsset(HASH_iced_foe_iceblooms, NULL);

    for (S32 i = 0; i < MAX_ICICLES; i++) {
        icicleList[i].model = imodel ? xModelInstanceAlloc(imodel, NULL, 0, 0, NULL) : NULL;
        xModelInstanceCheckAlloc(icicleList[i].model, xSTAssetName(HASH_iced_foe_iceblooms));
        icicleList[i].model->modelID = HASH_iced_foe_iceblooms;
        icicleList[i].lifetime = 0.0f;
    }

    numIcicles = 0;
}

// UNUSED: needed to match (code from RotU GC)
void zFrostIcicle::DecalCreateAtomicCB(RpAtomic* atomic, const xMat4x3* driver, const xEnt* driver_ent, void* context)
{
    RpGeometry* geom = RpAtomicGetGeometry(atomic);
    xVec3* verts = (xVec3*)RpGeometryGetMorphTarget(geom, 0)->verts;
    RpTriangle* tris = RpGeometryGetTriangles(geom);
    S32 tri_size = RpGeometryGetNumTriangles(geom);

    for (S32 i = 0; i < tri_size; i++, tris++) {
        ProcessTriangle(&verts[tris->vertIndex[0]], &verts[tris->vertIndex[1]], &verts[tris->vertIndex[2]], driver,
                        driver_ent, context);
    }
}

// UNUSED: needed to match (code from RotU GC)
void zFrostIcicle::DecalCreateVertListCB(const xVec3* verts, const U16* vertIndexes, S32 vertIndexSize,
                                         const xMat4x3* driver, const xEnt* driver_ent, void* context)
{
    for (S32 i = 0; i < vertIndexSize; i += 3) {
        ProcessTriangle(&verts[vertIndexes[i]], &verts[vertIndexes[i + 1]], &verts[vertIndexes[i + 2]], driver,
                        driver_ent, context);
    }
}

// UNUSED: needed to match (code from RotU GC)
void zFrostIcicle::ProcessTriangle(const xVec3* vert0, const xVec3* vert1, const xVec3* vert2, const xMat4x3* driver,
                                   const xEnt* driver_ent, void* context)
{
    const config& iceConfig = context ? *(config*)context : defaultIceSettings;

    xVec3 v01, v02, v12, v10;
    v01.Sub(*vert1, *vert0);
    v02.Sub(*vert2, *vert0);
    v12.Sub(*vert2, *vert1);
    v10.Sub(*vert0, *vert1);

    xVec3 cross012;
    xVec3Cross(&cross012, &v10, &v12);

    areaCounter += xVec3Length(&cross012);
    S32 iceCounter = areaCounter / iceConfig.areaPerIcicle;
    areaCounter -= iceCounter * iceConfig.areaPerIcicle;

    if (iceCounter) {
        xVec3 norm;
        xVec3Normalize(&norm, &cross012);

        while (iceCounter--) {
            xVec3 pos;
            pos.AddCombine(*vert0, v01, xrand_GenRandFloat(), v02, xrand_GenRandFloat());

            xVec3 v1p;
            v1p.Sub(pos, *vert1);

            xVec3 crossp12;
            xVec3Cross(&crossp12, &v1p, &v12);

            if (xVec3Dot(&crossp12, &cross012) < 0.0f) {
                xVec3 midpoint;
                midpoint.AddCombine(*vert0, v01, 0.5f, v02, 0.5f);

                xVec3 delta;
                delta.Sub(midpoint, pos);
                pos.AddScale(delta, 2.0f);
            }

            AddIcicle(&pos, &norm, &iceConfig, driver, driver_ent);
        }
    }
}

// UNUSED: needed to match (code from RotU GC)
void zFrostIcicle::AddIcicle(const xVec3* pos, const xVec3* norm, const config* iceConfig, const xMat4x3* driver,
                             const xEnt* driver_ent)
{
    if (numIcicles < MAX_ICICLES && !driver) {
        icicle& curIce = icicleList[numIcicles];
        xModelInstance* model = curIce.model;
        xASSERT_FMT(model, "Could not find icicle model ID 0x%X", HASH_iced_foe_iceblooms);

        xModelResetMaterial(model);
        xMat4x3* frame = xModelGetFrame(model);
        xMat3x3LookVec3(*frame, *norm);
        xVec3Copy(&frame->pos, pos);
        xModelSetFrame(model, frame);

        curIce.driver = driver;
        curIce.driver_ent = driver_ent;
        curIce.lifetime = iceConfig->lifetime;
        curIce.scale = xrand_RandomFloatRange(iceConfig->minScale, iceConfig->maxScale);
        curIce.iceConfig = iceConfig;

        if (driver_ent && driver_ent->model) {
            curIce.initModelData = driver_ent->model->Data;
        }

        numIcicles++;
    }
}

// UNUSED: needed to match (code from RotU GC)
inline bool ShouldKillIcicle(icicle& curIce)
{
    if (curIce.lifetime <= 0.0f) {
        return true;
    }

    if (curIce.driver_ent && curIce.driver_ent->model) {
        if (!xEntIsVisible(curIce.driver_ent)) {
            return true;
        }

        if (curIce.initModelData != curIce.driver_ent->model->Data) {
            return true;
        }
    }

    return false;
}

// UNUSED: needed to match (code from RotU GC)
void zFrostIcicle::UpdateIcicles(F32 dt)
{
    for (S32 i = 0; i < numIcicles;) {
        icicle& curIce = icicleList[i];
        curIce.lifetime -= dt;

        if (ShouldKillIcicle(curIce)) {
            numIcicles--;
            if (i != numIcicles) {
                curIce.Copy(icicleList[numIcicles]);
            }
            icicleList[numIcicles].lifetime = 0.0f;
        } else {
            F32 curScale = curIce.scale;
            if (curIce.lifetime > curIce.iceConfig->lifetime - curIce.iceConfig->growTime) {
                curScale *= (curIce.iceConfig->lifetime - curIce.lifetime) / curIce.iceConfig->growTime;
            } else if (curIce.lifetime < curIce.iceConfig->shrinkTime) {
                curScale *= curIce.lifetime / curIce.iceConfig->shrinkTime;
            }

            xModelSetScale(curIce.model, curScale);
            xModelUpdate(curIce.model, dt);
            i++;
        }
    }
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE(
    "FX|Frost Effect|Default Icicle Config",
    "%s|Ice Density",
    "%s|Lifetime",
    "%s|Max Scale",
    "%s|Min Scale",
    "%s|Grow Time",
    "%s|Shrink Time",
    "FX|Frost Effect|Current numIcicles"
)
#endif
