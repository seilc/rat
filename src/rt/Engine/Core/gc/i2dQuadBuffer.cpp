#include "xCommon.h"
#include "i2dQuadBuffer.h"

bool i2dQuadBuffer::Flush()
{
    S32 num_quads = vertsUsed / 4;
    if (num_quads == 0) {
        return true;
    }

    F32 ProjectionMatrix[GX_PROJECTION_SZ];
    GXGetProjectionv(ProjectionMatrix);

    _rw2DRenderPrimitiveInit();

    GXBegin(GX_QUADS, GX_VTXFMT0, vertsUsed);

    RwIm2DVertex* verts = vertArray;
    RwIm2DVertex* end_marker = verts + vertsUsed;
    xASSERT_L(50, (vertsUsed % 4) == 0);

    while (verts != end_marker) {
        U32 color = (verts[0].r << 24) | (verts[0].g << 16) | (verts[0].b << 8) | verts[0].a;

        GXPosition3f32(verts[0].x, verts[0].y, verts[0].z);
        GXColor1u32(color);
        GXTexCoord2f32(verts[0].u, verts[0].v);

        GXPosition3f32(verts[1].x, verts[1].y, verts[1].z);
        GXColor1u32(color);
        GXTexCoord2f32(verts[1].u, verts[1].v);

        GXPosition3f32(verts[2].x, verts[2].y, verts[2].z);
        GXColor1u32(color);
        GXTexCoord2f32(verts[2].u, verts[2].v);

        GXPosition3f32(verts[3].x, verts[3].y, verts[3].z);
        GXColor1u32(color);
        GXTexCoord2f32(verts[3].u, verts[3].v);

        verts += 4;
    }
    GXEnd();

    GXSetProjectionv(ProjectionMatrix);

    vertsUsed = 0;
    return true;
}
