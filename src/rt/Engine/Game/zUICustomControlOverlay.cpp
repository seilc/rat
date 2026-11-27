#include "xCommon.h"
#include "zUICustomControlOverlay.h"

#include "xFile.h"
#include "xMath.h"
#include "xRenderState.h"
#include "xString.h"
#include "xTextAsset.h"
#include "zEvent.h"
#include "zMain.h"
#include "zScene.h"
#include "zUI.h"
#include "zUICustomOptionsManager.h"
#include "zUIText.h"

struct zUICustomControlOverlayLineList
{
    F32 x1;
    F32 y1;
    F32 x2;
    F32 y2;
    F32 x3;
    F32 y3;
};

zUICustomControlOverlay::zUICustomControlOverlay(zUI* ui) : zUICustom(ui)
{
}

void zUICustomControlOverlay::Init()
{
    controlCount = 11;
    currPlayer = false;
}

void zUICustomControlOverlay::UpdatePlayerText()
{
    zUIText* text = (zUIText*)zSceneFindObject(xStrHash("UI PLY LSTICK TEXT"));
    text->SetText(xTextFindString(xStrHash("MRI TXT LEFTSTICK"), NULL));

    text = (zUIText*)zSceneFindObject(xStrHash("UI PLY RIGHTSTICK TEXT"));
    xASSERT_L(34, text);
    text->SetText(xTextFindString(xStrHash("MRI TXT RIGHTSTICK"), NULL));

    text = (zUIText*)zSceneFindObject(xStrHash("UI PLY LTRIGGER TEXT"));
    xASSERT_L(38, text);
    text->SetText(xTextFindString(xStrHash("MRI TXT LTRIGGER"), NULL));

    text = (zUIText*)zSceneFindObject(xStrHash("UI PLY RTRIGGER TEXT"));
    xASSERT_L(42, text);
    text->SetText(xTextFindString(xStrHash("MRI TXT RTRIGGER"), NULL));

    text = (zUIText*)zSceneFindObject(xStrHash("UI PLY START TEXT"));
    xASSERT_L(46, text);
    text->SetText(xTextFindString(xStrHash("MRI TXT START"), NULL));

    text = (zUIText*)zSceneFindObject(xStrHash("UI PLY Y TEXT"));
    xASSERT_L(50, text);
    text->SetText(xTextFindString(xStrHash("MRI TXT Y"), NULL));

    text = (zUIText*)zSceneFindObject(xStrHash("UI PLY X TEXT"));
    xASSERT_L(54, text);
    text->SetText(xTextFindString(xStrHash("MRI TXT X"), NULL));

    text = (zUIText*)zSceneFindObject(xStrHash("UI PLY A TEXT"));
    xASSERT_L(58, text);
    text->SetText(xTextFindString(xStrHash("MRI TXT A"), NULL));

    text = (zUIText*)zSceneFindObject(xStrHash("UI PLY B TEXT"));
    xASSERT_L(62, text);
    if (currPlayer)
        text->SetText(xTextFindString(xStrHash("FRO TXT B"), NULL));
    else
        text->SetText(xTextFindString(xStrHash("MRI TXT B"), NULL));

    text = (zUIText*)zSceneFindObject(xStrHash("UI PLY WHITE TEXT"));
    xASSERT_L(69, text);
    text->SetText(xTextFindString(xStrHash("MRI TXT WHITE"), NULL));

    text = (zUIText*)zSceneFindObject(xStrHash("UI MAIN TITLE TEXT"));
    xASSERT_L(73, text);
    if (currPlayer)
        text->SetText(xTextFindString(xStrHash("UI TXT OPTIONS-CONTROLS TITLE FRO"), NULL));
    else
        text->SetText(xTextFindString(xStrHash("UI TXT OPTIONS-CONTROLS TITLE MRI"), NULL));
}

void zUICustomControlOverlay::PreUpdate(F32)
{
    UpdatePlayerText();
}

// UNUSED: needed to match (speculative code)
static void DrawGrid()
{
    RwIm2DVertex vertex[2];

    RwReal z = RwIm2DGetNearScreenZ();
    if (xabs(z) <= EPSILON)
        z = (z >= 0.0f) ? EPSILON : -EPSILON;

    RwIm2DVertexSetScreenZ(&vertex[0], z);
    RwIm2DVertexSetRecipCameraZ(&vertex[0], 1.0f / z);
    RwIm2DVertexSetIntRGBA(&vertex[0], 255, 255, 255, 255);
    RwIm2DVertexSetScreenZ(&vertex[1], z);
    RwIm2DVertexSetRecipCameraZ(&vertex[1], 1.0f / z);
    RwIm2DVertexSetIntRGBA(&vertex[1], 255, 255, 255, 255);

    RwIm2DVertexSetScreenY(&vertex[0], 0.0f);
    RwIm2DVertexSetScreenY(&vertex[1], FB_YRES);

    F32 i = 1.0f;
    U32 count;
    for (count = 0; count < 64; count++) {
        RwIm2DVertexSetScreenX(&vertex[0], i * FB_XRES * (1.0f / SCREEN_WIDTH));
        RwIm2DVertexSetScreenX(&vertex[1], i * FB_XRES * (1.0f / SCREEN_WIDTH));
        RwIm2DRenderLine(vertex, 2, 0, 1);
        i += 10.0f;
    }

    RwIm2DVertexSetScreenX(&vertex[0], 0.0f);
    RwIm2DVertexSetScreenX(&vertex[1], FB_XRES);

    i = 1.0f;
    for (count = 0; count < 48; count++) {
        RwIm2DVertexSetScreenY(&vertex[0], i * FB_YRES * (1.0f / SCREEN_HEIGHT));
        RwIm2DVertexSetScreenY(&vertex[1], i * FB_YRES * (1.0f / SCREEN_HEIGHT));
        RwIm2DRenderLine(vertex, 2, 0, 1);
        i += 10.0f;
    }
}

bool zUICustomControlOverlay::PreRender()
{
    RenderLines();
    return false;
}

void zUICustomControlOverlay::RenderLines()
{
    RwIm2DVertex vertices[88];
    RwImVertexIndex indices[132];
    zUICustomControlOverlayLineList lineList[11] = {
        { 130.0f, 210.0f, 130.0f, 120.0f, 385.0f, 120.0f },
        { -1.0f, -1.0f, 260.0f, 260.0f, 385.0f, 260.0f },
        { 120.0f, 158.0f, 120.0f, 150.0f, 385.0f, 150.0f },
        { -1.0f, -1.0f, 290.0f, 158.0f, 385.0f, 158.0f },
        { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f },
        { 280.0f, 190.0f, 280.0f, 190.0f, 385.0f, 190.0f },
        { 250.0f, 230.0f, 250.0f, 295.0f, 385.0f, 295.0f },
        { 280.0f, 220.0f, 280.0f, 330.0f, 385.0f, 330.0f },
        { 312.0f, 215.0f, 312.0f, 225.0f, 385.0f, 225.0f },
        { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f },
        { 150.0f, 260.0f, 280.0f, 365.0f, 385.0f, 365.0f },
    };

    F32 lineHalfWidth = 1.5f;
    S32 vertexCount = 0;
    S32 indexCount = 0;

    RwReal z = RwIm2DGetNearScreenZ();
    if (xabs(z) <= EPSILON)
        z = (z >= 0.0f) ? EPSILON : -EPSILON;

    RwReal zRecip = 1.0f / z;
    F32 scaleX = ui->current.width / FB_XRES;
    F32 scaleY = ui->current.height / FB_YRES;

    for (U32 i = 0; i < controlCount; i++) {
        if (lineList[i].x2 == -1.0f || lineList[i].y2 == -1.0f)
            continue;

        if (lineList[i].x1 != -1.0f && lineList[i].y1 != -1.0f) {
            xVec3 p1, p2, dir;
            p1.x = lineList[i].x1 * FB_XRES * (1.0f / SCREEN_WIDTH);
            p1.y = lineList[i].y1 * FB_YRES * (1.0f / SCREEN_HEIGHT);
            p1.z = 0.0f;
            p2.x = lineList[i].x2 * FB_XRES * (1.0f / SCREEN_WIDTH);
            p2.y = lineList[i].y2 * FB_YRES * (1.0f / SCREEN_HEIGHT);
            p2.z = 0.0f;

            xVec3Sub(&dir, &p2, &p1);
            xVec3Normalize(&dir, &dir);
            F32 dot = xVec3Dot(&p1, &dir);
            xVec3SMulBy(&dir, dot);
            xVec3Sub(&dir, &p1, &dir);
            xVec3Normalize(&dir, &dir);
            xVec3SMulBy(&dir, lineHalfWidth);

            S32 oldCount = vertexCount;

            RwIm2DVertexSetScreenX(&vertices[vertexCount], p1.x - dir.x);
            RwIm2DVertexSetScreenY(&vertices[vertexCount], p1.y - dir.y);
            RwIm2DVertexSetScreenZ(&vertices[vertexCount], z);
            RwIm2DVertexSetRecipCameraZ(&vertices[vertexCount], zRecip);
            RwIm2DVertexSetIntRGBA(&vertices[vertexCount], 204, 127, 30, 255);
            vertexCount++;

            RwIm2DVertexSetScreenX(&vertices[vertexCount], p1.x + dir.x);
            RwIm2DVertexSetScreenY(&vertices[vertexCount], p1.y + dir.y);
            RwIm2DVertexSetScreenZ(&vertices[vertexCount], z);
            RwIm2DVertexSetRecipCameraZ(&vertices[vertexCount], zRecip);
            RwIm2DVertexSetIntRGBA(&vertices[vertexCount], 204, 127, 30, 255);
            vertexCount++;

            RwIm2DVertexSetScreenX(&vertices[vertexCount], p2.x + dir.x);
            RwIm2DVertexSetScreenY(&vertices[vertexCount], p2.y + dir.y);
            RwIm2DVertexSetScreenZ(&vertices[vertexCount], z);
            RwIm2DVertexSetRecipCameraZ(&vertices[vertexCount], zRecip);
            RwIm2DVertexSetIntRGBA(&vertices[vertexCount], 204, 127, 30, 255);
            vertexCount++;

            RwIm2DVertexSetScreenX(&vertices[vertexCount], p2.x - dir.x);
            RwIm2DVertexSetScreenY(&vertices[vertexCount], p2.y - dir.y);
            RwIm2DVertexSetScreenZ(&vertices[vertexCount], z);
            RwIm2DVertexSetRecipCameraZ(&vertices[vertexCount], zRecip);
            RwIm2DVertexSetIntRGBA(&vertices[vertexCount], 204, 127, 30, 255);
            vertexCount++;

            indices[indexCount++] = oldCount;
            indices[indexCount++] = oldCount + 1;
            indices[indexCount++] = oldCount + 2;
            indices[indexCount++] = oldCount + 2;
            indices[indexCount++] = oldCount + 3;
            indices[indexCount++] = oldCount;
        }

        if (lineList[i].x3 != -1.0f && lineList[i].y3 != -1.0f) {
            xVec3 p1, p2, dir;
            p1.x = lineList[i].x3 * FB_XRES * (1.0f / SCREEN_WIDTH);
            p1.y = lineList[i].y3 * FB_YRES * (1.0f / SCREEN_HEIGHT);
            p1.z = 0.0f;
            p2.x = lineList[i].x2 * FB_XRES * (1.0f / SCREEN_WIDTH);
            p2.y = lineList[i].y2 * FB_YRES * (1.0f / SCREEN_HEIGHT);
            p2.z = 0.0f;

            xVec3Sub(&dir, &p2, &p1);
            xVec3Normalize(&dir, &dir);
            F32 dot = xVec3Dot(&p1, &dir);
            xVec3SMulBy(&dir, dot);
            xVec3Sub(&dir, &p1, &dir);
            xVec3Normalize(&dir, &dir);
            xVec3SMulBy(&dir, lineHalfWidth);

            S32 oldCount = vertexCount;

            RwIm2DVertexSetScreenX(&vertices[vertexCount], p1.x - dir.x);
            RwIm2DVertexSetScreenY(&vertices[vertexCount], p1.y - dir.y);
            RwIm2DVertexSetScreenZ(&vertices[vertexCount], z);
            RwIm2DVertexSetRecipCameraZ(&vertices[vertexCount], zRecip);
            RwIm2DVertexSetIntRGBA(&vertices[vertexCount], 204, 127, 30, 255);
            vertexCount++;

            RwIm2DVertexSetScreenX(&vertices[vertexCount], p1.x + dir.x);
            RwIm2DVertexSetScreenY(&vertices[vertexCount], p1.y + dir.y);
            RwIm2DVertexSetScreenZ(&vertices[vertexCount], z);
            RwIm2DVertexSetRecipCameraZ(&vertices[vertexCount], zRecip);
            RwIm2DVertexSetIntRGBA(&vertices[vertexCount], 204, 127, 30, 255);
            vertexCount++;

            RwIm2DVertexSetScreenX(&vertices[vertexCount], p2.x + dir.x);
            RwIm2DVertexSetScreenY(&vertices[vertexCount], p2.y + dir.y);
            RwIm2DVertexSetScreenZ(&vertices[vertexCount], z);
            RwIm2DVertexSetRecipCameraZ(&vertices[vertexCount], zRecip);
            RwIm2DVertexSetIntRGBA(&vertices[vertexCount], 204, 127, 30, 255);
            vertexCount++;

            RwIm2DVertexSetScreenX(&vertices[vertexCount], p2.x - dir.x);
            RwIm2DVertexSetScreenY(&vertices[vertexCount], p2.y - dir.y);
            RwIm2DVertexSetScreenZ(&vertices[vertexCount], z);
            RwIm2DVertexSetRecipCameraZ(&vertices[vertexCount], zRecip);
            RwIm2DVertexSetIntRGBA(&vertices[vertexCount], 204, 127, 30, 255);
            vertexCount++;

            indices[indexCount++] = oldCount;
            indices[indexCount++] = oldCount + 1;
            indices[indexCount++] = oldCount + 2;
            indices[indexCount++] = oldCount + 2;
            indices[indexCount++] = oldCount + 3;
            indices[indexCount++] = oldCount;
        }
    }

    RwShadeMode oldShadeMode;
    RwBool oldAlphaEnable;
    RwRenderStateGet(rwRENDERSTATESHADEMODE, &oldShadeMode);
    RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &oldAlphaEnable);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEFLAT);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);

    RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, vertices, vertexCount, indices, indexCount);

    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)oldShadeMode);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)oldAlphaEnable);
}

bool zUICustomControlOverlay::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    switch (toEvent) {
    case eEventVisible:
        inOverlayScreen = true;
        break;

    case eEventInvisible:
        inOverlayScreen = false;
        inOverlayScreenExitTimer = 0.4f;
        break;

    case eEventPadPressX:
        currPlayer = !currPlayer;
        break;

    default:
        break;
    }

    return zUICustom::HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}
