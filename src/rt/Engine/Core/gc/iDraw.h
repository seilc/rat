#ifndef IDRAW_H
#define IDRAW_H

#include "types.h"

#include "xColor.h"

struct xVec3;

void iDrawBegin();
void iQueueLineList(xColor color, RwV3d* verts, U32* indices, RwMatrix* mat, U32 numVerts, U32 numIndices);
void iDrawEnd();
void iDrawShadedQuad(xColor color, RwMatrix* mat, xVec3* v1, xVec3* v2, xVec3* v3, xVec3* v4, S32 cullmode);
void iDrawSetFBMSK(U32 abgr, U32 setMask, bool);
void iXFBreset();
void* iXFBalloc(U32 size);

extern U32 lastSetFBMSK;

inline U32 iDrawGetFBMSK()
{
    return lastSetFBMSK;
}

#endif
