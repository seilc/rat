#ifndef IMODEL_H
#define IMODEL_H

#include "xColor.h"

#include <rwcore.h>
#include <rpworld.h>
#include <rphanim.h>

struct xModelTag;
struct xModelTagWithNormal;
struct xVec3;
struct xQuat;
struct xSphere;
struct xBox;

void iModelUnload(RpAtomic* userdata);
RpAtomic* iModelFile_RWMultiAtomic(RpAtomic* model);
RpHAnimHierarchy* iModelGetHierarchy(RpAtomic* imodel);
void iModelInit();
void iModelDeInit();
RpAtomic* iModelFileNew(void* buffer, U32 size);
RpAtomic* iModelFileLoad(char* filename); // UNUSED
U32 iModelNumBones(RpAtomic* model);
void iModelRender(RpAtomic* model, RwMatrix* mat);
xSphere* iModelWorldBoundSphere(RpAtomic* model, RwMatrix* mat, xSphere* sphere);
void iModelSetWorldBoundFromBox(RpAtomic* model, xBox* box);
U32 iModelVertCount(RpAtomic* model);
void iModelQuatToMat(xQuat* quat, xVec3* tran, RwMatrix* mat);
void iModelQuatFromMat(xQuat* quat, xVec3* tran, const RwMatrix* mat); // UNUSED
void iModelAnimMatrices(RpAtomic* model, xQuat* quat, xVec3* tran, RwMatrix* mat);
void iModelUpdateFrame(RpAtomic* model, RwMatrix* mat); // UNUSED
void iModelDrawWire(RpAtomic* model, RwMatrix* mat, xColor color);
#if DEBUG
void iModelDump(RpAtomic* model); // UNUSED
#endif
void iModelSetMaterialAlpha(RpAtomic* model, U8 alpha);
void iModelSetMaterialTexture(RpAtomic* model, void* texture);
void iModelResetMaterial(RpAtomic* model);
void iModelMaterialMul(RpAtomic* model, F32 rm, F32 gm, F32 bm);
void iModelMakeMaterialChangesPermanent(); // UNUSED
RwTexCoords* iModelLockAndGetTextureCoords(RpAtomic* pModel, S32* numVertices); // UNUSED
void iModelUnlock(RpAtomic* pModel); // UNUSED
U32 iModelVertEval(RpAtomic* model, U32 index, U32 count, RwMatrix* mat, xVec3* vert, xVec3* dest);
U32 iModelNormalEval(xVec3* out, const RpAtomic& m, const RwMatrix* mat, unsigned long index, S32 size,
                     const xVec3* in); // UNUSED
U32 iModelTagIndex(xModelTag* tag, RpAtomic* model, S32 v); // UNUSED
U32 iModelTagSetup(xModelTag* tag, const RpAtomic* model, F32 x, F32 y, F32 z);
U32 iModelTagSetup(xModelTagWithNormal* tag, const RpAtomic* model, F32 x, F32 y, F32 z); // UNUSED
void iModelTagEval(RpAtomic* model, const xModelTag* tag, RwMatrix* mat, xVec3* dest);
void iModelTagEval(RpAtomic* model, const xModelTagWithNormal* tag, RwMatrix* mat, xVec3* dest, xVec3* normal); // UNUSED
void iModelAnimBoundPrepare(RpAtomic* model);
void iModelAnimBoundWorld(xBox* box, xBox* combinedBox, RpAtomic* model, RwMatrix* mat, RwMatrix* bonemat);

#endif
