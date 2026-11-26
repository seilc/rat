#include "xCommon.h"
#include "zUICustomWireframe.h"

#include "iMath.h"
#include "xFile.h"
#include "xMath.h"
#include "xRenderState.h"
#include "xString.h"
#include "xpkrsvc.h"
#include "xstransvc.h"
#include "zMain.h"
#include "zUI.h"
#include "zWireFrameAsset.h"

#include <string.h>

xTWEAKABLE F32 WIREFRAME_ROTATE_SPEED = 25.0f;
xTWEAKABLE F32 WIREFRAME_UPDATE_RATE = 20.0f;
U32 loadSceneID;

zUICustomWireframe::zUICustomWireframe(zUI* ui) : zUICustom(ui)
{
    wireframeAsset = NULL;
    vertices = NULL;
}

void zUICustomWireframe::Init()
{
    char wireframeName[1024], sceneName[5];
    strcpy(wireframeName, "UI WIREFRAME ");

    char* ID = (char*)&loadSceneID;
    sceneName[0] = ID[0];
    sceneName[1] = ID[1];
    sceneName[2] = ID[2];
    sceneName[3] = ID[3];
    sceneName[4] = '\0';
    strcat(wireframeName, sceneName);

    rotation = 0.0f;
    vertices = NULL;

    PKRAssetTOCInfo info;
    memset(&info, 0, sizeof(info));
    S32 found = xSTGetAssetInfo(xStrHash(wireframeName), &info);
    if (!found) {
        wireframeAsset = NULL;
        vertices = NULL;
        return;
    }

    U32 size = (info.size + 31) & ~31;
    wireframeAsset = (zWireframeAsset*)zUIAllocStaticMemory(size);
    const char* hipFilename = xST_xAssetID_HIPFullPath(info.aid);
    xASSERT_L(66, hipFilename);

    bool error = false;
    if (hipFilename) {
        xFile hipFile;
        S32 err = iFileOpen(hipFilename, IFILE_READ, &hipFile);
        if (err == 0) {
            err = iFileSeek(&hipFile, info.plus_offset + info.sector * iFileGetSectorSize(XFILE_DRIVE_CDVD), 0);
            if (err > -1) {
                err = iFileRead(&hipFile, wireframeAsset, size);
                if (err) {
                    err = iFileClose(&hipFile);
                    if (err)
                        error = true;
                } else
                    error = true;
            } else
                error = true;
        } else
            error = true;
    } else
        error = true;

    if (error) {
        wireframeAsset = NULL;
        vertices = NULL;
        return;
    }

    vertices = (RwIm2DVertex*)zUIAllocStaticMemory(wireframeAsset->vertexCount * sizeof(RwIm2DVertex));

    WireframeAssetSetupPointers(wireframeAsset);

    F32 dx = 0.0f, dy = 0.0f;
    for (U32 i = 0; i < wireframeAsset->vertexCount; i++) {
        F32 dist = xsqrt(xsqr(wireframeAsset->vertices[i].x) + xsqr(wireframeAsset->vertices[i].z));
        if (dist > dx)
            dx = dist;

        if (xabs(wireframeAsset->vertices[i].y) > dy)
            dy = xabs(wireframeAsset->vertices[i].y);
    }

    dX = 2.0f * dx;
    dY = 2.0f * dy;

    dY *= 1.333f;

    xASSERT_L(128, dX != 0.0f);
    xASSERT_L(129, dY != 0.0f);
}

bool zUICustomWireframe::PreRender()
{
    if (!wireframeAsset)
        return false;

    F32 scaleX = ui->current.width / dX;
    F32 scaleY = ui->current.height / dY;

    scale = MIN(scaleX, scaleY);

    RwReal z = RwIm2DGetNearScreenZ();
    if (xabs(z) <= EPSILON)
        z = (z >= 0.0f) ? EPSILON : -EPSILON;

    RwReal zRecip = 1.0f / z;
    zWireframeVertex* assetVertices = wireframeAsset->vertices;
    RwImVertexIndex* indices = (RwImVertexIndex*)wireframeAsset->lines;

    const F32 WIREFRAME_ROTATE_UNIT = WIREFRAME_ROTATE_SPEED / WIREFRAME_UPDATE_RATE;
    S32 rotationIndex = rotation / WIREFRAME_ROTATE_UNIT;
    F32 phi = rotationIndex * WIREFRAME_ROTATE_UNIT * (PI / 180.0f);
    F32 sinPhi = isin(phi);
    F32 cosPhi = icos(phi);

    F32 sinTheta = isin(20.0f * (PI / 180.0f));
    F32 cosTheta = icos(20.0f * (PI / 180.0f));

    for (U32 i = 0; i < wireframeAsset->vertexCount; i++) {
        xVec3 currVert;
        currVert.x = assetVertices[i].x;
        currVert.y = assetVertices[i].y;
        currVert.z = assetVertices[i].z;

        F32 newX, newY, newZ;
        newX = cosPhi * currVert.x + sinPhi * currVert.z;
        newZ = -sinPhi * currVert.x + cosPhi * currVert.z;
        currVert.x = newX;
        currVert.z = newZ;

        newY = cosTheta * currVert.y - sinTheta * currVert.z;
        newZ = sinTheta * currVert.y + cosTheta * currVert.z;
        currVert.y = newY;
        currVert.z = newZ;

        RwIm2DVertexSetScreenX(&vertices[i], (ui->current.x + currVert.x * scale + ui->current.width / 2.0f) * FB_XRES * (1.0f / SCREEN_WIDTH));
        RwIm2DVertexSetScreenY(&vertices[i], (ui->current.y + currVert.y * -scale + ui->current.height / 2.0f) * FB_YRES * (1.0f / SCREEN_HEIGHT));
        RwIm2DVertexSetScreenZ(&vertices[i], 1.0f);
        RwIm2DVertexSetRecipCameraZ(&vertices[i], zRecip);
        RwIm2DVertexSetIntRGBA(&vertices[i], 255, 255, 255, 255);
    }

    RwShadeMode oldShadeMode;
    RwBool oldAlphaEnable;
    RwCullMode oldCullMode;
    RwRenderStateGet(rwRENDERSTATESHADEMODE, &oldShadeMode);
    RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &oldAlphaEnable);
    RwRenderStateGet(rwRENDERSTATECULLMODE, &oldCullMode);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEFLAT);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);

    xRenderStateZModesSet(FALSE, FALSE);

    RwIm2DRenderIndexedPrimitive(rwPRIMTYPELINELIST, vertices, wireframeAsset->vertexCount, indices, wireframeAsset->lineCount * 2);

    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)oldShadeMode);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)oldAlphaEnable);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)oldCullMode);

    return false;
}

void zUICustomWireframe::PreUpdate(F32 dt)
{
    if (wireframeAsset) {
        rotation += dt * WIREFRAME_ROTATE_SPEED;
        if (rotation > 360.0f)
            rotation -= 360.0f * (S32)(rotation / 360.0f);
    }
}

void zUICustomWireframe::Exit()
{
}
