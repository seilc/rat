#include "xIM.h"

#include "xDebug.h"
#include "xMemMgr.h"

#include <rwcore.h>

void xIMLock(xIMLockContext& context, xIMFormat format, S32 vertCount)
{
    xASSERT(9, vertCount > 0);
    xASSERT(10, !((format == XIM_TRILIST) && ((vertCount % 3) != 0)));

    S32 pos_qwords = (vertCount * sizeof(xVec3) + (16-1)) / 16;
    S32 uv_qwords = (vertCount * sizeof(xVec2) + (16-1)) / 16;
    S32 color_qwords = (vertCount * sizeof(xColor) + (16-1)) / 16;
    S32 alloc_qwords = pos_qwords + uv_qwords + color_qwords;

    context.data = xMEMPUSHTEMP((alloc_qwords*16), 0, 18);

    U8* data_buffer = (U8*)context.data;

    context.pos = (xVec3*)data_buffer;
    data_buffer += pos_qwords*16;

    context.uv = (xVec2*)data_buffer;
    data_buffer += uv_qwords*16;

    context.color = (xColor*)data_buffer;
    data_buffer += color_qwords*16;

    xASSERT(28, ((U32)data_buffer - (U32)context.data) == (alloc_qwords*16));

    context.vertCount = vertCount;
    context.format = format;
}

#ifndef NON_MATCHING
void xIMLock(xIMLockContext&, xIMFormat, S32, const xIMLockParameters& params)
{
    xASSERT(0, params.cpos != 0);
    xASSERT(0, params.cuv != 0);
    xASSERT(0, params.ccolor != 0);
}

void xIMLock2(xIMLockContext& context1, xIMLockContext& context2, xIMFormat, S32, const xIMLockParameters& params1, const xIMLockParameters& params2)
{
    xASSERT(0, (params1.cpos != 0) && (((int)params1.cpos & 15) == 0));
    xASSERT(0, (params1.cuv != 0) && (((int)params1.cuv & 15) == 0));
    xASSERT(0, (params1.ccolor != 0) && (((int)params1.ccolor & 15) == 0));
    xASSERT(0, (params2.cpos != 0) && (((int)params2.cpos & 15) == 0));
    xASSERT(0, (params2.cuv != 0) && (((int)params2.cuv & 15) == 0));
    xASSERT(0, (params2.ccolor != 0) && (((int)params2.ccolor & 15) == 0));

    U8* data_buffer = NULL;
    S32 alloc_qwords = 0;
    xASSERT(0, ((U32)data_buffer - (U32)context1.data) == (alloc_qwords*16));
}
#endif

void xIMUnlock(xIMLockContext& context)
{
    if (context.data) {
        xMEMPOPTEMP(context.data);
    }
}

void xIMRender(xIMLockContext& context, const xMat4x3* mat, U32 flags)
{
    S32 verts_size = context.vertCount;
    const xVec3* pos = context.pos;
    const xVec2* uv = context.uv;
    const xColor* color = context.color;

    if (flags == 0) {
        flags = (rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA);
    }

    RwIm3DVertex* vert_buffer = (RwIm3DVertex*)xMEMPUSHTEMP(verts_size * sizeof(RwIm3DVertex), 0, 225);

    RwIm3DVertex* vert = vert_buffer;
    RwIm3DVertex* end_vert = vert + verts_size;
    while (vert != end_vert) {
        RwV3d tmp;
        tmp.x = pos->x;
        tmp.y = pos->y;
        tmp.z = pos->z;
        RwIm3DVertexSetPos(vert, tmp.x, tmp.y, tmp.z);
        RwIm3DVertexSetUV(vert, uv->x, uv->y);
        if (flags & 0x10) {
            RwRGBA col;
            col.red = color->r;
            col.green = color->g;
            col.blue = color->b;
            col.alpha = color->a;
            RwIm3DVertexSetRGBA(vert, col.red, col.green, col.blue, col.alpha);
        }
        color++;
        pos++;
        uv++;
        vert++;
    }
    
    RwMatrix* m = (RwMatrix*)mat;
    RwIm3DTransform(vert_buffer, verts_size, m, flags);
    RwIm3DRenderPrimitive((RwPrimitiveType)context.format);
    RwIm3DEnd();

    xMEMPOPTEMP(vert_buffer);
}

void xIM2DRenderQuad(F32 x1, F32 y1, F32 x2, F32 y2, F32 uMax, F32 vMax, U32 alpha, U32 r, U32 g, U32 b)
{
    F32 z = RwIm2DGetNearScreenZ();
    F32 recipCamZ = 1.0f / RwCameraGetNearClipPlane(RwCameraGetCurrentCamera());

    static RwIm2DVertex vx[4];
    
    RwIm2DVertexSetScreenX(&vx[0], x1);
    RwIm2DVertexSetScreenY(&vx[0], y1);
    RwIm2DVertexSetScreenZ(&vx[0], z);
    RwIm2DVertexSetIntRGBA(&vx[0], r, g, b, alpha);
    RwIm2DVertexSetU(&vx[0], 0.0f, recipCamZ);
    RwIm2DVertexSetV(&vx[0], 0.0f, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[1], x1);
    RwIm2DVertexSetScreenY(&vx[1], y2);
    RwIm2DVertexSetScreenZ(&vx[1], z);
    RwIm2DVertexSetIntRGBA(&vx[1], r, g, b, alpha);
    RwIm2DVertexSetU(&vx[1], 0.0f, recipCamZ);
    RwIm2DVertexSetV(&vx[1], vMax, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[2], x2);
    RwIm2DVertexSetScreenY(&vx[2], y1);
    RwIm2DVertexSetScreenZ(&vx[2], z);
    RwIm2DVertexSetIntRGBA(&vx[2], r, g, b, alpha);
    RwIm2DVertexSetU(&vx[2], uMax, recipCamZ);
    RwIm2DVertexSetV(&vx[2], 0.0f, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[3], x2);
    RwIm2DVertexSetScreenY(&vx[3], y2);
    RwIm2DVertexSetScreenZ(&vx[3], z);
    RwIm2DVertexSetIntRGBA(&vx[3], r, g, b, alpha);
    RwIm2DVertexSetU(&vx[3], uMax, recipCamZ);
    RwIm2DVertexSetV(&vx[3], vMax, recipCamZ);

    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, vx, 4);
}
