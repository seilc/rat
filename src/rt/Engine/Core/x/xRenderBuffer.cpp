#include "xRenderBuffer.h"

#include "xMemMgr.h"
#include "iCamera.h"

tagiRenderArrays gRenderArr;
tagiRenderInput gRenderBuffer;
i2dQuadBuffer* gQuadBuffer;

void xRenderBufferInit()
{
    gRenderBuffer.m_mode = 0;
    gRenderBuffer.m_indexCount = 0;
    gRenderBuffer.m_vertexCount = 0;
    gRenderBuffer.m_vertexTypeSize = sizeof(RwIm3DVertex);
    gRenderBuffer.m_index = gRenderArr.m_index;
    gRenderBuffer.m_vertex = gRenderArr.m_vertex;
    gRenderBuffer.m_vertexTZ = gRenderArr.m_vertexTZ;
    gRenderBuffer.m_vertex_2d = gRenderArr.m_vertex_2d;

    RwIm2DVertex nullVert = {};
    RwIm2DVertexSetScreenZ(&nullVert, sCameraNearClip);
    RwIm2DVertexSetIntRGBA(&nullVert, 255, 255, 255, 255);

    for (S32 i = 0; i < IRENDER_NUM_VERTEX_2D; i++) {
        gRenderBuffer.m_vertex_2d[i] = nullVert;
    }

    gQuadBuffer = new (eMemStaticType_0, eMemMgrTag_RenderBuffer) i2dQuadBuffer(gRenderBuffer.m_vertex_2d, IRENDER_NUM_VERTEX_2D);
}
