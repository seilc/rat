#ifndef XMODEL_H
#define XMODEL_H

#include "iModel.h"
#include "xAnim.h"
#include "xModelBucket.h"
#include "xLightKit.h"

#include <rpworld.h>

struct xSurface;
struct xLight;

struct xModelInstance;
struct xModelPool;

#if DEBUG || RELEASE
extern bool prelit_and_thats_the_way_we_likes_it;
#endif

struct xModelInstance
{
    xModelInstance* Next;
    xModelInstance* Parent;
    xModelPool* Pool;
    xAnimPlay* Anim;
    RpAtomic* Data;
    xModelPipe Pipe;
    U8 InFrustum;
    U8 TrueClip;
    S8 sortBias;
    U8 modelpad;
    F32 RedMultiplier;
    F32 GreenMultiplier;
    F32 BlueMultiplier;
    F32 Alpha;
    F32 FadeStart;
    F32 FadeEnd;
    xSurface* Surf;
    xModelBucket** Bucket;
    xModelInstance* BucketNext;
    xLightKit* LightKit;
    xLightKit* LightKitBlend;
    F32 BlendDuration;
    F32 BlendTimeRemaining;
    void* Object;
    U16 Flags;
    U8 BoneCount;
    U8 BoneIndex;
    U8* BoneRemap;
    RwMatrix* Mat;
    xVec3 Scale;
    xBox animBound;
    xBox combinedAnimBound;
    U32 modelID;
    U32 shadowID;
    struct
    {
        xVec3* verts;
    } anim_coll;
    xLight* lights[4];
    F32 lightsDistance[4];
};

struct xModelPool
{
    xModelPool* Next;
    U32 NumMatrices;
    xModelInstance* List;
};

struct xModelTag
{
    xVec3 v;
    U32 matidx;
    F32 wt[4];
};

xModelInstance* xModelInstanceAllocReuse(RpAtomic* data, void* object, U16 flags, U8 boneIndex, U8* boneRemap, xModelInstance* reuse_instance);
#if DEBUG || RELEASE
void xModelInstanceFreeDebug(xModelInstance* modelInst, bool animFree, const char* file, S32 line, const char* function);
#define xModelInstanceFree(modelInst, animFree) xModelInstanceFreeDebug(modelInst, animFree, __FILE__, __LINE__, __FUNCTION__)
#else
void xModelInstanceFree(xModelInstance* modelInst, bool animFree);
#endif
void xModelUpdate(xModelInstance* modelInst, F32 timeDelta);
void xModelEval(xModelInstance* modelInst);
void xModelAnimCollRefresh(const xModelInstance& cm);
S32 xModelCullSingle(xModelInstance* minst);
bool xModelIsPreinstanced(const xModelInstance* model);
void xModelRender(xModelInstance* modelInst);
void xModelSetLightKit(xModelInstance* minst, xLightKit* lightKit, F32 transitionTime);
RpAtomic* xModelFindAtomic(U32 id, U32* actual_id);

inline xMat4x3* xModelGetFrame(xModelInstance* model)
{
    xASSERT_L(855, model);
    xASSERT_L(856, model->Mat);
    return (xMat4x3*)model->Mat;
}

bool xModelAnimCollDirty(const xModelInstance& m);

inline xModelInstance* xModelInstanceAlloc(RpAtomic* data, void* object, U16 flags, U8 boneIndex, U8* boneRemap)
{
    return xModelInstanceAllocReuse(data, object, flags, boneIndex, boneRemap, NULL);
}

inline void xModelSetMaterialAlpha(xModelInstance* modelInst, U8 alpha)
{
    xASSERT_L(1064, modelInst);
    iModelSetMaterialAlpha(modelInst->Data, alpha);
}

inline void xModelMaterialMul(xModelInstance* modelInst, F32 rm, F32 gm, F32 bm)
{
    xASSERT_L(1076, modelInst);
    xASSERT_L(1077, modelInst->Data);
    iModelMaterialMul(modelInst->Data, rm, gm, bm);
}

inline void xModelResetMaterial(xModelInstance* modelInst)
{
    xASSERT_L(1085, modelInst);
    xASSERT_L(1086, modelInst->Data);
    iModelResetMaterial(modelInst->Data);
}

#if DEBUG || RELEASE
extern bool render_log_enabled;
extern xEnt* render_log_entity;

inline bool xModelRenderLogEnabled()
{
    return render_log_enabled;
}

inline void xModelRenderLogSetEntity(xEnt* ent)
{
    render_log_entity = ent;
}
#endif

#endif
