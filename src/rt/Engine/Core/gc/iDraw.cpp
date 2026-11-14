#include "xCommon.h"
#include "iDraw.h"

#include "xCam.h"
#include "xDebug.h"
#include "xMath.h"
#include "xMath3.h"
#include "xRenderState.h"
#include "xVec3.h"
#include "zGlobals.h"
#include "zMain.h"

#include "decomp.h"

U32 lastSetFBMSK;

void iDrawSetFBMSK(U32 abgr, U32 setMask, bool)
{
    abgr = (abgr & setMask) | (lastSetFBMSK & ~setMask);
    lastSetFBMSK = abgr;
    if (setMask & 0xFF000000) {
        U8 alpha = abgr >> 24;
        if (alpha == 0)
            GXSetAlphaUpdate(GX_TRUE);
        else if (alpha == 0xFF)
            GXSetAlphaUpdate(GX_FALSE);
    }

    if (setMask & 0xFFFFFF) {
        U32 color = abgr & 0xFFFFFF;
        if (color == 0)
            GXSetColorUpdate(GX_TRUE);
        else
            GXSetColorUpdate(GX_FALSE);
    }
}

xTWEAKABLE F32 PAL_OFFX = 40.0f;
xTWEAKABLE F32 PAL_OFFY = 23.0f;

void iDrawSetDisplayOffset(F32 x, F32 y)
{
    if (zMainIsPAL()) {
        F32 nPosX = x * FB_XRES + PAL_OFFX;
        F32 nPosY = y * FB_YRES + PAL_OFFY;

        nPosX = CLAMP(nPosX, 0.0f, 80.0f);
        nPosY = CLAMP(nPosY, 0.0f, 551.0f);

        _RwDlRenderMode->viXOrigin = nPosX;
        _RwDlRenderMode->viYOrigin = nPosY;

        VIConfigure(_RwDlRenderMode);
    }
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("iDraw.cpp", "count <= 64", "%s", "count <= MAX_VERTS")

#define MAX_LINE_VERTS 10000
#define MAX_LINE_INDICES 40000
#define MAX_SHADED_VERTS 10000

static RwIm3DVertex gLineVerts[MAX_LINE_VERTS];
static RwImVertexIndex gLineIndicies[MAX_LINE_INDICES];
static RwIm3DVertex gShadedVerts[MAX_SHADED_VERTS];
static U32 gNumLineVerts;
static U32 gNumLineIndices;
static U32 gNumShadedVerts;

void iDrawBegin()
{
    gNumLineVerts = 0;
    gNumLineIndices = 0;
    gNumShadedVerts = 0;
}

void iQueueLineList(xColor color, RwV3d* verts, U32* indices, RwMatrix* mat, U32 numVerts, U32 numIndices)
{
    U32 maxi, i, qi;

    maxi = gNumLineIndices + numIndices;

    if (maxi >= MAX_LINE_INDICES) {
        DBprintf(DBML_USER, "iQueueLineList:max num indices reached:%d\n", maxi);
        return;
    }

    for (i = gNumLineIndices, qi = 0; i < maxi; i++, qi++) {
        gLineIndicies[i] = gNumLineVerts + indices[qi];
    }

    gNumLineIndices = maxi;
    maxi = gNumLineVerts + numVerts;

    if (maxi >= MAX_LINE_VERTS) {
        DBprintf(DBML_USER, "iQueueLineList:max num verts reached:%d\n", maxi);
        return;
    }

    for (i = gNumLineVerts, qi = 0; i < maxi; i++, qi++) {
        if (mat) {
            xMat4x3ToWorld((xVec3*)&gLineVerts[i].x, (xMat4x3*)mat, (xVec3*)&verts[qi]);
        } else {
            xVec3Copy((xVec3*)&gLineVerts[i].x, (xVec3*)&verts[qi]);
        }

        xColorCopy((xColor*)&gLineVerts[i].r, color);
    }

    gNumLineVerts = maxi;
}

void iDrawEnd()
{
    if (xglobals->PlaybackMode || gScreenDump)
        return;
    if (gNumLineIndices < 2) {
        return;
    }

    RwIm3DTransform(gLineVerts, gNumLineVerts, (RwMatrix*)&g_I3, rwIM3D_VERTEXRGBA | rwIM3D_VERTEXXYZ | rwIM3D_ALLOPAQUE);
    RwIm3DRenderIndexedPrimitive(rwPRIMTYPELINELIST, gLineIndicies, gNumLineIndices);
    RwIm3DEnd();

    if (gNumShadedVerts < 3)
        return;

    S32 cullstate, ztest;
    RwRenderStateGet(rwRENDERSTATECULLMODE, &cullstate);
    RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &ztest);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);

    xRenderStateZModesSet(TRUE, TRUE);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    if (RwIm3DTransform(gShadedVerts, gNumShadedVerts, NULL, rwIM3D_VERTEXRGBA | rwIM3D_VERTEXXYZ)) {
        RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
    }
    RwIm3DEnd();
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)cullstate);
    xRenderStateZModesSet(TRUE, ztest);
}

void iDrawShadedQuad(xColor color, RwMatrix* mat, xVec3* v1, xVec3* v2, xVec3* v3, xVec3* v4, S32 cullmode)
{
    RwCamera* camera = globals.screen->icam;
    RwIm3DVertex* v3d;
    U32 maxi = gNumShadedVerts + 6;

    if (maxi >= MAX_SHADED_VERTS) {
        return;
    }

    v3d = &gShadedVerts[gNumShadedVerts];

    if (cullmode) {
        F32 eyedot;
        xVec3 normal, eyedelta, edge1, edge2;

        xVec3Sub(&edge1, v2, v1);
        xVec3Sub(&edge2, v3, v1);
        xVec3Cross(&normal, &edge1, &edge2);
        xVec3Normalize(&normal, &normal);

        RwFrame* frame = RwCameraGetFrame(camera);
        RwMatrix* cammat = RwFrameGetMatrix(frame);
        eyedelta.x = cammat->pos.x - v1->x;
        eyedelta.y = cammat->pos.y - v1->y;
        eyedelta.z = cammat->pos.z - v1->z;
        eyedot = xVec3Dot(&eyedelta, &normal);

        if (cullmode == IDRAW_CULL_BACK && eyedot < 0.0f)
            return;
        if (cullmode == IDRAW_CULL_FRONT && eyedot > 0.0f) {
            return;
        }
    }

    if (mat) {
        xVec3 pos;

        xMat4x3ToWorld(&pos, (xMat4x3*)mat, v1);
        v3d[0].x = pos.x;
        v3d[0].y = pos.y;
        v3d[0].z = pos.z;

        xMat4x3ToWorld(&pos, (xMat4x3*)mat, v2);
        v3d[1].x = pos.x;
        v3d[1].y = pos.y;
        v3d[1].z = pos.z;

        xMat4x3ToWorld(&pos, (xMat4x3*)mat, v4);
        v3d[2].x = pos.x;
        v3d[2].y = pos.y;
        v3d[2].z = pos.z;

        xMat4x3ToWorld(&pos, (xMat4x3*)mat, v2);
        v3d[3].x = pos.x;
        v3d[3].y = pos.y;
        v3d[3].z = pos.z;

        xMat4x3ToWorld(&pos, (xMat4x3*)mat, v4);
        v3d[4].x = pos.x;
        v3d[4].y = pos.y;
        v3d[4].z = pos.z;

        xMat4x3ToWorld(&pos, (xMat4x3*)mat, v3);
        v3d[5].x = pos.x;
        v3d[5].y = pos.y;
        v3d[5].z = pos.z;
    } else {
        v3d[0].x = v1->x;
        v3d[0].y = v1->y;
        v3d[0].z = v1->z;

        v3d[1].x = v2->x;
        v3d[1].y = v2->y;
        v3d[1].z = v2->z;

        v3d[2].x = v4->x;
        v3d[2].y = v4->y;
        v3d[2].z = v4->z;

        v3d[3].x = v2->x;
        v3d[3].y = v2->y;
        v3d[3].z = v2->z;

        v3d[4].x = v4->x;
        v3d[4].y = v4->y;
        v3d[4].z = v4->z;

        v3d[5].x = v3->x;
        v3d[5].y = v3->y;
        v3d[5].z = v3->z;
    }

    for (S32 i = 0; i < 6; i++) {
        v3d[i].r = color.r;
        v3d[i].g = color.r;
        v3d[i].g = color.b;
        v3d[i].a = color.a;
    }

    gNumShadedVerts += 6;
}
#else
void iDrawBegin() {}
void iDrawEnd() {}
#endif

struct GCmemXFB
{
    U8* base;
    U8* addr;
    U8* max;
};

static GCmemXFB xfb;
static void* xfbDisp;
static void* xfbCopy;

void iXFBinit()
{
    RwGameCubeGetXFBs(&xfbDisp, &xfbCopy);
}

void iXFBreset()
{
    iXFBinit();

    xfb.base = (U8*)xfbCopy;

    xfb.max = xfb.base + FB_XRES * FB_YRES * 2;
    xfb.base = (U8*)OSRoundUp32B(xfb.base);
    xfb.addr = xfb.base;
}

void* iXFBalloc(U32 size)
{
    size = OSRoundUp32B(size);
    if (xfb.addr + size > xfb.max)
        return (void*)0xBEEEEEEF;
    U8* addr = xfb.addr;
    xfb.addr += size;
    return addr;
}
