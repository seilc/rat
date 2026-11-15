#include "xCommon.h"
#include "iParMgr.h"

#include "xCam.h"
#include "xRenderBuffer.h"
#include "xRenderState.h"
#include "xstransvc.h"
#include "zGlobals.h"
#include "zParSys.h"

#include <string.h>

static S32 gColorTableInit;
static F32 gColorTable[256];

void iParMgrInit()
{
    if (!gColorTableInit) {
        for (S32 i = 0; i < 256; i++)
            gColorTable[i] = i / 255.0f;

        gColorTableInit = TRUE;
    }
}

void iParMgrUpdate(F32)
{
}

void iParMgrRenderParSys_Sprite(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;

    iRenderSetCameraViewMatrix(NULL);

    zParSys* s = (zParSys*)data;
    RwTexture* texture = (RwTexture*)xSTFindAsset(s->tasset->textureID, NULL);
    if (texture) {
        RwRaster* raster = RwTextureGetRaster(texture);
        if (raster) {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
        }
    }

    xASSERT_L(312, gRenderBuffer.m_indexCount == 0);
    xASSERT_L(313, gRenderBuffer.m_vertexCount == 0);

    S32 indexCount = 0, vertexCount = 0;
    U16* i3d = gRenderBuffer.m_index;
    RwIm3DVertex* v3d = gRenderBuffer.m_vertex;
    xParCmdTex* tex = ps->m_cmdTex;

    while (idx) {
        xPar* p = idx;
        idx = idx->m_next;

        F32 size = 0.5f * p->m_size;
        xVec3 h, v;

        h.x = size * gRenderBuffer.m_camViewR.x;
        h.y = size * gRenderBuffer.m_camViewR.y;
        h.z = size * gRenderBuffer.m_camViewR.z;
        v.x = size * gRenderBuffer.m_camViewU.x;
        v.y = size * gRenderBuffer.m_camViewU.y;
        v.z = size * gRenderBuffer.m_camViewU.z;

        U8 r = p->m_c[0], g = p->m_c[1], b = p->m_c[2], a = p->m_c[3];
        xVec3 vert[4];

        vert[0].x = p->m_pos.x - h.x - v.x;
        vert[0].y = p->m_pos.y - h.y - v.y;
        vert[0].z = p->m_pos.z - h.z - v.z;

        vert[1].x = p->m_pos.x + h.x + v.x;
        vert[1].y = p->m_pos.y + h.y + v.y;
        vert[1].z = p->m_pos.z + h.z + v.z;

        vert[2].x = p->m_pos.x + h.x - v.x;
        vert[2].y = p->m_pos.y + h.y - v.y;
        vert[2].z = p->m_pos.z + h.z - v.z;

        vert[3].x = p->m_pos.x - h.x + v.x;
        vert[3].y = p->m_pos.y - h.y + v.y;
        vert[3].z = p->m_pos.z - h.z + v.z;

        RwIm3DVertexSetRGBA(&v3d[0], r, g, b, a);
        RwIm3DVertexSetRGBA(&v3d[1], r, g, b, a);
        RwIm3DVertexSetRGBA(&v3d[2], r, g, b, a);
        RwIm3DVertexSetRGBA(&v3d[3], r, g, b, a);

        RwIm3DVertexSetPos(&v3d[0], vert[0].x, vert[0].y, vert[0].z);
        RwIm3DVertexSetPos(&v3d[1], vert[1].x, vert[1].y, vert[1].z);
        RwIm3DVertexSetPos(&v3d[2], vert[2].x, vert[2].y, vert[2].z);
        RwIm3DVertexSetPos(&v3d[3], vert[3].x, vert[3].y, vert[3].z);

        if (tex) {
            F32 u1, u2, v1, v2;

            u1 = p->m_texIdx[0] * tex->unit_width + tex->x1;
            v1 = p->m_texIdx[1] * tex->unit_height + tex->y1;

            u2 = (p->m_texIdx[0] + 1) * tex->unit_width + tex->x1;
            v2 = (p->m_texIdx[1] + 1) * tex->unit_height + tex->y1;

            RwIm3DVertexSetUV(&v3d[0], u1, v1);
            RwIm3DVertexSetUV(&v3d[1], u2, v2);
            RwIm3DVertexSetUV(&v3d[2], u2, v1);
            RwIm3DVertexSetUV(&v3d[3], u1, v2);
        } else {
            RwIm3DVertexSetUV(&v3d[0], 0.0f, 0.0f);
            RwIm3DVertexSetUV(&v3d[1], 1.0f, 1.0f);
            RwIm3DVertexSetUV(&v3d[2], 1.0f, 0.0f);
            RwIm3DVertexSetUV(&v3d[3], 0.0f, 1.0f);
        }

        i3d[0] = vertexCount, i3d[1] = vertexCount + 1, i3d[2] = vertexCount + 2,
            i3d[3] = vertexCount + 3, i3d[4] = vertexCount, i3d[5] = vertexCount + 1;

        i3d += 6;
        v3d += 4;
        vertexCount += 4;
        indexCount += 6;

        if (indexCount > IRENDER_NUM_INDEX - 6 || vertexCount > IRENDER_NUM_VERTEX - 4) {
            gRenderBuffer.m_indexCount = indexCount;
            gRenderBuffer.m_vertexCount = vertexCount;
            i3d = gRenderBuffer.m_index;
            v3d = gRenderBuffer.m_vertex;
            iRenderFlush();
            indexCount = 0;
            vertexCount = 0;
        }
    }

    gRenderBuffer.m_indexCount = indexCount;
    gRenderBuffer.m_vertexCount = vertexCount;
    iRenderFlush();
}

void iRenderSetCameraViewMatrix(xMat4x3* m)
{
    if (!m && globals.screen->icam) {
        RwMatrix* mat = RwFrameGetMatrix(RwCameraGetFrame(RwCameraGetCurrentCamera()));
        gRenderBuffer.m_camViewMatrix = *(xMat4x3*)mat;
    } else {
        xASSERT_L(456, m);
        gRenderBuffer.m_camViewMatrix = *m;
    }

    xVec3Inv((xVec3*)&gRenderBuffer.m_camViewR, &gRenderBuffer.m_camViewMatrix.right);
    xVec3Inv((xVec3*)&gRenderBuffer.m_camViewU, &gRenderBuffer.m_camViewMatrix.up);
}

void iRenderPushQuadStreak(xPar* p, xParCmdTex* tex)
{
    static RwIm3DVertex v3d[4];
    static U16 i3d[6] = { 0, 1, 2, 0, 2, 3 };

    void* vertices = v3d;
    U16* indices = i3d;
    xVec3 h;
    S32 vertexCount = 4, indexCount = 6;

    if (indexCount + gRenderBuffer.m_indexCount > IRENDER_NUM_INDEX) {
        iRenderFlush();
    }

    if (vertexCount + gRenderBuffer.m_vertexCount > IRENDER_NUM_VERTEX) {
        iRenderFlush();
    }

    U8 r = p->m_c[0], g = p->m_c[1], b = p->m_c[2], a = p->m_c[3];
    F32 size = p->m_size;
    xVec3 vert[4];

    h.x = size * gRenderBuffer.m_camViewR.x;
    h.y = size * gRenderBuffer.m_camViewR.y;
    h.z = size * gRenderBuffer.m_camViewR.z;

    vert[0].x = p->m_pos.x;
    vert[0].y = p->m_pos.y;
    vert[0].z = p->m_pos.z;

    vert[3].x = vert[0].x;
    vert[3].y = vert[0].y;
    vert[3].z = vert[0].z;

    vert[1].x = p->m_pos.x - 5.0f * p->m_vel.x;
    vert[1].y = p->m_pos.y - 5.0f * p->m_vel.y;
    vert[1].z = p->m_pos.z - 5.0f * p->m_vel.z;

    vert[2].x = vert[1].x;
    vert[2].y = vert[1].y;
    vert[2].z = vert[1].z;

    vert[0].x -= h.x;
    vert[0].y -= h.y;
    vert[0].z -= h.z;

    vert[1].x -= h.x;
    vert[1].y -= h.y;
    vert[1].z -= h.z;

    vert[2].x += h.x;
    vert[2].y += h.y;
    vert[2].z += h.z;

    vert[3].x += h.x;
    vert[3].y += h.y;
    vert[3].z += h.z;

    RwIm3DVertexSetRGBA(&v3d[0], r, g, b, a);
    RwIm3DVertexSetRGBA(&v3d[1], r, g, b, a);
    RwIm3DVertexSetRGBA(&v3d[2], r, g, b, a);
    RwIm3DVertexSetRGBA(&v3d[3], r, g, b, a);

    RwIm3DVertexSetPos(&v3d[0], vert[0].x, vert[0].y, vert[0].z);
    RwIm3DVertexSetPos(&v3d[1], vert[1].x, vert[1].y, vert[1].z);
    RwIm3DVertexSetPos(&v3d[2], vert[2].x, vert[2].y, vert[2].z);
    RwIm3DVertexSetPos(&v3d[3], vert[3].x, vert[3].y, vert[3].z);

    if (tex) {
        F32 u1, u2, v1, v2;

        u1 = p->m_texIdx[0] * tex->unit_width + tex->x1;
        v1 = p->m_texIdx[1] * tex->unit_height + tex->y1;

        u2 = (p->m_texIdx[0] + 1) * tex->unit_width + tex->x1;
        v2 = (p->m_texIdx[1] + 1) * tex->unit_height + tex->y1;

        RwIm3DVertexSetUV(&v3d[0], u1, v2);
        RwIm3DVertexSetUV(&v3d[1], u1, v1);
        RwIm3DVertexSetUV(&v3d[2], u2, v1);
        RwIm3DVertexSetUV(&v3d[3], u2, v2);
    } else {
        RwIm3DVertexSetUV(&v3d[0], 0.0f, 1.0f);
        RwIm3DVertexSetUV(&v3d[1], 0.0f, 0.0f);
        RwIm3DVertexSetUV(&v3d[2], 1.0f, 0.0f);
        // BUG: sets v3d[2].v, not v3d[3].v
        v3d[3].u = 0.0f; v3d[2].v = 1.0f;
    }

    U16* src = indices;
    U16* dst = gRenderBuffer.m_index + gRenderBuffer.m_indexCount;

    for (S32 i = 0; i < indexCount; i++) {
        *dst++ = gRenderBuffer.m_vertexCount + *src++;
    }

    memcpy((U8*)gRenderBuffer.m_vertex + gRenderBuffer.m_vertexTypeSize * gRenderBuffer.m_vertexCount, vertices,
           gRenderBuffer.m_vertexTypeSize * vertexCount);

    gRenderBuffer.m_indexCount += indexCount;
    gRenderBuffer.m_vertexCount += vertexCount;
}

static void iRenderPushFlat(xPar* p, xParCmdTex* tex)
{
    static RwIm3DVertex v3d[4];
    static U16 i3d[6] = { 0, 1, 2, 0, 2, 3 };

    void* vertices = v3d;
    U16* indices = i3d;
    S32 vertexCount = 4, indexCount = 6;

    if (indexCount + gRenderBuffer.m_indexCount > IRENDER_NUM_INDEX) {
        iRenderFlush();
    }

    if (vertexCount + gRenderBuffer.m_vertexCount > IRENDER_NUM_VERTEX) {
        iRenderFlush();
    }

    U8 r = p->m_c[0], g = p->m_c[1], b = p->m_c[2], a = p->m_c[3];
    F32 size = 0.5f * p->m_size;
    xVec3 vert[4];
    xMat3x3 groundmat;
    F32 yaw = 0.0f;

    if (p->m_rotdeg[0]) {
        yaw = 2.0f * PI * (p->m_rotdeg[0] / 255.0f);
    }

    xMat3x3Euler(&groundmat, yaw, 0.0f, 0.0f);

    xVec3 zdir, xdir, centre;

    zdir.x = groundmat.at.x * size;
    zdir.y = groundmat.at.y * size;
    zdir.z = groundmat.at.z * size;

    xdir.x = groundmat.right.x * size;
    xdir.y = groundmat.right.y * size;
    xdir.z = groundmat.right.z * size;

    centre.x = p->m_pos.x;
    centre.y = p->m_pos.y;
    centre.z = p->m_pos.z;

    vert[0].x = centre.x - xdir.x - zdir.x;
    vert[0].y = centre.y;
    vert[0].z = centre.z - xdir.z - zdir.z;

    vert[1].x = centre.x + xdir.x - zdir.x;
    vert[1].y = centre.y;
    vert[1].z = centre.z + xdir.z - zdir.z;

    vert[2].x = centre.x + xdir.x + zdir.x;
    vert[2].y = centre.y;
    vert[2].z = centre.z + xdir.z + zdir.z;

    vert[3].x = centre.x - xdir.x + zdir.x;
    vert[3].y = centre.y;
    vert[3].z = centre.z - xdir.z + zdir.z;

    RwIm3DVertexSetRGBA(&v3d[0], r, g, b, a);
    RwIm3DVertexSetRGBA(&v3d[1], r, g, b, a);
    RwIm3DVertexSetRGBA(&v3d[2], r, g, b, a);
    RwIm3DVertexSetRGBA(&v3d[3], r, g, b, a);

    RwIm3DVertexSetPos(&v3d[0], vert[0].x, vert[0].y, vert[0].z);
    RwIm3DVertexSetPos(&v3d[1], vert[1].x, vert[1].y, vert[1].z);
    RwIm3DVertexSetPos(&v3d[2], vert[2].x, vert[2].y, vert[2].z);
    RwIm3DVertexSetPos(&v3d[3], vert[3].x, vert[3].y, vert[3].z);

    if (tex) {
        F32 u1, u2, v1, v2;

        u1 = p->m_texIdx[0] * tex->unit_width + tex->x1;
        v1 = p->m_texIdx[1] * tex->unit_height + tex->y1;

        u2 = (p->m_texIdx[0] + 1) * tex->unit_width + tex->x1;
        v2 = (p->m_texIdx[1] + 1) * tex->unit_height + tex->y1;

        RwIm3DVertexSetUV(&v3d[0], u1, v1);
        RwIm3DVertexSetUV(&v3d[1], u2, v1);
        RwIm3DVertexSetUV(&v3d[2], u2, v2);
        RwIm3DVertexSetUV(&v3d[3], u1, v2);
    } else {
        RwIm3DVertexSetUV(&v3d[0], 0.0f, 0.0f);
        RwIm3DVertexSetUV(&v3d[1], 1.0f, 0.0f);
        RwIm3DVertexSetUV(&v3d[2], 1.0f, 1.0f);
        RwIm3DVertexSetUV(&v3d[3], 0.0f, 1.0f);
    }

    U16* src = indices;
    U16* dst = gRenderBuffer.m_index + gRenderBuffer.m_indexCount;

    for (S32 i = 0; i < indexCount; i++) {
        *dst++ = gRenderBuffer.m_vertexCount + *src++;
    }

    memcpy((U8*)gRenderBuffer.m_vertex + gRenderBuffer.m_vertexTypeSize * gRenderBuffer.m_vertexCount, vertices,
           gRenderBuffer.m_vertexTypeSize * vertexCount);

    gRenderBuffer.m_indexCount += indexCount;
    gRenderBuffer.m_vertexCount += vertexCount;
}

void iRenderFlush()
{
    if (gRenderBuffer.m_vertexCount > 0) {
        iRenderTrianglesImmediate(gRenderBuffer.m_vertexType, gRenderBuffer.m_vertexTypeSize, gRenderBuffer.m_vertex,
                                  gRenderBuffer.m_vertexCount, gRenderBuffer.m_index, gRenderBuffer.m_indexCount);
    }

    gRenderBuffer.m_vertexCount = 0;
    gRenderBuffer.m_indexCount = 0;
}

void iRenderTrianglesImmediate(S32, S32, void* vertices, S32 vertexCount, U16* indices, S32 indexCount)
{
    U32 transformFlags = rwIM3D_VERTEXUV;

    if (RwIm3DTransform((RwIm3DVertex*)vertices, vertexCount, NULL, transformFlags)) {
        if (indexCount) {
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, indices, indexCount);
        } else {
            RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
        }

        RwIm3DEnd();
    }
}

void iParMgrRenderParSys_Streak(void* data, xParGroup* ps)
{
    U32 transformFlags;
    xPar* idx = ps->m_root;

    iRenderSetCameraViewMatrix(NULL);

    zParSys* s = (zParSys*)data;
    RwTexture* texture = (RwTexture*)xSTFindAsset(s->tasset->textureID, NULL);
    if (texture) {
        RwRaster* raster = RwTextureGetRaster(texture);
        if (raster) {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
        }
    } else {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    }

    transformFlags = rwIM3D_VERTEXUV;
    RwIm3DVertex* v3d = gRenderBuffer.m_vertex;

    while (idx) {
        xPar* p = idx;
        void* vertices = v3d;
        S32 vertexCount = 3;
        xVec3 h;
        F32 size = p->m_size;

        h.x = size * gRenderBuffer.m_camViewR.x;
        h.y = size * gRenderBuffer.m_camViewR.y;
        h.z = size * gRenderBuffer.m_camViewR.z;

        xVec3 vert[3];

        vert[0].x = p->m_pos.x;
        vert[0].y = p->m_pos.y;
        vert[0].z = p->m_pos.z;

        vert[1].x = p->m_pos.x - 5.0f * p->m_vel.x;
        vert[1].y = p->m_pos.y - 5.0f * p->m_vel.y;
        vert[1].z = p->m_pos.z - 5.0f * p->m_vel.z;

        vert[2].x = vert[1].x;
        vert[2].y = vert[1].y;
        vert[2].z = vert[1].z;

        vert[1].x -= h.x;
        vert[1].y -= h.y;
        vert[1].z -= h.z;

        vert[2].x += h.x;
        vert[2].y += h.y;
        vert[2].z += h.z;

        RwIm3DVertexSetPos(&v3d[0], vert[2].x, vert[2].y, vert[2].z);
        RwIm3DVertexSetPos(&v3d[1], vert[1].x, vert[1].y, vert[1].z);
        RwIm3DVertexSetPos(&v3d[2], vert[0].x, vert[0].y, vert[0].z);

        U8 r = p->m_c[0];
        U8 g = p->m_c[1];
        U8 b = p->m_c[2];
        U8 a = p->m_c[3];

        RwIm3DVertexSetRGBA(&v3d[0], r, g, b, a);
        RwIm3DVertexSetRGBA(&v3d[1], r, g, b, a);
        RwIm3DVertexSetRGBA(&v3d[2], r, g, b, a);

        RwIm3DVertexSetUV(&v3d[2], 0.5f, 1.0f);
        RwIm3DVertexSetUV(&v3d[1], 0.0f, 0.0f);
        RwIm3DVertexSetUV(&v3d[0], 1.0f, 0.0f);

        v3d += 3;
        gRenderBuffer.m_vertexCount += vertexCount;

        if (vertexCount + gRenderBuffer.m_vertexCount > IRENDER_NUM_VERTEX - 3) {
            iRenderFlush();
            v3d = gRenderBuffer.m_vertex;
        }

        idx = idx->m_next;
    }

    iRenderFlush();
}

void iParMgrRenderParSys_InvStreak(void* data, xParGroup* ps)
{
    U32 transformFlags;
    xPar* idx = ps->m_root;

    iRenderSetCameraViewMatrix(NULL);

    zParSys* s = (zParSys*)data;
    RwTexture* texture = (RwTexture*)xSTFindAsset(s->tasset->textureID, NULL);
    if (texture) {
        RwRaster* raster = RwTextureGetRaster(texture);
        if (raster) {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
        }
    } else {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    }

    transformFlags = rwIM3D_VERTEXUV;
    RwIm3DVertex* v3d = gRenderBuffer.m_vertex;

    while (idx) {
        xPar* p = idx;
        void* vertices = v3d;
        S32 vertexCount = 3;
        xVec3 h;
        F32 size = p->m_size;

        h.x = size * gRenderBuffer.m_camViewR.x;
        h.y = size * gRenderBuffer.m_camViewR.y;
        h.z = size * gRenderBuffer.m_camViewR.z;

        xVec3 vert[3];

        vert[0].x = p->m_pos.x;
        vert[0].y = p->m_pos.y;
        vert[0].z = p->m_pos.z;

        vert[2].x = vert[1].x = p->m_pos.x - 5.0f * p->m_vel.x;
        vert[2].y = vert[1].y = p->m_pos.y - 5.0f * p->m_vel.y;
        vert[2].z = vert[1].z = p->m_pos.z - 5.0f * p->m_vel.z;

        vert[1].x -= h.x;
        vert[1].y -= h.y;
        vert[1].z -= h.z;

        vert[2].x += h.x;
        vert[2].y += h.y;
        vert[2].z += h.z;

        RwIm3DVertexSetPos(&v3d[0], vert[2].x, vert[2].y, vert[2].z);
        RwIm3DVertexSetPos(&v3d[1], vert[1].x, vert[1].y, vert[1].z);
        RwIm3DVertexSetPos(&v3d[2], vert[0].x, vert[0].y, vert[0].z);

        U8 r = p->m_c[0];
        U8 g = p->m_c[1];
        U8 b = p->m_c[2];
        U8 a = p->m_c[3];

        RwIm3DVertexSetRGBA(&v3d[0], r, g, b, a);
        RwIm3DVertexSetRGBA(&v3d[1], r, g, b, a);
        RwIm3DVertexSetRGBA(&v3d[2], r, g, b, a);

        RwIm3DVertexSetUV(&v3d[2], 0.5f, 1.0f);
        RwIm3DVertexSetUV(&v3d[1], 0.0f, 0.0f);
        RwIm3DVertexSetUV(&v3d[0], 1.0f, 0.0f);

        v3d += 3;
        gRenderBuffer.m_vertexCount += vertexCount;

        if (vertexCount + gRenderBuffer.m_vertexCount > IRENDER_NUM_VERTEX - 3) {
            iRenderFlush();
            v3d = gRenderBuffer.m_vertex;
        }

        idx = idx->m_next;
    }

    iRenderFlush();
}

void iParMgrRenderParSys_QuadStreak(void* data, xParGroup* ps)
{
    U32 transformFlags;
    xPar* idx = ps->m_root;

    iRenderSetCameraViewMatrix(NULL);

    zParSys* s = (zParSys*)data;
    RwTexture* texture = (RwTexture*)xSTFindAsset(s->tasset->textureID, NULL);
    if (texture) {
        RwRaster* raster = RwTextureGetRaster(texture);
        if (raster) {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
        }
    } else {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    }

    transformFlags = rwIM3D_VERTEXUV;

    while (idx) {
        iRenderPushQuadStreak(idx, ps->m_cmdTex);
        idx = idx->m_next;
    }

    iRenderFlush();
}

void iParMgrRenderParSys_Static(void*, xParGroup*)
{
}

void iParMgrRenderParSys_Ground(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;

    iRenderSetCameraViewMatrix(NULL);

    zParSys* s = (zParSys*)data;
    RwTexture* texture = (RwTexture*)xSTFindAsset(s->tasset->textureID, NULL);
    if (texture) {
        RwRaster* raster = RwTextureGetRaster(texture);
        if (raster) {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
        }
    }

    while (idx) {
        static RwIm3DVertex v3d[4];
        static U16 i3d[6] = { 0, 1, 2, 3, 0, 1 };

        xParCmdTex* tex = ps->m_cmdTex;
        xPar* p = idx;
        void* vertices = v3d;
        U16* indices = i3d;
        S32 vertexCount = 4, indexCount = 6;

        if (indexCount + gRenderBuffer.m_indexCount > IRENDER_NUM_INDEX ||
            vertexCount + gRenderBuffer.m_vertexCount > IRENDER_NUM_VERTEX) {
            iRenderFlush();
        }

        F32 size = 0.5f * p->m_size;
        xVec3 vert[4];
        U8 r = p->m_c[0], g = p->m_c[1], b = p->m_c[2], a = p->m_c[3];
        xMat3x3 groundmat;
        F32 angx, angy, angz;

        angx = angy = angz = 0.0f;
        if (p->m_rotdeg[0]) {
            angx = 2.0f * PI * (p->m_rotdeg[0] / 255.0f);
        }
        if (p->m_rotdeg[1]) {
            angy = 2.0f * PI * (p->m_rotdeg[1] / 255.0f);
        }
        if (p->m_rotdeg[2]) {
            angz = 2.0f * PI * (p->m_rotdeg[2] / 255.0f);
        }

        xMat3x3Euler(&groundmat, angx, angy, angz);

        xVec3 zdir, xdir, centre;

        zdir.x = groundmat.at.x * size;
        zdir.y = groundmat.at.y * size;
        zdir.z = groundmat.at.z * size;

        xdir.x = groundmat.right.x * size;
        xdir.y = groundmat.right.y * size;
        xdir.z = groundmat.right.z * size;

        centre.x = p->m_pos.x;
        centre.y = p->m_pos.y;
        centre.z = p->m_pos.z;

        vert[0].x = centre.x - xdir.x - zdir.x;
        vert[0].y = centre.y - xdir.y - zdir.y;
        vert[0].z = centre.z - xdir.z - zdir.z;

        vert[1].x = centre.x + xdir.x + zdir.x;
        vert[1].y = centre.y + xdir.y + zdir.y;
        vert[1].z = centre.z + xdir.z + zdir.z;

        vert[2].x = centre.x + xdir.x - zdir.x;
        vert[2].y = centre.y + xdir.y - zdir.y;
        vert[2].z = centre.z + xdir.z - zdir.z;

        vert[3].x = centre.x - xdir.x + zdir.x;
        vert[3].y = centre.y - xdir.y + zdir.y;
        vert[3].z = centre.z - xdir.z + zdir.z;

        RwIm3DVertexSetRGBA(&v3d[0], r, g, b, a);
        RwIm3DVertexSetRGBA(&v3d[1], r, g, b, a);
        RwIm3DVertexSetRGBA(&v3d[2], r, g, b, a);
        RwIm3DVertexSetRGBA(&v3d[3], r, g, b, a);

        RwIm3DVertexSetPos(&v3d[0], vert[0].x, vert[0].y, vert[0].z);
        RwIm3DVertexSetPos(&v3d[1], vert[1].x, vert[1].y, vert[1].z);
        RwIm3DVertexSetPos(&v3d[2], vert[2].x, vert[2].y, vert[2].z);
        RwIm3DVertexSetPos(&v3d[3], vert[3].x, vert[3].y, vert[3].z);

        if (tex) {
            F32 u1, u2, v1, v2;

            u1 = p->m_texIdx[0] * tex->unit_width + tex->x1;
            v1 = p->m_texIdx[1] * tex->unit_height + tex->y1;

            u2 = (p->m_texIdx[0] + 1) * tex->unit_width + tex->x1;
            v2 = (p->m_texIdx[1] + 1) * tex->unit_height + tex->y1;

            RwIm3DVertexSetUV(&v3d[0], u1, v1);
            RwIm3DVertexSetUV(&v3d[1], u2, v2);
            RwIm3DVertexSetUV(&v3d[2], u2, v1);
            RwIm3DVertexSetUV(&v3d[3], u1, v2);
        } else {
            RwIm3DVertexSetUV(&v3d[0], 0.0f, 0.0f);
            RwIm3DVertexSetUV(&v3d[1], 1.0f, 1.0f);
            RwIm3DVertexSetUV(&v3d[2], 1.0f, 0.0f);
            RwIm3DVertexSetUV(&v3d[3], 0.0f, 1.0f);
        }

        U16* src = indices;
        U16* dst = gRenderBuffer.m_index + gRenderBuffer.m_indexCount;

        for (S32 i = 0; i < indexCount; i++) {
            *dst++ = gRenderBuffer.m_vertexCount + *src++;
        }

        memcpy((U8*)gRenderBuffer.m_vertex + gRenderBuffer.m_vertexTypeSize * gRenderBuffer.m_vertexCount, vertices,
               gRenderBuffer.m_vertexTypeSize * vertexCount);

        gRenderBuffer.m_indexCount += indexCount;
        gRenderBuffer.m_vertexCount += vertexCount;

        idx = idx->m_next;
    }

    iRenderFlush();
}

void iParMgrRenderParSys_Flat(void* data, xParGroup* ps)
{
    U32 transformFlags;
    xPar* idx = ps->m_root;

    iRenderSetCameraViewMatrix(NULL);

    zParSys* s = (zParSys*)data;
    RwTexture* texture = (RwTexture*)xSTFindAsset(s->tasset->textureID, NULL);
    if (texture) {
        RwRaster* raster = RwTextureGetRaster(texture);
        if (raster) {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
        }
    }

    transformFlags = rwIM3D_VERTEXUV;

    while (idx) {
        iRenderPushFlat(idx, ps->m_cmdTex);
        idx = idx->m_next;
    }

    iRenderFlush();
}
