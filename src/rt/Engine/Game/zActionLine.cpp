#include "xCommon.h"
#include "zActionLine.h"

#include "xRenderState.h"
#include "xstransvc.h"
#include "zHashes.h"

#include "decomp.h"

#include <rwcore.h>

#define MAX_ACTION_LINES 8

typedef struct _tagActionLine
{
    U32 flags;
    xVec3 pos[4];
    F32 time_left;
} ActionLine;

// ActionLine::flags
#define k_ACTIONLINE_ENABLED 0x1

static ActionLine* sActionLine[MAX_ACTION_LINES];
static RwRaster* sActionLineRaster;

void zActionLineInit()
{
    for (S32 i = 0; i < MAX_ACTION_LINES; i++) {
        sActionLine[i] = NULL;
    }

    sActionLineRaster = NULL;
    RwTexture* tex = (RwTexture*)xSTFindAsset(HASH_ACTIONLINES, NULL);
    if (tex) {
        sActionLineRaster = RwTextureGetRaster(tex);
    }
}

static ActionLine* FindFreeActionLine(); // UNUSED

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE(
    "zActionLine.cpp",
    "sActionLine[ i ]",
    "%s",
    "FindFreeActionLine, oh dear we have run out of available lightning slots!!\n",
    "*always*",
    "start",
    "mat")
#endif

void zActionLineUpdate(F32 seconds)
{
    for (S32 i = 0; i < MAX_ACTION_LINES; i++) {
        ActionLine* l = sActionLine[i];
        if (l && (l->flags & k_ACTIONLINE_ENABLED)) {
            l->time_left -= seconds;
            if (l->time_left <= 0.0f) {
                l->flags &= ~k_ACTIONLINE_ENABLED;
            }
        }
    }
}

#if DEBUG
DECOMP_FORCEACTIVE(
    "X to test streak effect\n",
    "               ",
    "1",
    "0",
    "-",
    "\n",
    "xEnt.h",
    "ent->model",
    "xModel.h",
    "model",
    "model->Mat",
    "zPlayerContainer.h",
    "index >= 0 && index < numPlayers")
#endif

static void RenderActionLine(ActionLine* l)
{
    static RwIm3DVertex sStripVert[4];

    xASSERT_L(185, l);

    RwIm3DVertex* vert = sStripVert;

    for (S32 i = 0; i < 4; i++) {
        RwIm3DVertexSetPos(vert, l->pos[i].x, l->pos[i].y, l->pos[i].z);
        RwIm3DVertexSetU(vert, 0.0f);
        RwIm3DVertexSetV(vert, 0.0f);
        RwIm3DVertexSetRGBA(vert, 255, 255, 255, 128);
        vert++;
    }

    sStripVert[2].u = 1.0f;
    sStripVert[3].u = 1.0f;
    sStripVert[3].v = 1.0f;

    if (RwIm3DTransform(sStripVert, 4, NULL, rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA)) {
        RwIm3DRenderPrimitive(rwPRIMTYPETRISTRIP);
        RwIm3DEnd();
    }
}

void zActionLineRender()
{
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, sActionLineRaster);

    for (S32 i = 0; i < MAX_ACTION_LINES; i++) {
        if (sActionLine[i] && (sActionLine[i]->flags & k_ACTIONLINE_ENABLED)) {
            RenderActionLine(sActionLine[i]);
        }
    }
}
