#ifndef IDRAW_H
#define IDRAW_H

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

// iDrawShadedQuad, iDrawShadedTri: cullmode
#define IDRAW_CULL_BACK 1
#define IDRAW_CULL_FRONT 2

void iDrawSetDisplayOffset(F32 x, F32 y);
void iDrawBillboard(xVec3* pos, U32 count, F32 dx, F32 dy, RwRaster* raster); // UNUSED
void iDrawShadedTri(xColor color, RwMatrix* mat, xVec3* v1, xVec3* v2, xVec3* v3, S32 cullmode); // UNUSED
void iXFBinit();
void iXFBflip(); // UNUSED

#endif
