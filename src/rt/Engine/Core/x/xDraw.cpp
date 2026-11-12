#include "xDraw.h"

#include "xColor.h"
#include "xDebug.h"
#include "xMath.h"
#include "xMath3.h"
#include "xMemMgr.h"
#include "xRand.h"
#include "xRenderState.h"
#include "xutil.h"
#include "iDraw.h"

#include <string.h>

#if DEBUG || RELEASE
#define SPHERE_NUM_PTS 50
#define MARKER_NUM_LINES 7
#define BOX_NUM_PTS 8

static xColor sgDrawColor = { 0xFF, 0xFF, 0xFF, 0xFF };
static xVec3 g_normalXZ = { 0.0f, 1.0f, 0.0f };
static xVec3 g_normalXY = { 0.0f, 0.0f, 1.0f };
static F32 sph2_yaw;

void xDrawBegin()
{
#if DEBUG
    xColorInit(g_COLOR_TRYME, xrand_RandomChoice(256), xrand_RandomChoice(256), xrand_RandomChoice(256), 255);

    static U8 ctr = 0;
    static U8 ctr2 = 0;
    if (++ctr % 3 == 0) {
        if (ctr == 3) {
            xColorCopy(&g_COLOR_TRYME2, g_NEON_BLUE);
        } else {
            xColorCopy(&g_COLOR_TRYME2, g_NEON_RED);
            ctr = 0;
        }
    }

    if (++ctr2 % 2 == 0) {
        if (ctr2 == 2) {
            xColorCopy(&g_COLOR_TRYME3, g_NEON_GREEN);
        } else if (ctr2 == 4) {
            xColorCopy(&g_COLOR_TRYME3, g_NEON_BLUE);
        } else if (ctr2 == 6) {
            xColorCopy(&g_COLOR_TRYME3, g_BLACK);
        } else {
            xColorCopy(&g_COLOR_TRYME3, g_NEON_RED);
            ctr2 = 0;
        }
    }

    sph2_yaw = xAngleClamp(0.05235988f + sph2_yaw);
#endif

    iDrawBegin();
}

void xDrawEnd()
{
    iDrawEnd();
}

void xDrawSetColor(xColor color)
{
    xColorCopy(&sgDrawColor, color);
}

void xDrawSetColor(U8 r, U8 g, U8 b, U8 a)
{
    sgDrawColor.r = r;
    sgDrawColor.g = g;
    sgDrawColor.b = b;
    sgDrawColor.a = a;
}

void xDrawLine(const xVec3* a, const xVec3* b)
{
    U32 indices[2];
    indices[0] = 0;
    indices[1] = 1;

    RwV3d verts[2];
    verts[0] = a->m_RwV3d;
    verts[1] = b->m_RwV3d;

    iQueueLineList(sgDrawColor, verts, indices, NULL, ARRAY_LENGTH(verts), ARRAY_LENGTH(indices));
}

void xDrawLine(F32 x1, F32 y1, F32 z1, F32 x2, F32 y2, F32 z2)
{
    U32 indices[2];
    indices[0] = 0;
    indices[1] = 1;

    RwV3d verts[2];
    verts[0].x = x1;
    verts[0].y = y1;
    verts[0].z = z1;
    verts[1].x = x2;
    verts[1].y = y2;
    verts[1].z = z2;

    iQueueLineList(sgDrawColor, verts, indices, NULL, ARRAY_LENGTH(verts), ARRAY_LENGTH(indices));
}

void xDrawArrow(const xVec3* a, const xVec3* b, const xVec3* n, bool asRay, F32 length)
{
    xVec3 btemp;
    if (asRay) {
        xVec3SMul(&btemp, b, length);
        xVec3Add(&btemp, a, &btemp);
        b = &btemp;
    }

    xDrawLine(a, b);

    xVec3 direction;
    xVec3Sub(&direction, b, a);

    xVec3 base;
    xVec3SMul(&base, &direction, 0.9f);
    xVec3Add(&base, &base, a);

    xVec3 mod;
    xVec3Cross(&mod, &direction, n);
    xVec3SMul(&mod, &mod, 0.1f);

    xVec3 newB;
    xVec3Add(&newB, &base, &mod);
    xDrawLine(b, &newB);

    xVec3Sub(&newB, &base, &mod);
    xDrawLine(b, &newB);
}

void xDrawArrow(const xVec3* from, const xVec3* dir, bool asRay, F32 length)
{
    if (asRay) {
        xDrawArrow(from, dir, &g_normalXZ, asRay, length);
    } else {
        xVec3 end;
        end.AddScale(*from, *dir, length);
        xDrawArrow(from, &end, &g_normalXZ, asRay, length);
    }
}

void xDrawGrid(const xVec3& c1, const xVec3& c2, const xVec3& up, F32 spacing_hint)
{
    xVec3 diff, right;
    diff.Sub(c2, c1);
    right.AddScale(diff, up, -diff.dot(up));
    right.safe_normalize(g_X3);

    S32 grid_size_x = 0.5f + diff.dot(right) / spacing_hint;
    S32 grid_size_y = 0.5f + diff.dot(up) / spacing_hint;

    F32 spacing = diff.dot(right) / grid_size_x;

    for (S32 i = 0; i < grid_size_x; i++) {
        xVec3 v1, v2;
        v1.AddCombine(c1, right, i * (grid_size_x * spacing / (grid_size_x - 1)), up, grid_size_y * spacing);
        v2.AddScale(c1, right, i * (grid_size_x * spacing / (grid_size_x - 1)));
        xDrawLine(&v1, &v2);
    }

    for (S32 j = 0; j < grid_size_y; j++) {
        xVec3 v1, v2;
        v1.AddScale(c1, up, j * (grid_size_y * spacing / (grid_size_y - 1)));
        v2.AddCombine(c1, right, grid_size_x * spacing, up, j * (grid_size_y * spacing / (grid_size_y - 1)));
        xDrawLine(&v1, &v2);
    }
}

void xDrawArrowXY(const xVec3* a, const xVec3* b, bool asRay, F32 length)
{
    xDrawArrow(a, b, &g_normalXY, asRay, length);
}

void xDrawLine2D(F32 x1, F32 y1, F32 x2, F32 y2)
{
    RwIm2DVertex vtx[2];
    RwReal z = RwIm2DGetNearScreenZ();
    RwReal rcz = 1.0f / RwCameraGetNearClipPlane(RwCameraGetCurrentCamera());

    RwRaster* raster;
    S32 vae;
    RwRenderStateGet(rwRENDERSTATETEXTURERASTER, &raster);
    RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &vae);

    RwIm2DVertexSetScreenX(&vtx[0], x1);
    RwIm2DVertexSetScreenY(&vtx[0], y1);
    RwIm2DVertexSetScreenZ(&vtx[0], z);
    RwIm2DVertexSetIntRGBA(&vtx[0], sgDrawColor.r, sgDrawColor.g, sgDrawColor.b, sgDrawColor.a);

    RwIm2DVertexSetScreenX(&vtx[1], x2);
    RwIm2DVertexSetScreenY(&vtx[1], y2);
    RwIm2DVertexSetScreenZ(&vtx[1], z);
    RwIm2DVertexSetIntRGBA(&vtx[1], sgDrawColor.r, sgDrawColor.g, sgDrawColor.b, sgDrawColor.a);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);

    RwIm2DRenderLine(vtx, ARRAY_LENGTH(vtx), 0, 1);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)vae);
}

void xDrawPolyLine2D(const xVec2* p, U32 len)
{
    RwIm2DVertex* vtx = (RwIm2DVertex*)xMEMPUSHTEMP_L(len * sizeof(RwIm2DVertex), 0, 286);
    RwIm2DVertex* vp = vtx;
    RwReal z = RwIm2DGetNearScreenZ();
    RwReal rcz = 1.0f / RwCameraGetNearClipPlane(RwCameraGetCurrentCamera());

    xVALIDATE_L(292, vtx);
    if (!vtx) return;

    for (S32 i = 0; i < (S32)len; i++, p++, vp++) {
        RwIm2DVertexSetScreenX(vp, p->x);
        RwIm2DVertexSetScreenY(vp, p->y);
        RwIm2DVertexSetScreenZ(vp, z);

        RwIm2DVertexSetIntRGBA(vp, sgDrawColor.r, sgDrawColor.g, sgDrawColor.b, 255);
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);

    xRenderStateBlendModesSet(rwBLENDONE, rwBLENDZERO);
    RwIm2DRenderPrimitive(rwPRIMTYPEPOLYLINE, vtx, len);

    xRenderStateBlendModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);
    xMEMPOPTEMP(vtx);
}

void xDrawCircle2D(F32 x, F32 y, F32 radius, S32 segments)
{
    if (segments < 3) segments = 3;

    if (segments < 3) segments = 3;
    RwIm2DVertex* verts = (RwIm2DVertex*)xMEMPUSHTEMP_L((segments + 1) * sizeof(RwIm2DVertex), 0, 331);
    if (!verts) return;
    RwCamera* cam = RwCameraGetCurrentCamera();
    RwReal z = RwIm2DGetNearScreenZ();
    RwReal rcz = 1.0f / RwCameraGetNearClipPlane(cam);

    F32 dtheta = (2.0f * PI) / segments;
    F32 theta = 0.0f;
    for (RwIm2DVertex* v = verts, *end = v + segments + 1; v != end; v++, theta += dtheta) {
        RwIm2DVertexSetScreenX(v, x + radius * icos(theta));
        RwIm2DVertexSetScreenY(v, y + radius * isin(theta));
        RwIm2DVertexSetScreenZ(v, z);

        RwIm2DVertexSetIntRGBA(v, sgDrawColor.r, sgDrawColor.g, sgDrawColor.b, sgDrawColor.a);
    }

    RwRaster* old_raster;
    RwBlendFunction old_srcblend, old_destblend;
    RwRenderStateGet(rwRENDERSTATETEXTURERASTER, &old_raster);
    xRenderStateBlendModesGet(&old_srcblend, &old_destblend);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    xRenderStateBlendModesSet(rwBLENDONE, rwBLENDZERO);
    RwIm2DRenderPrimitive(rwPRIMTYPEPOLYLINE, verts, segments + 1);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, old_raster);
    xRenderStateBlendModesSet(old_srcblend, old_destblend);
    xMEMPOPTEMP(verts);
}

void xDrawSphere(const xSphere* sph, U32 flags)
{
    xDrawSphere(&sph->center, sph->r, flags);
}

void xDrawSphere(const xVec3* center, F32 r, U32 flags)
{
    xDrawSphere(center->x, center->y, center->z, r, flags);
}

void xDrawBox(const xBox* b)
{
    static xVec3 v[BOX_NUM_PTS];

    const xVec3* min = &b->lower;
    const xVec3* max = &b->upper;

    for (S32 i = 0; i < 4; i++) {
        v[i] = *min;
    }
    for (S32 i = 4; i < BOX_NUM_PTS; i++) {
        v[i] = *max;
    }

    v[1].x = max->x;

    v[2].x = max->x;
    v[2].y = max->y;

    v[3].y = max->y;

    v[4].x = min->x;
    v[4].y = min->y;

    v[5].y = min->y;

    v[7].x = min->x;

    xDrawLine(&v[0], &v[1]);
    xDrawLine(&v[1], &v[2]);
    xDrawLine(&v[2], &v[3]);
    xDrawLine(&v[3], &v[0]);

    xDrawLine(&v[4], &v[5]);
    xDrawLine(&v[5], &v[6]);
    xDrawLine(&v[6], &v[7]);
    xDrawLine(&v[7], &v[4]);

    xDrawLine(&v[0], &v[4]);
    xDrawLine(&v[1], &v[5]);
    xDrawLine(&v[2], &v[6]);
    xDrawLine(&v[3], &v[7]);
}

void xDrawBox(const xBox* b, const xMat4x3* mat)
{
    static xVec3 v[BOX_NUM_PTS];

    xVec3 min = b->lower;
    xVec3 max = b->upper;

    for (S32 i = 0; i < 4; i++) {
        v[i] = min;
    }
    for (S32 i = 4; i < BOX_NUM_PTS; i++) {
        v[i] = max;
    }

    v[1].x = max.x;

    v[2].x = max.x;
    v[2].y = max.y;

    v[3].y = max.y;

    v[4].x = min.x;
    v[4].y = min.y;

    v[5].y = min.y;

    v[7].x = min.x;

    for (S32 i = 0; i < BOX_NUM_PTS; i++) {
        xMat3x3RMulVec(&v[i], mat, &v[i]);
        v[i] += mat->pos;
    }

    xDrawLine(&v[0], &v[1]);
    xDrawLine(&v[1], &v[2]);
    xDrawLine(&v[2], &v[3]);
    xDrawLine(&v[3], &v[0]);

    xDrawLine(&v[4], &v[5]);
    xDrawLine(&v[5], &v[6]);
    xDrawLine(&v[6], &v[7]);
    xDrawLine(&v[7], &v[4]);

    xDrawLine(&v[0], &v[4]);
    xDrawLine(&v[1], &v[5]);
    xDrawLine(&v[2], &v[6]);
    xDrawLine(&v[3], &v[7]);
}

void xDrawBox(F32 x1, F32 y1, F32 z1, F32 x2, F32 y2, F32 z2)
{
    xBox b;
    b.lower.x = x1;
    b.lower.y = y1;
    b.lower.z = z1;
    b.upper.x = x2;
    b.upper.y = y2;
    b.upper.z = z2;
    xDrawBox(&b);
}

static xVec3 sphere_pts[SPHERE_NUM_PTS] = {
    { 0.57735f, 0.57735f, -0.57735f },
    { -0.57735f, 0.57735f, -0.57735f },
    { -0.57735f, 0.57735f, 0.57735f },
    { 0.57735f, 0.57735f, 0.57735f },
    { 0.57735f, -0.57735f, -0.57735f },
    { -0.57735f, -0.57735f, -0.57735f },
    { -0.57735f, -0.57735f, 0.57735f },
    { 0.57735f, -0.57735f, 0.57735f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, -1.0f },
    { -1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f },
    { 1.0f, 0.0f, 0.0f },
    { 0.0f, -1.0f, 0.0f },
    { 0.0f, 0.707107f, -0.707107f },
    { -0.707107f, 0.707107f, 0.0f },
    { 0.0f, 0.707107f, 0.707107f },
    { 0.707107f, 0.707107f, 0.0f },
    { 0.707107f, 0.0f, -0.707107f },
    { -0.707107f, 0.0f, -0.707107f },
    { -0.707107f, 0.0f, 0.707107f },
    { 0.707107f, 0.0f, 0.707107f },
    { 0.0f, -0.707107f, -0.707107f },
    { -0.707107f, -0.707107f, 0.0f },
    { 0.0f, -0.707107f, 0.707107f },
    { 0.707107f, -0.707107f, 0.0f },
    { 0.325058f, 0.888074f, -0.325058f },
    { -0.325058f, 0.888074f, -0.325058f },
    { -0.325058f, 0.888074f, 0.325058f },
    { 0.325058f, 0.888074f, 0.325058f },
    { 0.325058f, 0.325058f, -0.888074f },
    { -0.325058f, 0.325058f, -0.888074f },
    { -0.325058f, -0.325058f, -0.888074f },
    { 0.325058f, -0.325058f, -0.888074f },
    { -0.888074f, 0.325058f, -0.325058f },
    { -0.888074f, 0.325058f, 0.325058f },
    { -0.888074f, -0.325058f, 0.325058f },
    { -0.888074f, -0.325058f, -0.325058f },
    { 0.325058f, 0.325058f, 0.888074f },
    { 0.325058f, -0.325058f, 0.888074f },
    { -0.325058f, -0.325058f, 0.888074f },
    { -0.325058f, 0.325058f, 0.888074f },
    { 0.888074f, -0.325058f, 0.325058f },
    { 0.888074f, -0.325058f, -0.325058f },
    { 0.888074f, 0.325058f, -0.325058f },
    { 0.888074f, 0.325058f, 0.325058f },
    { -0.325058f, -0.888074f, -0.325058f },
    { -0.325058f, -0.888074f, 0.325058f },
    { 0.325058f, -0.888074f, 0.325058f },
    { 0.325058f, -0.888074f, -0.325058f },
};

static struct
{
    U32 begin;
    U32 end;
} sphere_segs[144] = {
    { 8, 26 }, { 26, 29 }, { 29, 17 }, { 17, 26 }, { 26, 0 }, { 0, 17 },
    { 8, 29 }, { 29, 28 }, { 28, 16 }, { 16, 29 }, { 29, 3 }, { 3, 16 },
    { 8, 28 }, { 28, 27 }, { 27, 15 }, { 15, 28 }, { 28, 2 }, { 2, 15 },
    { 8, 27 }, { 27, 26 }, { 26, 14 }, { 14, 27 }, { 27, 1 }, { 1, 14 },
    { 9, 33 }, { 33, 30 }, { 30, 18 }, { 18, 33 }, { 33, 4 }, { 4, 18 },
    { 9, 30 }, { 30, 31 }, { 31, 14 }, { 14, 30 }, { 30, 0 }, { 0, 14 },
    { 9, 31 }, { 31, 32 }, { 32, 19 }, { 19, 31 }, { 31, 1 }, { 1, 19 },
    { 9, 32 }, { 32, 33 }, { 33, 22 }, { 22, 32 }, { 32, 5 }, { 5, 22 },
    { 10, 37 }, { 37, 34 }, { 34, 19 }, { 19, 37 }, { 37, 5 }, { 5, 19 },
    { 10, 34 }, { 34, 35 }, { 35, 15 }, { 15, 34 }, { 34, 1 }, { 1, 15 },
    { 10, 35 }, { 35, 36 }, { 36, 20 }, { 20, 35 }, { 35, 2 }, { 2, 20 },
    { 10, 36 }, { 36, 37 }, { 37, 23 }, { 23, 36 }, { 36, 6 }, { 6, 23 },
    { 11, 39 }, { 39, 40 }, { 40, 24 }, { 24, 39 }, { 39, 7 }, { 7, 24 },
    { 11, 40 }, { 40, 41 }, { 41, 20 }, { 20, 40 }, { 40, 6 }, { 6, 20 },
    { 11, 41 }, { 41, 38 }, { 38, 16 }, { 16, 41 }, { 41, 2 }, { 2, 16 },
    { 11, 38 }, { 38, 39 }, { 39, 21 }, { 21, 38 }, { 38, 3 }, { 3, 21 },
    { 12, 43 }, { 43, 42 }, { 42, 25 }, { 25, 43 }, { 43, 4 }, { 4, 25 },
    { 12, 42 }, { 42, 45 }, { 45, 21 }, { 21, 42 }, { 42, 7 }, { 7, 21 },
    { 12, 45 }, { 45, 44 }, { 44, 17 }, { 17, 45 }, { 45, 3 }, { 3, 17 },
    { 12, 44 }, { 44, 43 }, { 43, 18 }, { 18, 44 }, { 44, 0 }, { 0, 18 },
    { 13, 46 }, { 46, 47 }, { 47, 23 }, { 23, 46 }, { 46, 5 }, { 5, 23 },
    { 13, 47 }, { 47, 48 }, { 48, 24 }, { 24, 47 }, { 47, 6 }, { 6, 24 },
    { 13, 48 }, { 48, 49 }, { 49, 25 }, { 25, 48 }, { 48, 7 }, { 7, 25 },
    { 13, 49 }, { 49, 46 }, { 46, 22 }, { 22, 49 }, { 49, 4 }, { 4, 22 },
};

void xDrawSphere(F32 x, F32 y, F32 z, F32 r, U32)
{
    xVec3 vertices[SPHERE_NUM_PTS];
    for (S32 i = 0; i < SPHERE_NUM_PTS; i++) {
        vertices[i].x = x + r * sphere_pts[i].x;
        vertices[i].y = y + r * sphere_pts[i].y;
        vertices[i].z = z + r * sphere_pts[i].z;
    }

    iQueueLineList(sgDrawColor, (RwV3d*)vertices, (U32*)sphere_segs, NULL, ARRAY_LENGTH(vertices), 2 * ARRAY_LENGTH(sphere_segs));
}

void xDrawSphere2(F32 x, F32 y, F32 z, F32 r, U32 lod)
{
    U32 ninr = MIN(lod, 50);
    U32 nlverts = ninr * 3;
    U32 nlidxs = nlverts * 2;
    RwV3d* lverts = (RwV3d*)xMEMPUSHTEMP_L(nlverts * sizeof(RwV3d), 0, 750);
    U32* lidxs = (U32*)xMEMPUSHTEMP_L(nlidxs * sizeof(U32), 0, 751);
    U32 idx;

    xMat3x3 sph_rot;
    xMat3x3RotY(&sph_rot, sph2_yaw);

    for (idx = 0; idx < ninr; idx++) {
        xVec3* v = (xVec3*)&lverts[idx];
        v->x = 0.0f;
        v->y = r * icos(2.0f * PI * idx / ninr);
        v->z = r * isin(2.0f * PI * idx / ninr);
        xMat3x3RMulVec(v, &sph_rot, v);
        xVec3AddC(v, v, x, y, z);

        v = (xVec3*)&lverts[idx + ninr];
        v->x = r * isin(2.0f * PI * idx / ninr);
        v->y = 0.0f;
        v->z = r * icos(2.0f * PI * idx / ninr);
        xMat3x3RMulVec(v, &sph_rot, v);
        xVec3AddC(v, v, x, y, z);

        v = (xVec3*)&lverts[idx + ninr * 2];
        v->x = r * icos(2.0f * PI * idx / ninr);
        v->y = r * isin(2.0f * PI * idx / ninr);
        v->z = 0.0f;
        xMat3x3RMulVec(v, &sph_rot, v);
        xVec3AddC(v, v, x, y, z);

        lidxs[idx * 2] = idx;
        lidxs[idx * 2 + 1] = (idx + 1) % ninr;

        lidxs[idx * 2 + ninr * 2] = ninr + idx;
        lidxs[idx * 2 + ninr * 2 + 1] = ninr + (idx + 1) % ninr;

        lidxs[idx * 2 + ninr * 4] = idx + ninr * 2;
        lidxs[idx * 2 + ninr * 4 + 1] = ninr * 2 + (idx + 1) % ninr;
    }

    iQueueLineList(sgDrawColor, lverts, lidxs, NULL, nlverts, nlidxs);

    xMEMPOPTEMP(lidxs);
    xMEMPOPTEMP(lverts);
}

void xDrawSphere2(const xSphere* sph, U32 lod)
{
    xDrawSphere2(sph->center.x, sph->center.y, sph->center.z, sph->r, lod);
}

void xDrawSphere2(const xVec3* pos, F32 r, U32 lod)
{
    xDrawSphere2(pos->x, pos->y, pos->z, r, lod);
}

void xDrawCapsule(const xVec3* start, const xVec3* end, F32 r, U32 lod)
{
    xASSERT_L(806, start);
    xASSERT_L(807, end);

    U32 ninr = CLAMP(lod, 4, 12);
    U32 nlverts = ninr * 2 + 2;
    U32 nlidxs = ninr * 10;
    RwV3d* lverts = (RwV3d*)xMEMPUSHTEMP_L(nlverts * sizeof(RwV3d), 0, 812);
    U32* lidxs = (U32*)xMEMPUSHTEMP_L(nlidxs * sizeof(U32), 0, 813);
    U32 idx;

    xMat4x3 mat;
    xVec3 diff;
    xVec3Sub(&diff, end, start);
    xVec3SMulBy(&diff, 0.5f);
    xMat3x3LookVec(&mat, &diff);
    xVec3Lerp(&mat.pos, start, end, 0.5f);
    F32 len = xVec3Length(&diff);

    for (idx = 0; idx < ninr; idx++) {
        lverts[idx].x = r * icos(2.0f * PI * idx / ninr);
        lverts[idx].y = r * isin(2.0f * PI * idx / ninr);
        lverts[idx].z = -len;

        lverts[idx + ninr].x = lverts[idx].x;
        lverts[idx + ninr].y = lverts[idx].y;
        lverts[idx + ninr].z = len;

        lidxs[idx * 2] = ninr * 2;
        lidxs[idx * 2 + 1] = idx;

        lidxs[idx * 2 + ninr * 2] = idx;
        lidxs[idx * 2 + ninr * 2 + 1] = (idx + 1) % ninr;

        lidxs[idx * 2 + ninr * 4] = idx;
        lidxs[idx * 2 + ninr * 4 + 1] = idx + ninr;

        lidxs[idx * 2 + ninr * 6] = ninr + idx;
        lidxs[idx * 2 + ninr * 6 + 1] = ninr + (idx + 1) % ninr;

        lidxs[idx * 2 + ninr * 8] = ninr + idx;
        lidxs[idx * 2 + ninr * 8 + 1] = ninr * 2 + 1;
    }
    lverts[ninr * 2].x = 0.0f;
    lverts[ninr * 2].y = 0.0f;
    lverts[ninr * 2].z = -(len + r);
    lverts[ninr * 2 + 1].x = 0.0f;
    lverts[ninr * 2 + 1].y = 0.0f;
    lverts[ninr * 2 + 1].z = len + r;

    for (idx = 0; idx < nlverts; idx++) {
        xMat4x3ToWorld((xVec3*)&lverts[idx], &mat, (xVec3*)&lverts[idx]);
    }

    iQueueLineList(sgDrawColor, lverts, lidxs, NULL, nlverts, nlidxs);

    xMEMPOPTEMP(lidxs);
    xMEMPOPTEMP(lverts);
}

void xDrawOBB(const xBox* b, const xMat4x3* m)
{
    static U32 idxs[12][2] = {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
        { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
        { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 }
    };
    xVec3 lverts[BOX_NUM_PTS];
    RwV3d wverts[BOX_NUM_PTS];
    lverts[0].x = b->lower.x;
    lverts[0].y = b->lower.y;
    lverts[0].z = b->lower.z;
    lverts[1].x = b->lower.x;
    lverts[1].y = b->lower.y;
    lverts[1].z = b->upper.z;
    lverts[2].x = b->upper.x;
    lverts[2].y = b->lower.y;
    lverts[2].z = b->upper.z;
    lverts[3].x = b->upper.x;
    lverts[3].y = b->lower.y;
    lverts[3].z = b->lower.z;
    lverts[4].x = b->lower.x;
    lverts[4].y = b->upper.y;
    lverts[4].z = b->lower.z;
    lverts[5].x = b->lower.x;
    lverts[5].y = b->upper.y;
    lverts[5].z = b->upper.z;
    lverts[6].x = b->upper.x;
    lverts[6].y = b->upper.y;
    lverts[6].z = b->upper.z;
    lverts[7].x = b->upper.x;
    lverts[7].y = b->upper.y;
    lverts[7].z = b->lower.z;

    for (S32 i = 0; i < BOX_NUM_PTS; i++) {
        xMat4x3ToWorld((xVec3*)&wverts[i], m, &lverts[i]);
    }

    iQueueLineList(sgDrawColor, wverts, (U32*)idxs, NULL, ARRAY_LENGTH(wverts), 2 * ARRAY_LENGTH(idxs));
}

void xDrawOBBShaded(const xBox* b, const xMat4x3* m)
{
    S32 i;
    xVec3 lverts[BOX_NUM_PTS];
    for (i = 0; i < 4; i++) {
        lverts[i] = b->lower;
    }
    for (i = 4; i < BOX_NUM_PTS; i++) {
        lverts[i] = b->upper;
    }
    lverts[1].x = b->upper.x;
    lverts[2].x = b->upper.x;
    lverts[2].y = b->upper.y;
    lverts[3].y = b->upper.y;
    lverts[4].x = b->lower.x;
    lverts[4].y = b->lower.y;
    lverts[5].y = b->lower.y;
    lverts[7].x = b->lower.x;

    xVec3 wverts[BOX_NUM_PTS];
    for (i = 0; i < BOX_NUM_PTS; i++) {
        xMat4x3ToWorld(&wverts[i], m, &lverts[i]);
    }

    for (S32 cullmode = rwCULLMODECULLBACK; cullmode >= rwCULLMODECULLNONE; cullmode--) {
        iDrawShadedQuad(sgDrawColor, NULL, &wverts[3], &wverts[2], &wverts[1], &wverts[0], cullmode);
        iDrawShadedQuad(sgDrawColor, NULL, &wverts[4], &wverts[5], &wverts[6], &wverts[7], cullmode);
        iDrawShadedQuad(sgDrawColor, NULL, &wverts[0], &wverts[1], &wverts[5], &wverts[4], cullmode);
        iDrawShadedQuad(sgDrawColor, NULL, &wverts[1], &wverts[2], &wverts[6], &wverts[5], cullmode);
        iDrawShadedQuad(sgDrawColor, NULL, &wverts[2], &wverts[3], &wverts[7], &wverts[6], cullmode);
        iDrawShadedQuad(sgDrawColor, NULL, &wverts[3], &wverts[0], &wverts[4], &wverts[7], cullmode);
    }
}

void xDrawCircle(const xVec3* center, F32 radius, U32 flags)
{
    xMat4x3 mat = g_I3;
    mat.pos = *center;
    xDrawCircle(&mat, radius, flags);
}

void xDrawCircle(const xMat4x3* mat, F32 radius, U32 flags)
{
    U32 num_pts;
    xVec3* ptlist, *tmp_pt, *pt_from, *pt_to;
    F32 perarc, ang;
    U32 i;

    num_pts = flags & 0xFF;
    if (num_pts < 4) num_pts = 4;
    perarc = (2.0f * PI) / (num_pts - 1);

    ptlist = (xVec3*)xMEMPUSHTEMP_L(num_pts * sizeof(xVec3), 'CRCL', 991);
    memset(ptlist, 0, num_pts * sizeof(xVec3));

    for (i = 0; i < num_pts; i++) {
        ang = i * perarc;
        tmp_pt = &ptlist[i];
        tmp_pt->x = radius * isin(ang);
        tmp_pt->z = radius * icos(ang);
        xMat4x3ToWorld(tmp_pt, mat, tmp_pt);
    }

    for (i = 0; i < num_pts; i++) {
        if (i == 0) pt_from = &ptlist[num_pts - 1];
        else pt_from = &ptlist[i - 1];
        pt_to = &ptlist[i];

        xDrawLine(pt_from, pt_to);
    }

    xMEMPOPTEMP(ptlist);
}

// UNUSED: speculative
void xDrawCyl(const xVec3* draw_at, F32 radius, F32 height, const xVec3* orient, U32 flags)
{
    U32 num_cpts, num_vseg, num_totpt;
    xVec3* ptlist, *row_cur, *pt, *row_up, *pt_up, *pt_prev;
    F32 angper, ang, hytper, hytshft, hyt;
    xMat3x3 xform = { 1.0f, 0.0f, 0.0f, 0, 0.0f, 1.0f, 0.0f, 0, 0.0f, 0.0f, 1.0f, 0 };
    U32 i, j;

    num_cpts = flags & 0xFF;
    if (num_cpts < 4) num_cpts = 4;
    num_vseg = (flags >> 8) & 0xFF;
    if (num_vseg < 1) num_vseg = 1;
    num_totpt = num_cpts * (num_vseg + 1);

    angper = (2.0f * PI) / num_cpts;
    hytper = height / num_vseg;
    hytshft = -0.5f * height;

    xMat3x3Euler(&xform, orient);

    ptlist = (xVec3*)xMEMPUSHTEMP(num_totpt * sizeof(xVec3), 'CYLN');

    for (j = 0; j <= num_vseg; j++) {
        row_cur = &ptlist[j * num_cpts];
        hyt = hytshft + j * hytper;
        for (i = 0; i < num_cpts; i++) {
            ang = i * angper;
            pt = &row_cur[i];
            pt->x = radius * isin(ang);
            pt->y = hyt;
            pt->z = radius * icos(ang);
            xMat3x3RMulVec(pt, &xform, pt);
            *pt += *draw_at;
        }
    }

    for (j = 0; j <= num_vseg; j++) {
        row_cur = &ptlist[j * num_cpts];
        row_up = &ptlist[(j + 1) * num_cpts];
        pt_prev = &row_cur[num_cpts - 1];
        for (i = 0; i < num_cpts; i++) {
            pt = &row_cur[i];
            xDrawLine(pt_prev, pt);
            if (j < num_vseg) {
                pt_up = &row_up[i];
                xDrawLine(pt, pt_up);
            }
            pt_prev = pt;
        }
    }

    xMEMPOPTEMP(ptlist);
}

void xDrawCylinder(const xVec3& start, const xVec3& end, F32 radius, S32 sides, S32 rings)
{
    xDrawCone(start, radius, end, radius, sides, rings);
}

void xDrawCone(const xVec3& p1, F32 r1, const xVec3& p2, F32 r2, S32 sides, S32 rings)
{
    if (sides < 3) sides = 3;
    if (rings < 2) rings = 2;

    xMat3x3 mat;
    xVec3 at;
    at.Sub(p2, p1);
    F32 mag = xMat3x3LookVec3(mat, at);

    xVec3 loc[2][2];
    loc[0][0].assign(r1, 0.0f, 0.0f);
    loc[0][1].assign(r2, 0.0f, mag);
    xMat3x3RMulVec(&loc[0][0], &mat, &loc[0][0]); loc[0][0] += p1;
    xMat3x3RMulVec(&loc[0][1], &mat, &loc[0][1]); loc[0][1] += p1;

    F32 dang = (2.0f * PI) / sides;
    F32 dring = 1.0f / (rings - 1);

    F32 ang = dang;
    for (S32 i = 1, iend = sides + 1; i < iend; i++, ang += dang) {
        xVec3& loc1 = loc[i & 1][0];
        xVec3& loc2 = loc[i & 1][1];

        F32 ca = icos(ang), sa = isin(ang);
        loc1.assign(r1 * ca, r1 * sa, 0.0f);
        loc2.assign(r2 * ca, r2 * sa, mag);
        xMat3x3RMulVec(&loc1, &mat, &loc1); loc1 += p1;
        xMat3x3RMulVec(&loc2, &mat, &loc2); loc2 += p1;

        xDrawLine(&loc1, &loc2);

        xVec3& prevloc1 = loc[(i - 1) & 1][0];
        const xVec3& prevloc2 = loc[(i - 1) & 1][1];
        xVec3 dprevloc; dprevloc.ScaleDiff(prevloc2, prevloc1, dring);
        xVec3 dcurloc; dcurloc.ScaleDiff(loc2, loc1, dring);
        xVec3 curloc = loc1;
        for (S32 j = 0; j < rings; j++, prevloc1 += dprevloc, curloc += dcurloc)
            xDrawLine(&prevloc1, &curloc);
    }
}

void xDrawTri(const xVec3* a, const xVec3* b, const xVec3* c)
{
    xDrawLine(a, b);
    xDrawLine(b, c);
    xDrawLine(c, a);
}

void xDrawAxis(const xMat4x3* mat_orient, F32 scale)
{
    xColor prevColor;
    xColorCopy(&prevColor, sgDrawColor);

    xVec3 pos_end;
    pos_end.AddScale(mat_orient->pos, mat_orient->at, scale);
    xDrawSetColor(g_BLUE);
    xDrawLine(&mat_orient->pos, &pos_end);

    pos_end.AddScale(mat_orient->pos, mat_orient->up, scale);
    xDrawSetColor(g_GREEN);
    xDrawLine(&mat_orient->pos, &pos_end);

    pos_end.AddScale(mat_orient->pos, mat_orient->right, scale);
    xDrawSetColor(g_RED);
    xDrawLine(&mat_orient->pos, &pos_end);

    xColorCopy(&sgDrawColor, prevColor);
}

void xDrawMarker(const xVec3* pos, F32 size, U32 flags)
{
    xVec3 start, end;
    F32 size2 = size / 2.0f;

    static struct
    {
        xVec3 start;
        xVec3 end;
        F32 length;
    } lists[MARKER_NUM_LINES] = {
        { { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 1.0f },
        { { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 1.0f },
        { { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f }, 1.0f },
        { { -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, 0.6933613f },
        { { -1.0f, -1.0f, 1.0f }, { 1.0f, 1.0f, -1.0f }, 0.6933613f },
        { { -1.0f, 1.0f, 1.0f }, { 1.0f, -1.0f, -1.0f }, 0.6933613f },
        { { -1.0f, 1.0f, -1.0f }, { 1.0f, -1.0f, 1.0f }, 0.6933613f },
    };

    for (S32 i = 0; i < ((flags & 1) ? MARKER_NUM_LINES : 3); i++) {
        xVec3SMul(&start, &lists[i].start, size2 * lists[i].length);
        xVec3Add(&start, &start, pos);
        xVec3SMul(&end, &lists[i].end, size2 * lists[i].length);
        xVec3Add(&end, &end, pos);
        xDrawLine(&start, &end);
    }
}

void xDrawArc(const xMat4x3& mat, F32 radius, F32 arc_length, S32 segments)
{
    arc_length = CLAMP(arc_length, 0.0f, 2.0f * PI);
    segments = std::ceilf((1.0f / (2.0f * PI)) * (segments * arc_length));
    if (segments < 1) segments = 1;

    F32 dang = arc_length / segments;
    F32 sr = isin(dang), cr = icos(dang);
    F32 x = radius, y = 0.0f;

    xVec3 v[2];
    S32 vi = 0;
    v[0].assign(x, y, 0.0f);
    xMat4x3ToWorld(&v[0], &mat, &v[0]);
    for (S32 i = 0; i < segments; i++) {
        F32 xtemp = x;
        F32 ytemp = y;
        x = cr * xtemp - sr * ytemp;
        y = cr * ytemp + sr * xtemp;
        const xVec3& v0 = v[vi];
        vi ^= 1;
        xVec3& v1 = v[vi];
        v1.assign(x, y, 0.0f);
        xMat4x3ToWorld(&v1, &mat, &v1);
        xDrawLine(&v0, &v1);
    }
}

namespace {
    inline void set_vert(RwIm2DVertex& vert, F32 x, F32 y, F32 u, F32 v, F32 rcz, F32 nsz)
    {
        RwIm2DVertexSetScreenX(&vert, x);
        RwIm2DVertexSetScreenY(&vert, y);
        RwIm2DVertexSetScreenZ(&vert, nsz);

        RwIm2DVertexSetU(&vert, u, rcz);
        RwIm2DVertexSetV(&vert, v, rcz);
        RwIm2DVertexSetIntRGBA(&vert, 255, 255, 255, 255);
    }
}

void xDrawCard(RwRaster* raster, const xFRect& src_rect, const xFRect& dst_rect, bool zwrite)
{
    S32 old_filter, old_wrap, old_fog, old_cull, old_zwrite, old_ztest, old_vertalpha, old_shademode;
    RwBlendFunction old_srcblend, old_destblend;

    RwRenderStateGet(rwRENDERSTATETEXTUREFILTER, &old_filter);
    RwRenderStateGet(rwRENDERSTATETEXTUREADDRESS, &old_wrap);
    RwRenderStateGet(rwRENDERSTATEFOGENABLE, &old_fog);
    RwRenderStateGet(rwRENDERSTATECULLMODE, &old_cull);
    RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &old_zwrite);
    RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &old_ztest);
    RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &old_vertalpha);
    xRenderStateBlendModesGet(&old_srcblend, &old_destblend);
    RwRenderStateGet(rwRENDERSTATESHADEMODE, &old_shademode);

    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);

    xRenderStateBlendAndZModesSet(rwBLENDONE, rwBLENDZERO, zwrite, FALSE);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEFLAT);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);

    RwReal rcz = 1.0f / RwCameraGetNearClipPlane(RwCameraGetCurrentCamera());
    RwReal nsz = RwIm2DGetNearScreenZ();

    RwIm2DVertex vert[4];
    F32 x0 = dst_rect.x, x1 = x0 + dst_rect.w;
    F32 y0 = dst_rect.y, y1 = y0 + dst_rect.h;
    F32 uscale = 1.0f / raster->originalWidth;
    F32 vscale = 1.0f / raster->originalHeight;
    F32 u0 = src_rect.x * uscale, u1 = u0 + src_rect.w * uscale;
    F32 v0 = src_rect.y * vscale, v1 = v0 + src_rect.h * vscale;

    set_vert(vert[0], x0, y0, u0, v0, rcz, nsz);
    set_vert(vert[1], x0, y1, u0, v1, rcz, nsz);
    set_vert(vert[2], x1, y0, u1, v0, rcz, nsz);
    set_vert(vert[3], x1, y1, u1, v1, rcz, nsz);
    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, vert, ARRAY_LENGTH(vert));

    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)old_filter);
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)old_wrap);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)old_fog);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)old_cull);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)old_vertalpha);
    xRenderStateBlendAndZModesSet(old_srcblend, old_destblend, old_zwrite, old_ztest);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)old_shademode);
}

void xDrawCard(RwRaster* raster, F32 x, F32 y, F32 w, F32 h, bool zwrite)
{
    xFRect dst_rect = { x, y, w, h };
    xFRect src_rect = { 0.0f, 0.0f, raster->originalWidth, raster->originalHeight };
    xDrawCard(raster, src_rect, dst_rect, zwrite);
}

static F32 Det3x3(F32 a, F32 b, F32 c, F32 d, F32 e, F32 f, F32 g, F32 h, F32 i); // UNUSED
static void IntersectThreePlanes(const RwFrustumPlane* a, const RwFrustumPlane* b, const RwFrustumPlane* c, xVec3* o); // UNUSED
#endif

S32 Im2DRenderQuad(F32 x1, F32 y1, F32 x2, F32 y2, F32 z, F32 recipCamZ, F32 uvOffset)
{
    RwIm2DVertex vx[4];

    RwIm2DVertexSetScreenX(&vx[0], x1);
    RwIm2DVertexSetScreenY(&vx[0], y1);
    RwIm2DVertexSetScreenZ(&vx[0], z);
    RwIm2DVertexSetIntRGBA(&vx[0], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[0], uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[0], uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[1], x1);
    RwIm2DVertexSetScreenY(&vx[1], y2);
    RwIm2DVertexSetScreenZ(&vx[1], z);
    RwIm2DVertexSetIntRGBA(&vx[1], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[1], uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[1], 1.0f + uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[2], x2);
    RwIm2DVertexSetScreenY(&vx[2], y1);
    RwIm2DVertexSetScreenZ(&vx[2], z);
    RwIm2DVertexSetIntRGBA(&vx[2], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[2], 1.0f + uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[2], uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[3], x2);
    RwIm2DVertexSetScreenY(&vx[3], y2);
    RwIm2DVertexSetScreenZ(&vx[3], z);
    RwIm2DVertexSetIntRGBA(&vx[3], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[3], 1.0f + uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[3], 1.0f + uvOffset, recipCamZ);

    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, vx, ARRAY_LENGTH(vx));

    return TRUE;
}

S32 Im2DRenderQuadAlpha(F32 x1, F32 y1, F32 x2, F32 y2, F32 alpha, F32 z, F32 recipCamZ, F32 uvOffset)
{
    RwIm2DVertex vx[4];
    U8 alphaI = 255.0f * alpha;

    RwIm2DVertexSetScreenX(&vx[0], x1);
    RwIm2DVertexSetScreenY(&vx[0], y1);
    RwIm2DVertexSetScreenZ(&vx[0], z);
    RwIm2DVertexSetIntRGBA(&vx[0], 255, 255, 255, alphaI);
    RwIm2DVertexSetU(&vx[0], uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[0], uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[1], x1);
    RwIm2DVertexSetScreenY(&vx[1], y2);
    RwIm2DVertexSetScreenZ(&vx[1], z);
    RwIm2DVertexSetIntRGBA(&vx[1], 255, 255, 255, alphaI);
    RwIm2DVertexSetU(&vx[1], uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[1], 1.0f + uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[2], x2);
    RwIm2DVertexSetScreenY(&vx[2], y1);
    RwIm2DVertexSetScreenZ(&vx[2], z);
    RwIm2DVertexSetIntRGBA(&vx[2], 255, 255, 255, alphaI);
    RwIm2DVertexSetU(&vx[2], 1.0f + uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[2], uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[3], x2);
    RwIm2DVertexSetScreenY(&vx[3], y2);
    RwIm2DVertexSetScreenZ(&vx[3], z);
    RwIm2DVertexSetIntRGBA(&vx[3], 255, 255, 255, alphaI);
    RwIm2DVertexSetU(&vx[3], 1.0f + uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[3], 1.0f + uvOffset, recipCamZ);

    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, vx, ARRAY_LENGTH(vx));

    return TRUE;
}
