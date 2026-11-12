#ifndef XIM_H
#define XIM_H

#include "xMath3.h"
#include "xMath2.h"
#include "xColor.h"

enum xIMFormat
{
    XIM_TRISTRIP = 4,
    XIM_TRILIST = 3
};

struct xIMLockContext
{
    void* data;
    xVec3* pos;
    xVec2* uv;
    xColor* color;
    S32 vertCount;
    xIMFormat format;
};

struct xIMLockParameters
{
    S32 flags;
    const xVec3* cpos;
    const xVec2* cuv;
    const xColor* ccolor;
};

void xIMLock(xIMLockContext& context, xIMFormat format, S32 vertCount);
void xIMLock(xIMLockContext& context, xIMFormat format, S32 vertCount, const xIMLockParameters& params); // UNUSED
void xIMLock2(xIMLockContext& context1, xIMLockContext& context2, xIMFormat format, S32 vertCount, const xIMLockParameters& params1, const xIMLockParameters& params2); // UNUSED
void xIMUnlock(xIMLockContext& context);
void xIMRender(xIMLockContext& context, const xMat4x3* mat, U32 flags);
void xIM2DRenderQuad(F32 x1, F32 y1, F32 x2, F32 y2, F32 uMax, F32 vMax, U32 alpha, U32 r, U32 g, U32 b);

inline void xIMRenderLegacy(const RwIm3DVertex* verts, U32 count, const RwMatrix* ltm, RwPrimitiveType primType)
{
    if (RwIm3DTransform((RwIm3DVertex*)verts, count, (RwMatrix*)ltm, rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA)) {
        RwIm3DRenderPrimitive(primType);
        RwIm3DEnd();
    }
}

inline void xIMRenderIndexedLegacy(const RwIm3DVertex* verts, U32 count, const RwMatrix* ltm, RwPrimitiveType primType,
                                   const RwImVertexIndex* index, S32 index_size)
{
    if (RwIm3DTransform((RwIm3DVertex*)verts, count, (RwMatrix*)ltm, rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA)) {
        RwIm3DRenderIndexedPrimitive(primType, (RwImVertexIndex*)index, index_size);
        RwIm3DEnd();
    }
}

#endif
