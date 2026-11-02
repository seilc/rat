#ifndef IMODEL_H
#define IMODEL_H

#include "types.h"

#include <rwcore.h>
#include <rpworld.h>
#include <rphanim.h>

struct xModelTag;
struct xVec3;
struct xQuat;
struct xSphere;
struct xBox;

void iModelUnload(RpAtomic* userdata);
RpAtomic* iModelFile_RWMultiAtomic(RpAtomic* model);
RpHAnimHierarchy* iModelGetHierarchy(RpAtomic* imodel);
U32 iModelNumBones(RpAtomic* model);
void iModelRender(RpAtomic* model, RwMatrix* mat);
xSphere* iModelWorldBoundSphere(RpAtomic* model, RwMatrix* mat, xSphere* sphere);
void iModelSetWorldBoundFromBox(RpAtomic* model, xBox* box);
U32 iModelVertCount(RpAtomic* model);
void iModelAnimMatrices(RpAtomic* model, xQuat* quat, xVec3* tran, RwMatrix* mat);
void iModelSetMaterialAlpha(RpAtomic* model, U8 alpha);
void iModelResetMaterial(RpAtomic* model);
void iModelMaterialMul(RpAtomic* model, F32 rm, F32 gm, F32 bm);
U32 iModelVertEval(RpAtomic* model, U32 index, U32 count, RwMatrix* mat, xVec3* vert, xVec3* dest);
void iModelTagEval(RpAtomic* model, const xModelTag* tag, RwMatrix* mat, xVec3* dest);
void iModelAnimBoundWorld(xBox* box, xBox* combinedBox, RpAtomic* model, RwMatrix* mat, RwMatrix* bonemat);

#endif
