#include "xModel.h"

#include "xDebug.h"
#include "xDebugTweak.h"
#include "xEnt.h"
#include "xFile.h"
#include "xGlobals.h"
#include "xLight.h"
#include "xMath.h"
#include "xMath2.h"
#include "xMath3.h"
#include "xModelAsset.h"
#include "xMemMgr.h"
#include "xMemory.h"
#include "xMorph.h"
#include "xString.h"
#include "xstransvc.h"
#include "xutil.h"
#include "iAnim.h"
#include "iCamera.h"
#include "iModel.h"
#include "iTime.h"
#include "iFile.h"

#include "decomp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if DEBUG || RELEASE
U32 poolSize[IANIM_MAX_BONES + 1];
U32 poolUsed[IANIM_MAX_BONES + 1];
#endif
RpMorphTarget anim_coll_old_mt;
static xModelPool* sxModelPoolList;
static RwCamera* subcamera;
S32 xModelPipeNumTables;
S32 xModelPipeCount[16];
xModelPipeTable* xModelPipeData[16];
S32 xModelLookupCount;
xModelPipeLookup* xModelLookupList;
S32 xModelInstStaticAlloc;
S32 xModelBucketEnabled;
S32 xModelAlphaBucketEnabled = TRUE;

#if DEBUG
U32 xModelPoolStressDisable = 1 << eModelPoolStressor_MainLoop;

void xModelPoolStressTestEnable(eModelPoolStressor stressor)
{
    xModelPoolStressDisable &= ~(1 << stressor);
}

void xModelPoolStressTestDisable(eModelPoolStressor stressor)
{
    xModelPoolStressDisable |= 1 << stressor;
}

bool xModelPoolStressTesting()
{
    return xModelPoolStressDisable == 0;
}
#endif

#if DEBUG || RELEASE
static void _xModelPipeBlendFlagsToString(U32 flag, char*& buffer); // UNUSED

DECOMP_FORCEACTIVE("Layer %i%c", "Alpha Discard %i%c");
#endif

xModelPipe modelDefaultPipe = { 0 };

xModelPipe xModelGetPipe(RpAtomic* model)
{
    for (S32 i = 0; i < xModelLookupCount; i++) {
        if (xModelLookupList[i].model == model) {
            return xModelLookupList[i].Pipe;
        }
    }
    return modelDefaultPipe;
}

static RwCamera* CameraCreate(S32 width, S32 height, S32 zBuffer);

void xModelInit()
{
#if DEBUG || RELEASE
    xModelRenderLogSetup();
#endif

    iModelInit();

    sxModelPoolList = NULL;

#if DEBUG || RELEASE
    memset(poolSize, 0, sizeof(poolSize));
    memset(poolUsed, 0, sizeof(poolUsed));
#endif

    xModelPipeNumTables = 0;
}

void xModelInitOnce()
{
    if (!subcamera) {
        subcamera = CameraCreate(0, 0, TRUE);
        xASSERT_L(329, subcamera != NULL);
    }
}

void xModelPoolInit(U32 count, U32 numMatrices)
{
    S32 i;
    U8* buffer;
    RwMatrix* mat;
    xModelPool* pool;
    xModelPool* curr;
    xModelPool** prev;
    xModelInstance* inst;

    if (numMatrices < 1) {
        numMatrices = 1;
    }

    buffer = (U8*)xMEMALLOC_L(sizeof(xModelPool) + count * numMatrices * sizeof(RwMatrix) + count * sizeof(xModelInstance),
                              16, eMemMgrTag_Model, 'XMDL', 359);

    mat = (RwMatrix*)buffer; buffer += count * numMatrices * sizeof(RwMatrix);
    pool = (xModelPool*)buffer;
    buffer += sizeof(xModelPool);
    inst = (xModelInstance*)buffer;

    for (i = 0; i < (S32)count; i++) {
        inst[i].Next = &inst[i + 1];
        inst[i].Pool = pool;
        inst[i].Mat = mat;
#if DEBUG || RELEASE
        inst[i].modelID = 0xFFFFFFFF;
#endif
        mat += numMatrices;
    }
    inst[count - 1].Next = NULL;

    pool->NumMatrices = numMatrices;
    pool->List = inst;

    prev = &sxModelPoolList;
    curr = sxModelPoolList;
    while (curr && numMatrices < curr->NumMatrices) {
        prev = &curr->Next;
        curr = curr->Next;
    }
    pool->Next = curr;
    *prev = pool;

#if DEBUG || RELEASE
    poolSize[numMatrices] += count;
#endif
}

xModelInstance* xModelInstanceAllocReuse(RpAtomic* data, void* object, U16 flags, U8 boneIndex, U8* boneRemap, xModelInstance* reuse_instance)
{
    xASSERT_L(515, data);
    S32 i;
    U32 boneCount, matCount;
    xModelPool* curr, *found = NULL;
    xModelInstance* dude;

    boneCount = iModelNumBones(data);
    matCount = boneCount + 1 + ((flags & ModelInstance_ChildTransform) != 0);

    if (reuse_instance) {
        dude = reuse_instance;
        xASSERT_L(546, dude->BoneCount + 1 + ((flags & ModelInstance_ChildTransform) != 0) <= (S32)matCount);
    } else {
        if (xModelInstStaticAlloc) {
            if (flags & 0x2000) {
                dude = (xModelInstance*)xMEMALLOC_L(sizeof(xModelInstance), 0, eMemMgrTag_Model, 'XMDL', 555);
                dude->Pool = NULL;
                dude->Mat = NULL;
            } else {
                RwMatrix* allocmats = (RwMatrix*)xMEMALLOC_L(matCount * sizeof(RwMatrix) + sizeof(xModelInstance), 16, eMemMgrTag_Model, 'XMDL', 562);
                dude = (xModelInstance*)(allocmats + matCount);
                dude->Pool = NULL;
                dude->Mat = allocmats;
            }
        } else {
            if (flags & 0x2000) {
                flags &= (U16)~0x2000;
                flags |= 0x8;
            }

            curr = sxModelPoolList;
            while (curr) {
                if (curr->List && matCount <= curr->NumMatrices) {
                    found = curr;
                }
                curr = curr->Next;
            }

            if (!found) {
                iprintf("xModelInstanceAlloc failed...\n");
                return NULL;
            }

            dude = found->List;
            found->List = dude->Next;

            xASSERT_FMT_L(597, dude->modelID == 0xffffffff, "Model from free list is not marked free - model %s", xSTAssetName(dude->modelID));

#if DEBUG || RELEASE
            poolUsed[found->NumMatrices]++;
#endif
        }
    }

    dude->Next = NULL;
    dude->Parent = NULL;
    dude->Anim = NULL;
    dude->Data = data;
    dude->Object = object;
    dude->Flags = flags | 0x1 | 0x2;
    dude->BoneCount = boneCount;
    dude->BoneIndex = boneIndex;
    dude->BoneRemap = boneRemap;
    dude->modelID = 0;
    dude->shadowID = 0xDEADBEEF;

    dude->Scale.x = 0.0f;
    dude->Scale.y = 0.0f;
    dude->Scale.z = 0.0f;

    dude->RedMultiplier = 1.0f;
    dude->GreenMultiplier = 1.0f;
    dude->BlueMultiplier = 1.0f;

    xASSERT_L(626, data->geometry);
    dude->Alpha = data->geometry->matList.materials[0]->color.alpha / 255.0f;
    dude->Surf = NULL;
    dude->FadeStart = 9e37f;
    dude->FadeEnd = 1e38f;

    if (dude->Mat) {
        for (i = 0; i < (S32)matCount; i++)
            xMat4x3Identity((xMat4x3*)&dude->Mat[i]);
    }

    dude->Bucket = xModelBucket_GetBuckets(dude->Data);
    dude->LightKit = NULL;
    if (dude->Bucket) {
        dude->Pipe = dude->Bucket[0]->Pipe;
    } else {
        dude->Pipe = xModelGetPipe(dude->Data);
    }

    if (!dude->Pipe.Layer) {
        bool forceopaque = (dude->Pipe.Flags & eFORCEOPAQUE_MASK) != 0;
        bool nosrcblend = ((dude->Pipe.Flags >> eSRCBLEND_SHIFT) & 0xf) == 0;
        bool nodestblend = ((dude->Pipe.Flags >> eDESTBLEND_SHIFT) & 0xf) == 0;
        bool noreallywewantitthisway = forceopaque || (nosrcblend && nodestblend);
        if (!noreallywewantitthisway) {
            dude->Pipe.Layer = 21;
        }
    }

    dude->anim_coll.verts = NULL;
    dude->sortBias = 0;

    return dude;
}

#if DEBUG || RELEASE
void xModelDebugPrintPoolUsage()
{
    xprintf("ModelInst Pool Usage\n\n");
    for (U32 i = 0; i < ARRAY_LENGTH(poolSize); i++) {
        if (poolSize[i]) {
            xprintf("    Bones:%d   %d/%d\n", i, poolUsed[i], poolSize[i]);
        }
    }
}
#endif

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("ModelPool:", "  %dx%d", "\n");
#endif

#define cFreeID 0xFFFFFFFF

static xModelInstance* FindChild(xModelInstance* inst)
{
    xModelInstance* curr = inst->Next;
    while (curr) {
        if (curr->Parent == inst)
            return curr;
        curr = curr->Next;
    }
    return NULL;
}

#if DEBUG || RELEASE
void xModelInstanceFreeDebug(xModelInstance* modelInst, bool animFree, const char* file, S32 line, const char* function)
#else
void xModelInstanceFree(xModelInstance* modelInst, bool animFree)
#endif
{
    xModelInstance* child;
    xModelInstance* curr;
    xModelInstance** prev;

    xASSERT_FMT_L(730, modelInst->modelID != cFreeID, "Programming Bug: Duplicate Free of Model Instance. From file %s (%d), %s", file, line, function);
    xASSERT_FMT_L(731, modelInst->Next == NULL || modelInst->Next->modelID != cFreeID, "Programming Bug: Duplicate Free of Model Instance. From file %s (%d), %s", file, line, function);
#if DEBUG || RELEASE
    if (modelInst->modelID == cFreeID) {
        return;
    }
#endif

    while (child = FindChild(modelInst)) {
#if DEBUG || RELEASE
        xModelInstanceFreeDebug(child, true, file, line, function);
#else
        xModelInstanceFree(child, true);
#endif
    }

    if (modelInst->Parent) {
        curr = modelInst->Parent->Next; prev = &modelInst->Parent->Next;
        while (curr && curr != modelInst) {
            prev = &curr->Next;
            curr = curr->Next;
        }
        xASSERT_L(765, curr);
        *prev = curr->Next;
    }

    if (modelInst->Anim && animFree) {
        xAnimPoolFree(modelInst->Anim);
        modelInst->Anim = NULL;
    }

    if (modelInst->Pool) {
        modelInst->Next = modelInst->Pool->List;
        modelInst->Pool->List = modelInst;
#if DEBUG || RELEASE
        poolUsed[modelInst->Pool->NumMatrices]--;
#endif
    }

#if DEBUG || RELEASE
    modelInst->modelID = cFreeID;
#endif
}

void xModelInstanceAttach(xModelInstance* inst, xModelInstance* parent)
{
    xModelInstance* curr;

    if (!inst || !parent)
        return;

    curr = parent;
    while (curr->Next) {
        curr = curr->Next;
    }
    curr->Next = inst;
    inst->Parent = parent;

    if (inst->Flags & 0x2000) {
        inst->Mat = parent->Mat;
    }
}

void xModelUpdate(xModelInstance* modelInst, F32 timeDelta)
{
    xSTATS_BEGIN(t0);

    while (modelInst) {
        if (modelInst->Anim && (modelInst->Flags & 0x4)) {
            if (modelInst->Flags & 0x100) {
                xAnimPlayChooseTransition(modelInst->Anim);
            }
            xAnimPlayUpdate(modelInst->Anim, timeDelta);
        }

        if (modelInst->LightKitBlend) {
            modelInst->BlendTimeRemaining -= timeDelta;
            if (modelInst->BlendTimeRemaining <= 0.0f) {
                modelInst->LightKitBlend = NULL;
            }
        }

        modelInst = modelInst->Next;
    }

    xSTATS_END(t0, anim);
}

void xModelEvalSingle(xModelInstance* modelInst, xBox* combinedAnimBound)
{
    S32 i;
    U16 flags = modelInst->Flags;

    if (flags & 0x2) {
        modelInst->Flags &= (U16)~0x1000;

        if (modelInst->Anim) {
            xAnimPlayEval(modelInst->Anim);
        }

        if (modelInst->Parent) {
            xModelInstance* dad = modelInst->Parent;

            if (flags & 0x8) {
                xASSERT_L(891, modelInst->BoneCount == dad->BoneCount);
                xMemoryCopyUpA128(modelInst->Mat, dad->Mat, (modelInst->BoneCount + 1) * sizeof(RwMatrix));
            } else if (flags & 0x10) {
                U8* remap = modelInst->BoneRemap;
                xASSERT_L(898, remap);
                xMemoryCopyUpA128(modelInst->Mat, dad->Mat, sizeof(RwMatrix));

                for (i = 0; i < modelInst->BoneCount; i++) {
                    xASSERT_L(902, remap[i]+1 < dad->BoneCount);
                    xMemoryCopyUpA128(&modelInst->Mat[i + 1], &dad->Mat[remap[i] + 1], sizeof(RwMatrix));
                }
            } else if (flags & 0x20) {
                if (dad->BoneCount && modelInst->BoneIndex) {
                    xASSERT_L(910, modelInst->BoneIndex < dad->BoneCount);
                    xMat4x3Mul((xMat4x3*)modelInst->Mat, (xMat4x3*)&dad->Mat[modelInst->BoneIndex + 1], (xMat4x3*)dad->Mat);
                } else {
                    xMemoryCopyUpA128(modelInst->Mat, dad->Mat, sizeof(RwMatrix));
                }

                if (flags & ModelInstance_ChildTransform) {
                    xMat4x3Mul((xMat4x3*)modelInst->Mat, (xMat4x3*)&modelInst->Mat[modelInst->BoneCount + 1], (xMat4x3*)modelInst->Mat);
                }
            }
        }
    }

    if ((flags & 0x1) && modelInst->BoneCount) {
        RwMatrix* modelMat = modelInst->Mat;
        if (modelInst->Scale.x != 0.0f) {
            xMat4x3& mat = *(xMat4x3*)modelMat;

            xMat3x3 temp_mat = mat;
            xMat3x3MulScale(&mat, &mat, &modelInst->Scale);
            iModelAnimBoundWorld(&modelInst->animBound, combinedAnimBound, modelInst->Data, modelMat, modelMat + 1);

            (xMat3x3&)mat = temp_mat;
        } else {
            iModelAnimBoundWorld(&modelInst->animBound, combinedAnimBound, modelInst->Data, modelMat, modelMat + 1);
        }
    }
}

void xModelEval(xModelInstance* modelInst)
{
    xSTATS_BEGIN(t0);

    xBox* combinedAnimBound = &modelInst->combinedAnimBound;
    combinedAnimBound->upper.x = -1e38f;

    while (modelInst) {
        xModelEvalSingle(modelInst, combinedAnimBound);
        modelInst = modelInst->Next;
    }

    xSTATS_END(t0, anim);
}

#if DEBUG || RELEASE
bool prelit_and_thats_the_way_we_likes_it;
U32 xModel_LastModelID;
#endif

void xModelRenderSingle(xModelInstance* modelInst)
{
#if DEBUG || RELEASE
    prelit_and_thats_the_way_we_likes_it = (modelInst->Pipe.Flags & eLIGHT_MASK) == ePIPELIGHT_KITPRELIGHT;

    if (xModelRenderLogEnabled()) {
        xModelRenderLogSetEntity((xEnt*)modelInst->Object);
    }
#endif

    if ((modelInst->Flags & 0x401) == 0x1) {
        bool reset = false;
        xMat3x3 tmpmat;

        if (modelInst->Scale.x) {
            xMat3x3Copy(&tmpmat, (xMat3x3*)modelInst->Mat);

            modelInst->Mat->right.x *= modelInst->Scale.x;
            modelInst->Mat->right.y *= modelInst->Scale.x;
            modelInst->Mat->right.z *= modelInst->Scale.x;

            modelInst->Mat->up.x *= modelInst->Scale.y;
            modelInst->Mat->up.y *= modelInst->Scale.y;
            modelInst->Mat->up.z *= modelInst->Scale.y;

            modelInst->Mat->at.x *= modelInst->Scale.z;
            modelInst->Mat->at.y *= modelInst->Scale.z;
            modelInst->Mat->at.z *= modelInst->Scale.z;
        }

        if (!xModelCullSingle(modelInst)) {
            xLightKit_Enable(*modelInst, xglobals->currWorld);

            if (modelInst->RedMultiplier != 1.0f || modelInst->GreenMultiplier != 1.0f || modelInst->BlueMultiplier != 1.0f) {
                reset = true;
                xModelMaterialMul(modelInst, modelInst->RedMultiplier, modelInst->GreenMultiplier, modelInst->BlueMultiplier);
            }

            U8 alpha = 255.0f * modelInst->Alpha;

            if (alpha != 255) {
                reset = true;
                xModelSetMaterialAlpha(modelInst, alpha);
            }

            if (modelInst->Flags & 0x80) {
                xAnimPlay* a = modelInst->Anim;

                for (U16 i = 0; i < a->NumSingle; i++)
                    if (a->Single[i].SingleFlags & 0x8000) {
                        if (a->Single[i].State) {
                            xMorphRender(*(xMorphSeqFile**)a->Single[i].State->Data->RawData, modelInst->Mat, xAnimFileRawTime(a->Single[i].State->Data, a->Single[i].Time));
                        }
                        break;
                    }
            }
#if DEBUG
            else if (modelInst->Flags & 0x200)
                iModelDrawWire(modelInst->Data, modelInst->Mat, g_WHITE);
#endif
            else {
#if DEBUG || RELEASE
                xModel_LastModelID = modelInst->modelID;
                xModelInstance* it = modelInst->Parent;
                while (!xModel_LastModelID && it) {
                    xModel_LastModelID = it->modelID;
                    it = it->Parent;
                }
#endif

                xLightAddLocal(modelInst);
                iModelRender(modelInst->Data, modelInst->Mat);
                xLightRemoveLocal(modelInst);
            }
        }

        if (reset) {
            xModelResetMaterial(modelInst);
        }

        if (modelInst->Scale.x) {
            xMat3x3Copy((xMat3x3*)modelInst->Mat, &tmpmat);
        }
    }

#if DEBUG || RELEASE
    prelit_and_thats_the_way_we_likes_it = false;
#endif
}

void xModelRender(xModelInstance* modelInst)
{
    xSTATS_BEGIN(t0);

#if DEBUG
    static U32 check_model = 0;
    if (check_model) {
        const char* model_name = xSTAssetNameFromPointer(modelInst->Data);
        if (model_name && xStrHash(model_name) == check_model) {
            S32 asdf = 1234;
        }
    }

    static U32 check_ent = 0;
    if (check_ent) {
        if (modelInst->Object && ((xEnt*)modelInst->Object)->id == check_ent) {
            S32 asdf = 1234;
        }
    }
#endif

    while (modelInst) {
        if (xModelBucketEnabled)
            xModelBucket_Add(modelInst, false);
        else
            xModelRenderSingle(modelInst);
        modelInst = modelInst->Next;
    }

    xSTATS_END(t0, model_render);
}

// UNUSED: needed to match (speculative code)
void xModelRender2D(xModelInstance* modelInst, basic_rect<F32>* sr, xVec3* from, xVec3* to, F32)
{
    RwCamera* camera;
    RwFrame* frame;
    RwMatrix* cammat;
    RwMatrix objmat, shearmat, tmpmat;
    const RwV2d* camvw;
    F32 viewscale, shearX, shearY;

    if (sr->x + sr->w < 0.0f || sr->y + sr->h < 0.0f || sr->x > FB_XRES - 5 || sr->y > FB_YRES - 5 || sr->w == 0.0f || sr->h == 0.0f) {
        return;
    }

    camera = RwCameraGetCurrentCamera();
    frame = RwCameraGetFrame(camera);
    cammat = RwFrameGetLTM(frame);

    xMat3x3LookAt((xMat3x3*)&objmat, to, from);
    xMat3x3Transpose((xMat3x3*)&objmat, (xMat3x3*)&objmat);
    objmat.pos.x = 0.0f;
    objmat.pos.y = 0.0f;
    objmat.pos.z = xVec3Dist(to, from);
    objmat.flags = 0;

    camvw = RwCameraGetViewWindow(camera);

    viewscale = camvw->x * sr->w * (1.0f / 320.0f);
    shearX = camvw->x * -((sr->x + 0.5f * sr->w - 320.0f) / 320.0f);
    shearY = camvw->y * -((sr->y + 0.5f * sr->h - 240.0f) / 240.0f);

    shearmat.right.x = viewscale;
    shearmat.right.y = 0.0f;
    shearmat.right.z = 0.0f;
    shearmat.up.x = 0.0f;
    shearmat.up.y = 448.0f * viewscale / 480.0f;
    shearmat.up.z = 0.0f;
    shearmat.at.x = shearX;
    shearmat.at.y = shearY;
    shearmat.at.z = 1.0f;
    shearmat.pos.x = 0.0f;
    shearmat.pos.y = 0.0f;
    shearmat.pos.z = 0.0f;
    shearmat.flags = 0;

    xMat4x3Mul((xMat4x3*)&tmpmat, (xMat4x3*)&objmat, (xMat4x3*)&shearmat);

    RwMatrix tmp;
    xMat4x3Mul((xMat4x3*)&tmp, (xMat4x3*)&tmpmat, (xMat4x3*)cammat);
    xMat4x3Mul((xMat4x3*)&objmat, (xMat4x3*)modelInst->Mat, (xMat4x3*)&tmp);

    RwMatrix tmp2;
    tmp2 = *modelInst->Mat;
    *modelInst->Mat = objmat;
    iModelRender(modelInst->Data, modelInst->Mat);
    *modelInst->Mat = tmp2;
}

void xModelRender2D(const xModelInstance& model, const basic_rect<F32>& r, const xVec3& from, const xVec3& to)
{
    RwCamera* camera;
    RwFrame* frame;
    RwMatrix* cammat;
    xMat4x3 objmat, shearmat, temp1, temp2;
    const RwV2d* camvw;
    F32 viewscale, shearX, shearY;
    xVec3 vscale;
    xMat4x3 objmat1;
    if (r.w <= 0.0f || r.h <= 0.0f || r.x + r.w < 0.0f || r.x > 1.0f || r.y + r.h < 0.0f || r.y > 1.0f) {
        return;
    }

    camera = RwCameraGetCurrentCamera();
    frame = RwCameraGetFrame(camera);
    cammat = RwFrameGetLTM(frame);

    xMat3x3LookAt(&objmat, &to, &from);
    xMat3x3Transpose(&objmat, &objmat);
    objmat.pos.x = 0.0f;
    objmat.pos.y = 0.0f;
    objmat.pos.z = xVec3Dist(&to, &from);
    objmat.flags = 0;

    camvw = RwCameraGetViewWindow(camera);

    viewscale = 2.0f * (camvw->x * r.w);
    shearX = camvw->x * -(2.0f * r.x + r.w - 1.0f);
    shearY = camvw->y * -(2.0f * r.y + r.h - 1.0f);

    shearmat.right.x = viewscale;
    shearmat.right.y = 0.0f;
    shearmat.right.z = 0.0f;
    shearmat.up.x = 0.0f;
    shearmat.up.y = viewscale;
    shearmat.up.z = 0.0f;
    shearmat.at.x = shearX;
    shearmat.at.y = shearY;
    shearmat.at.z = 1.0f;
    shearmat.pos.x = 0.0f;
    shearmat.pos.y = 0.0f;
    shearmat.pos.z = 0.0f;
    shearmat.flags = 0;

    xMat4x3Mul(&temp1, &objmat, &shearmat);
    xMat4x3Mul(&temp2, &temp1, (xMat4x3*)cammat);
    xMat4x3Mul(&objmat, (xMat4x3*)model.Mat, &temp2);

    if (model.Scale.x != 0.0f) {
        vscale.x = model.Scale.x;
        vscale.y = model.Scale.y;
        vscale.z = model.Scale.z;
        xASSERT_MSG_L(1409, !xfeq0(model.Scale.y) && !xfeq0(model.Scale.z), "Scales cannot be equal to 0 for HUD elements");
    } else {
        vscale.assign(1.0f, 1.0f, 1.0f);
    }

    xMat4x3Scale(&objmat1, &vscale);
    xMat4x3Mul(&objmat1, &objmat1, &objmat);

    temp1 = *(xMat4x3*)model.Mat;
    *model.Mat = *(RwMatrix*)&objmat1;
    iModelRender(model.Data, model.Mat);
    *model.Mat = *(RwMatrix*)&temp1;
}

static void CameraDestroy(RwCamera* camera);

static RwCamera* CameraCreate(S32 width, S32 height, S32 zBuffer)
{
    RwCamera* camera;

    camera = RwCameraCreateWrapper();
    if (camera) {
        RwCameraSetFrame(camera, RwFrameCreate());
        RwCameraSetRaster(camera, RwRasterCreate(width, height, 0, rwRASTERTYPECAMERA));

        if (zBuffer) {
            RwCameraSetZRaster(camera, RwRasterCreate(width, height, 0, rwRASTERTYPEZBUFFER));
        }

        if (RwCameraGetFrame(camera) && RwCameraGetRaster(camera) && (!zBuffer || RwCameraGetZRaster(camera))) {
            return camera;
        }
    }

    CameraDestroy(camera);

    return NULL;
}

static void CameraDestroy(RwCamera* camera)
{
    if (camera) {
        RwRaster* raster;

        _rwFrameSyncDirty();

        RwFrame* frame = RwCameraGetFrame(camera);
        if (frame) {
            RwCameraSetFrame(camera, NULL);

            RwFrameDestroy(frame);
        }

        raster = RwCameraGetRaster(camera);
        if (raster) {
            RwRasterDestroy(raster);

            RwCameraSetRaster(camera, NULL);
        }

        raster = RwCameraGetZRaster(camera);
        if (raster) {
            RwRasterDestroy(raster);

            RwCameraSetZRaster(camera, NULL);
        }

        RwCameraDestroy(camera);
    }
}

static void CameraSetViewWindow(RwCamera* camera, F32 width, F32 height, F32 vw); // UNUSED

void xModel_SceneEnter(RpWorld* world)
{
    xASSERT_L(1606, world);

    RpWorldAddCamera(world, subcamera);
}

void xModel_SceneExit(RpWorld* world)
{
    xASSERT_L(1620, world);

    RpWorldRemoveCamera(world, subcamera);

    iModelDeInit();
}

void xModelAnimCollStart(xModelInstance& m)
{
    m.Flags = (m.Flags & ~0x1000) | 0x800;
    if (m.anim_coll.verts)
        return;

    U32 size = iModelVertCount(m.Data);
    if (size) {
        m.anim_coll.verts = (xVec3*)xMEMALLOC_L(size * sizeof(xVec3), 0, eMemMgrTag_Model, 'XMDL', 1637);
        xASSERT_L(1638, m.anim_coll.verts != 0);
    }
}

void xModelAnimCollRefresh(const xModelInstance& cm)
{
    xModelInstance& m = (xModelInstance&)cm;
    xASSERT_L(1645, xModelAnimCollDirty(m));
    U32 size = iModelVertCount(m.Data);
    xMat4x3& mat = *(xMat4x3*)m.Mat;
    xMat4x3 old_mat = mat;
    mat = g_I3;
    xASSERT_L(1650, m.anim_coll.verts != 0);
    iModelVertEval(m.Data, 0, size, m.Mat, NULL, m.anim_coll.verts);
    mat = old_mat;
    m.Flags |= 0x1000;
}

void xModelGetBoneLocation(xVec3& loc, const xModelInstance& model, size_t index)
{
    xASSERT_L(1658, index <= model.BoneCount);

    xMat4x3& root_mat = *(xMat4x3*)model.Mat;
    xMat4x3& anim_mat = *(xMat4x3*)(model.Mat + index);
    if (index == 0)
        loc = root_mat.pos;
    else
        xMat4x3ToWorld(&loc, &root_mat, &anim_mat.pos);
}

void xModelGetBoneMat(xMat4x3& mat, const xModelInstance& model, size_t index)
{
    xASSERT_L(1682, index <= model.BoneCount);
    const xMat4x3& root_mat = *(const xMat4x3*)model.Mat;
    if (index == 0) mat = root_mat;
    else {
        const xMat4x3& anim_mat = *(const xMat4x3*)(model.Mat + index);
        xMat4x3Mul(&mat, &anim_mat, &root_mat);
    }
}

void xModelGetBoneLocationScaled(xVec3& loc, const xModelInstance& model, size_t index)
{
    xASSERT_L(1710, index <= model.BoneCount);
    xMat4x3 temp_mat;
    const xMat4x3* root_mat = (const xMat4x3*)model.Mat;
    if (model.Scale.x != 0.0f) {
        xMat3x3MulScaleC(&temp_mat, root_mat, model.Scale.x, model.Scale.y, model.Scale.z);
        temp_mat.pos = root_mat->pos;
        root_mat = &temp_mat;
    }

    if (index == 0) loc = root_mat->pos;
    else {
        const xMat4x3& anim_mat = *(const xMat4x3*)(model.Mat + index);
        xMat4x3ToWorld(&loc, root_mat, &anim_mat.pos);
    }
}

S32 xModelSphereCull(xSphere* sphere)
{
    if (sphere == NULL) {
        return TRUE;
    }

    return RwCameraFrustumTestSphere(RwCameraGetCurrentCamera(), (RwSphere*)sphere) == rwSPHEREOUTSIDE;
}

S32 xModelSphereCullPlusShadow(xSphere* sphere, xVec3* shadowVec, S32* shadowOutside)
{
    RwCamera* cam = RwCameraGetCurrentCamera();
    RwSphere* worldsph = (RwSphere*)sphere;
    const RwFrustumPlane* frustumPlane;
    S32 numPlanes;

    frustumPlane = cam->frustumPlanes;
    numPlanes = 6;
    while (numPlanes--) {
        F32 nDot = RwV3dDotProductMacro(&worldsph->center, &frustumPlane->plane.normal);
        nDot -= frustumPlane->plane.distance;

        if (nDot > worldsph->radius) {
            goto outside;
        }

        frustumPlane++;
    }

    *shadowOutside = FALSE;
    return FALSE;

outside:
    F32 nDot, sDot;

    sDot = RwV3dDotProductMacro((RwV3d*)shadowVec, &frustumPlane->plane.normal);
    sDot -= frustumPlane->plane.distance;
    if (sDot > worldsph->radius) {
        *shadowOutside = TRUE;
        return TRUE;
    }

    frustumPlane++;
    while (numPlanes--) {
        nDot = RwV3dDotProductMacro(&worldsph->center, &frustumPlane->plane.normal);
        nDot -= frustumPlane->plane.distance;
        sDot = RwV3dDotProductMacro((RwV3d*)shadowVec, &frustumPlane->plane.normal);
        sDot -= frustumPlane->plane.distance;
        if (nDot > worldsph->radius && sDot > worldsph->radius) {
            *shadowOutside = TRUE;
            return TRUE;
        }
        frustumPlane++;
    }

    *shadowOutside = FALSE;
    return TRUE;
}

S32 xModelBoxCull(xBox* box)
{
    const RwFrustumPlane* frustumPlane;
    S32 numPlanes;
    RwFrustumTestResult result;
    RwCamera* camera = RwCameraGetCurrentCamera();

    F32 centerX = 0.5f * (box->upper.x + box->lower.x);
    F32 centerY = 0.5f * (box->upper.y + box->lower.y);
    F32 centerZ = 0.5f * (box->upper.z + box->lower.z);
    F32 sizeX = 0.5f * (box->upper.x - box->lower.x);
    F32 sizeY = 0.5f * (box->upper.y - box->lower.y);
    F32 sizeZ = 0.5f * (box->upper.z - box->lower.z);
    xASSERT_L(1872, (sizeX >= 0.0f) && (sizeY >= 0.0f) && (sizeZ >= 0.0f));

    result = rwSPHEREINSIDE;

    frustumPlane = camera->frustumPlanes;
    numPlanes = 6;
    while (numPlanes--) {
        F32 centerDot = centerX * frustumPlane->plane.normal.x + centerY * frustumPlane->plane.normal.y +
                        centerZ * frustumPlane->plane.normal.z;
        centerDot -= frustumPlane->plane.distance;
        F32 nearDot = xabs(sizeX * frustumPlane->plane.normal.x) + xabs(sizeY * frustumPlane->plane.normal.y) +
                      xabs(sizeZ * frustumPlane->plane.normal.z);

        if (centerDot > nearDot) {
            result = rwSPHEREOUTSIDE;
            break;
        }

        if (centerDot > -nearDot) {
            result = rwSPHEREBOUNDARY;
        }

        frustumPlane++;
    }

    return result == rwSPHEREOUTSIDE;
}

S32 xModelBoxCullPlusShadow(xBox* box, xVec3* shadowVec, S32* shadowOutside)
{
    const RwFrustumPlane* frustumPlane;
    S32 numPlanes;
    RwCamera* camera = RwCameraGetCurrentCamera();
    F32 nearDot;

    if (box->upper.x == -1e38f) {
        return TRUE;
    }

    F32 centerX = 0.5f * (box->upper.x + box->lower.x);
    F32 centerY = 0.5f * (box->upper.y + box->lower.y);
    F32 centerZ = 0.5f * (box->upper.z + box->lower.z);
    F32 sizeX = 0.5f * (box->upper.x - box->lower.x);
    F32 sizeY = 0.5f * (box->upper.y - box->lower.y);
    F32 sizeZ = 0.5f * (box->upper.z - box->lower.z);
    xASSERT_L(1939, (sizeX >= 0.0f) && (sizeY >= 0.0f) && (sizeZ >= 0.0f));

    frustumPlane = camera->frustumPlanes;
    numPlanes = 6;
    while (numPlanes--) {
        F32 centerDot = centerX * frustumPlane->plane.normal.x + centerY * frustumPlane->plane.normal.y +
                        centerZ * frustumPlane->plane.normal.z;
        centerDot -= frustumPlane->plane.distance;
        nearDot = xabs(sizeX * frustumPlane->plane.normal.x) + xabs(sizeY * frustumPlane->plane.normal.y) +
                  xabs(sizeZ * frustumPlane->plane.normal.z);

        if (centerDot > nearDot) {
            goto outside;
        }

        frustumPlane++;
    }

    *shadowOutside = FALSE;
    return FALSE;

outside:
    F32 shadowDot = RwV3dDotProductMacro(shadowVec, &frustumPlane->plane.normal);
    shadowDot -= frustumPlane->plane.distance;
    if (shadowDot > nearDot) {
        *shadowOutside = TRUE;
        return TRUE;
    }

    frustumPlane++;
    while (numPlanes--) {
        F32 centerDot = centerX * frustumPlane->plane.normal.x + centerY * frustumPlane->plane.normal.y +
                        centerZ * frustumPlane->plane.normal.z;
        centerDot -= frustumPlane->plane.distance;
        shadowDot = RwV3dDotProductMacro(shadowVec, &frustumPlane->plane.normal);
        shadowDot -= frustumPlane->plane.distance;
        nearDot = xabs(sizeX * frustumPlane->plane.normal.x) + xabs(sizeY * frustumPlane->plane.normal.y) +
                  xabs(sizeZ * frustumPlane->plane.normal.z);

        if (centerDot > nearDot && shadowDot > nearDot) {
            *shadowOutside = TRUE;
            return TRUE;
        }

        frustumPlane++;
    }

    *shadowOutside = FALSE;
    return TRUE;
}

#if DEBUG || RELEASE
S32 BoxEnable = TRUE;
#else
static const S32 BoxEnable = TRUE;
#endif

S32 xModelCullGroup(xModelInstance* minst)
{
    if (BoxEnable && minst->BoneCount) {
        return xModelBoxCull(&minst->combinedAnimBound);
    }

    return xModelSphereCull(iModelWorldBoundSphere(minst->Data, minst->Mat, NULL));
}

S32 xModelCullGroupPlusShadow(xModelInstance* minst, F32 shadowDepth, S32* shadowOutside)
{
    xVec3 shadVec;

    if (BoxEnable && minst->BoneCount) {
        xBox* bbox = &minst->combinedAnimBound;
        shadVec.x = 0.5f * (bbox->upper.x + bbox->lower.x);
        shadVec.y = minst->Mat->pos.y + 0.5f * (bbox->upper.x - bbox->lower.x) - shadowDepth;
        shadVec.z = 0.5f * (bbox->upper.z + bbox->lower.z);
        return xModelBoxCullPlusShadow(&minst->combinedAnimBound, &shadVec, shadowOutside);
    }

    shadVec.x = minst->Mat->pos.x;
    shadVec.y = minst->Mat->pos.y - 10.0f;
    shadVec.z = minst->Mat->pos.z;
    return xModelSphereCullPlusShadow(iModelWorldBoundSphere(minst->Data, minst->Mat, NULL), &shadVec, shadowOutside);
}

S32 xModelCullSingle(xModelInstance* minst)
{
    if (BoxEnable && minst->BoneCount) {
        S32 result = xModelBoxCull(&minst->animBound);
        if (!result) {
            iModelSetWorldBoundFromBox(minst->Data, &minst->animBound);
        }

        return result;
    }

    return xModelSphereCull(iModelWorldBoundSphere(minst->Data, minst->Mat, NULL));
}

void xModelSetLightKit(xModelInstance* minst, xLightKit* lightKit, F32 transitionTime)
{
    while (minst) {
        if ((minst->Pipe.Flags & eLIGHT_MASK) != ePIPELIGHT_PRELIGHTONLY) {
            if (transitionTime) {
                minst->LightKitBlend = minst->LightKit;
                minst->BlendDuration = minst->BlendTimeRemaining = transitionTime;
            } else {
                minst->LightKitBlend = NULL;
            }

            minst->LightKit = lightKit;
        } else {
            minst->LightKit = NULL;
            minst->LightKitBlend = NULL;
        }

        minst = minst->Next;
    }
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("in->geometry->triangles", "ostream != 0", "istream != 0", "result != 0", "*out != 0");
#endif

bool xModelIsPreinstanced(const xModelInstance* model)
{
    RpGeometry* geom = RpAtomicGetGeometry(model->Data);
    if (!geom) return true;
    RpMorphTarget* mt = RpGeometryGetMorphTarget(geom, 0);
    return !mt || !RpMorphTargetGetVertices(mt);
}

bool xModelGetPipe(U32 modelID, U32 atomicNumber, xModelPipe** pipe)
{
    S32 j, k;
    U32 subAtomicBits;

    *pipe = NULL;
    if (!xModelPipeData) {
        return false;
    }

    for (j = 0; j < xModelPipeNumTables; j++) {
        for (k = 0; k < xModelPipeCount[j]; k++) {
            if (modelID == xModelPipeData[j][k].ModelHashID || modelID == xStrHashCat(xModelPipeData[j][k].ModelHashID, ".dff")) {
                subAtomicBits = xModelPipeData[j][k].SubObjectBits;
                if (subAtomicBits == 0xFFFFFFFF) {
                    *pipe = &xModelPipeData[j][k].Pipe;
                    return true;
                }

                if (subAtomicBits & (1 << atomicNumber)) {
                    *pipe = &xModelPipeData[j][k].Pipe;
                    return true;
                }
            }
        }
    }
    return false;
}

namespace {

RpAtomic* find_first_atomic(xModelAssetInfo* minf)
{
    xASSERT_L(2183, minf != 0);

    xModelAssetInst* inst = (xModelAssetInst*)(minf + 1);
    xModelAssetInst* end_inst = inst + minf->NumModelInst;
    for (; inst != end_inst; inst++) {
        if (!inst->ModelID)
            continue;

        void* data = xSTFindAsset(inst->ModelID, NULL);
        if (!data)
            continue;

        if (*(U32*)data == 'FNIM') {
            RpAtomic* child_atomic = find_first_atomic((xModelAssetInfo*)data);
            if (child_atomic)
                return child_atomic;
        } else
            return (RpAtomic*)data;
    }

    return NULL;
}

}

RpAtomic* xModelFindAtomic(U32 id, U32* actual_id)
{
    RpAtomic* atomic = NULL;
    U32 aid = id;

    if (id) {
        atomic = (RpAtomic*)xSTFindAsset(aid, NULL);
        if (!atomic) {
            aid = xStrHashCat(id, ".dff");
            atomic = (RpAtomic*)xSTFindAsset(aid, NULL);
            if (!atomic) {
                aid = xStrHashCat(id, ".minf");
                xModelAssetInfo* minf = (xModelAssetInfo*)xSTFindAsset(aid, NULL);
                if (minf)
                    atomic = find_first_atomic(minf);
            }
        }
    }

    if (actual_id)
        *actual_id = !atomic ? id : aid;
    return atomic;
}

#if DEBUG || RELEASE
bool render_log_enabled;
xEnt* render_log_entity;

namespace {

const char* render_log_output_file = "RENDERLOG.TXT";
bool render_log_output_to_file = true;

enum
{
    RL_SORT_NONE,
    RL_SORT_MODEL,
    RL_SORT_ENTITY,
    MAX_RL_SORT
};

const S32 MAX_RENDER_LOG = 1000;

struct render_log_entry
{
    char model_name[128];
    char ent_name[128];
    bool cloned;
    S32 vertices;
    S32 triangles;
};

render_log_entry* render_log;
S32 render_log_size;
S32 render_log_sort_by;

struct render_output_entry
{
    char* text[2];
    render_log_entry* log_entry;
};

class log_stream
{
public:
    virtual void write(const char* text) = 0;
};

class file_log_stream : public log_stream
{
public:
    xFile& file;

    file_log_stream(xFile& file) : file(file) {}

    virtual void write(const char* text)
    {
        iFileWrite(&file, (void*)text, strlen(text));
    }
};

class print_log_stream : public log_stream
{
public:
    virtual void write(const char* text)
    {
        printf(text);
    }
};

S32 compare_log_text(const void* e1, const void* e2)
{
    const render_output_entry& r1 = *(const render_output_entry*)e1;
    const render_output_entry& r2 = *(const render_output_entry*)e2;
    S32 c0 = stricmp(r1.text[0], r2.text[0]);
    if (c0)
        return c0;
    return stricmp(r1.text[1], r2.text[1]);
}

char* print_spaced(char* out, const char* s0, S32 min_len0, const char* s1, S32 min_len1)
{
    S32 len0 = strlen(s0), len1 = strlen(s1);
    strcpy(out, s0);
    min_len0 -= len0;
    out += len0;
    for (; min_len0 > 0; min_len0--)
        *out++ = ' ';
    strcpy(out, s1);
    min_len1 -= len1;
    out += len1;
    for (; min_len1 > 0; min_len1--)
        *out++ = ' ';
    *out = '\0';
    return out;
}

void dump_log(log_stream& os)
{
    S32 log_size = MIN(render_log_size, MAX_RENDER_LOG);

    render_output_entry* output_entry_buffer = (render_output_entry*)RwMallocL((log_size + 1) * sizeof(render_output_entry), 0, 2337);
    render_output_entry* output_entries = output_entry_buffer + 1;

    switch (render_log_sort_by) {
    case RL_SORT_ENTITY:
        output_entry_buffer->text[0] = "Entity";
        output_entry_buffer->text[1] = "Model";
        for (S32 i = 0; i < log_size; i++) {
            output_entries[i].text[0] = render_log[i].ent_name;
            output_entries[i].text[1] = render_log[i].model_name;
        }
        break;
    case RL_SORT_MODEL:
    default:
        output_entry_buffer->text[0] = "Model";
        output_entry_buffer->text[1] = "Entity";
        for (S32 i = 0; i < log_size; i++) {
            output_entries[i].text[0] = render_log[i].model_name;
            output_entries[i].text[1] = render_log[i].ent_name;
        }
    }

    output_entry_buffer->log_entry = NULL;
    for (S32 i = 0; i < log_size; i++)
        output_entries[i].log_entry = &render_log[i];

    if (render_log_sort_by != RL_SORT_NONE) {
        qsort(output_entries, log_size, sizeof(render_output_entry), compare_log_text);
    }

    char buffer[256];

    sprintf(buffer, "Rendered %d models, listing %d: \r\n", render_log_size, log_size);
    os.write(buffer);

    S32 max_len0 = 0, max_len1 = 0;
    for (S32 i = -1; i < log_size; i++) {
        const render_output_entry& entry = output_entries[i];
        S32 len0 = strlen(entry.text[0]);
        if (max_len0 < len0)
            max_len0 = len0;
        S32 len1 = strlen(entry.text[1]);
        if (max_len1 < len1)
            max_len1 = len1;
    }

    {
        const render_output_entry& entry = output_entries[-1];
        max_len0 += 5;
        max_len1 += 5;
        char* end = print_spaced(buffer, entry.text[0], max_len0, entry.text[1], max_len1);
        strcpy(end, " Vertices");
        end += strlen(end);
        strcpy(end, " Triangles");
        end += strlen(end);
        strcpy(end, " Cloned");
        end += strlen(end);
        strcpy(end, "\r\n");
        os.write(buffer);
    }

    for (S32 i = 0; i < log_size; i++) {
        const render_output_entry& entry = output_entries[i];
        char* end = print_spaced(buffer, entry.text[0], max_len0, entry.text[1], max_len1);
        sprintf(end, "% *d", 9, entry.log_entry->vertices);
        end += strlen(end);
        sprintf(end, "% *d", 10, entry.log_entry->triangles);
        end += strlen(end);
        sprintf(end, "% *s", 7, entry.log_entry->cloned ? "YES" : "NO");
        end += strlen(end);
        strcpy(end, "\r\n");
        os.write(buffer);
    }

    RwFree(output_entry_buffer);
}

void on_dump_log(const tweak_info& info)
{
    const char* filename = (const char*)info.context;
    render_log_output_to_file = filename != NULL;
    render_log_enabled = true;
}

}

void xModelRenderLogSetup()
{
    render_log = new (eMemTweakType_0, eMemMgrTag_Model) render_log_entry[MAX_RENDER_LOG];
    render_log_enabled = false;

    const char* prefix = "Debugging Options|Render Logging|";
    static char* sort_by_labels[] = { "Render Order", "Model", "Entity" };
    static const tweak_callback cb_dump_log = { on_dump_log };

    xAUTOTWEAK_SELECT(prefix, "Sort By", &render_log_sort_by, (const char**)sort_by_labels, (S32*)NULL, MAX_RL_SORT, NULL, NULL, 0, false);
    xAUTOTWEAK_MESSAGE(prefix, "Print Log", "Go!", &cb_dump_log, NULL, 0, false);
    xAUTOTWEAK_MESSAGE(prefix, "Write Log To", render_log_output_file, &cb_dump_log, (void*)render_log_output_file, 0, false);
}

void xModelRenderLogBegin()
{
    render_log_enabled = true;
    render_log_entity = NULL;
    render_log_size = 0;
}

void xModelRenderLogEnd()
{
    if (!render_log_enabled) {
        return;
    }

    if (render_log_output_to_file) {
        S32 flags = 0xE;
        xFile file;

        if (iFileOpen(render_log_output_file, flags, &file))
            return;

        file_log_stream os(file);
        dump_log(os);
        iFileClose(&file);
    } else {
        print_log_stream os;
        dump_log(os);
    }

    render_log_enabled = false;
}

void xModelRenderLogAdd(RpAtomic* model, bool cloned)
{
    if (render_log_size >= MAX_RENDER_LOG) {
        render_log_size++;
        return;
    }

    render_log_entry& entry = render_log[render_log_size];

    const char* model_name = xSTAssetName(model);
    if (!model_name)
        model_name = xSTAssetNameFromPointer(model);
    strcpy(entry.model_name, !model_name ? "<Unknown>" : model_name);

    const char* ent_name = !render_log_entity ? NULL : xSTAssetName(render_log_entity);
    strcpy(entry.ent_name, !ent_name ? "<Unknown>" : ent_name);

    entry.cloned = cloned;

    RpGeometry* geom = RpAtomicGetGeometry(model);
    if (geom) {
        entry.vertices = RpGeometryGetNumVertices(geom);
        entry.triangles = RpGeometryGetNumTriangles(geom);
    } else {
        entry.vertices = 0;
        entry.triangles = 0;
    }

    render_log_size++;
    render_log_entity = NULL;
}
#endif

S32 xModelGetNumAtomics(RpAtomic& model)
{
    S32 numAtomics = 1;

    RpAtomic* myModel = &model;
    while ((myModel = iModelFile_RWMultiAtomic(myModel)) != NULL) {
        numAtomics++;
    }
    return numAtomics;
}
