#ifndef IMODEL_H
#define IMODEL_H

#include "types.h"

#include <rwcore.h>
#include <rpworld.h>
#include <rphanim.h>

struct xModelTag;
struct xVec3;

RpHAnimHierarchy* iModelGetHierarchy(RpAtomic* imodel);
U32 iModelNumBones(RpAtomic* model);
void iModelRender(RpAtomic* model, RwMatrix* mat);
void iModelSetMaterialAlpha(RpAtomic* model, U8 alpha);
void iModelResetMaterial(RpAtomic* model);
void iModelMaterialMul(RpAtomic* model, F32 rm, F32 gm, F32 bm);
U32 iModelVertEval(RpAtomic* model, U32 index, U32 count, RwMatrix* mat, xVec3* vert, xVec3* dest);
void iModelTagEval(RpAtomic* model, const xModelTag* tag, RwMatrix* mat, xVec3* dest);

#endif
