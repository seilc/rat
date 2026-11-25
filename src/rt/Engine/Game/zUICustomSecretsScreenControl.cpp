#include "xCommon.h"
#include "zUICustomSecretsScreenControl.h"

#include "xEvent.h"
#include "xMath.h"
#include "xRenderState.h"
#include "zEvent.h"
#include "zMain.h"
#include "zScene.h"
#include "zUI.h"
#include "zUIAsset.h"

#include <rwcore.h>

zUICustomSecretsScreenControl::zUICustomSecretsScreenControl(zUI* ui) : zUICustom(ui), input(NULL)
{
}

void zUICustomSecretsScreenControl::Init()
{
}

void zUICustomSecretsScreenControl::Setup()
{
    xASSERT_L(23, ui->asset->customWidget != 0);
    input = zSceneFindObject(ui->asset->customWidget);
    xASSERT_L(25, input != NULL);
}

void zUICustomSecretsScreenControl::PostUpdate(F32 dt)
{
    zUICustom::PostUpdate(dt);

    S32 brightness = ui->current.brightness * 2 + 32;
    if (brightness > 255)
        brightness = 255;
    ui->current.brightness = brightness;
}

bool zUICustomSecretsScreenControl::PreRender()
{
    if (!ui->hdrPass) {
        return true;
    }

    F32 offsetX = ui->current.x, offsetY = ui->current.y, scaleX = ui->current.width / 256.0f, scaleY = ui->current.height / 256.0f;

    F32 glowBoxData[28][3] = {
        { 52.0f, 23.0f, 32.0f },
        { 92.0f, 23.0f, 32.0f },
        { 132.0f, 23.0f, 32.0f },
        { 172.0f, 23.0f, 32.0f },
        { 32.0f, 58.0f, 32.0f },
        { 72.0f, 58.0f, 32.0f },
        { 112.0f, 58.0f, 32.0f },
        { 152.0f, 58.0f, 32.0f },
        { 192.0f, 58.0f, 32.0f },
        { 12.0f, 93.0f, 32.0f },
        { 52.0f, 93.0f, 32.0f },
        { 92.0f, 93.0f, 32.0f },
        { 132.0f, 93.0f, 32.0f },
        { 172.0f, 93.0f, 32.0f },
        { 212.0f, 93.0f, 32.0f },
        { 12.0f, 128.5f, 32.0f },
        { 52.0f, 128.5f, 32.0f },
        { 92.0f, 128.5f, 32.0f },
        { 132.0f, 128.5f, 32.0f },
        { 172.0f, 128.5f, 32.0f },
        { 212.0f, 128.5f, 32.0f },
        { 32.0f, 163.0f, 32.0f },
        { 72.0f, 163.0f, 32.0f },
        { 112.0f, 163.0f, 32.0f },
        { 152.0f, 163.0f, 32.0f },
        { 192.0f, 163.0f, 32.0f },
        { 51.5f, 198.5f, 64.0f },
        { 140.5f, 198.5f, 64.0f },
    };

    RwReal z = RwIm2DGetNearScreenZ();
    if (xabs(z) <= EPSILON) {
        z = (z >= 0.0f) ? EPSILON : -EPSILON;
    }

    RwReal zRecip = 1.0f / z;
    RwIm2DVertex vertices[4];

    F32 x1 = (offsetX + scaleX * glowBoxData[currFocus][0]) * FB_XRES * (1.0f / SCREEN_WIDTH);
    F32 y1 = (offsetY + scaleY * glowBoxData[currFocus][1]) * FB_YRES * (1.0f / SCREEN_HEIGHT);
    F32 x2 = x1 + scaleX * glowBoxData[currFocus][2] * FB_XRES * (1.0f / SCREEN_WIDTH);
    F32 y2 = y1 + 32.0f * scaleY * FB_YRES * (1.0f / SCREEN_HEIGHT);

    RwIm2DVertexSetScreenX(&vertices[0], x1);
    RwIm2DVertexSetScreenY(&vertices[0], y1);
    RwIm2DVertexSetScreenX(&vertices[1], x1);
    RwIm2DVertexSetScreenY(&vertices[1], y2);
    RwIm2DVertexSetScreenX(&vertices[2], x2);
    RwIm2DVertexSetScreenY(&vertices[2], y1);
    RwIm2DVertexSetScreenX(&vertices[3], x2);
    RwIm2DVertexSetScreenY(&vertices[3], y2);

    xprintf("ui alpha:  %d\n", ui->current.color.a);

    U8 alpha = 0;
    alpha = 255;
    for (S32 i = 0; i < 4; i++) {
        RwIm2DVertexSetScreenZ(&vertices[i], z);
        RwIm2DVertexSetRecipCameraZ(&vertices[i], zRecip);
        RwIm2DVertexSetIntRGBA(&vertices[i], 0, 0, 0, alpha);
    }

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);
    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, vertices, 4);
    return false;
}

bool zUICustomSecretsScreenControl::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    switch (toEvent) {
    case eEventPadPressUp:
        changeFocusY(-1);
        break;
    case eEventPadPressDown:
        changeFocusY(1);
        break;
    case eEventPadPressLeft:
        changeFocusX(-1);
        break;
    case eEventPadPressRight:
        changeFocusX(1);
        break;
    case eEventPadPressX:
        if (currFocus < 26) {
            F32 param = currFocus + 'A';
            zEntEvent(input, eEventUIAddChar, param, 0.0f, 0.0f, 0.0f);
        }
        else if (currFocus == 26) {
            zEntEvent(input, eEventUISendStringAsCheat, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        else if (currFocus == 27) {
            zEntEvent(input, eEventUIDelChar, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        break;
    case eEventUIReset:
        Reset();
        zEntEvent(input, eEventUIReset, 0.0f, 0.0f, 0.0f, 0.0f);
        break;
    }

    return zUICustom::HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}

void zUICustomSecretsScreenControl::Reset()
{
    currFocus = 0;
    currRow = 0;
    currCol = 0;
}

void zUICustomSecretsScreenControl::changeFocusX(S32 dir)
{
    S32 rows[6] = { 4, 5, 6, 6, 5, 2 };

    currCol += (S8)dir;

    if (currCol >= rows[currRow])
        currCol = 0;
    else if (currCol < 0)
        currCol = rows[currRow] - 1;

    currFocus = 0;
    S8 i = 0;
    while (i < currRow) {
        currFocus += (U16)rows[i];
        i++;
    }

    currFocus += currCol;
}

void zUICustomSecretsScreenControl::changeFocusY(S32 dir)
{
    S32 rows[6] = { 4, 5, 6, 6, 5, 2 };

    bool isSpecialCase = false;
    S32 oldRow = currRow;
    S32 oldCol = currCol;

    if (oldRow == 5) {
        isSpecialCase = true;
    }

    currRow += (S8)dir;

    if (currRow < 0)
        currRow = 5;
    else if (currRow > 5)
        currRow = 0;

    if (currRow == 5) {
        isSpecialCase = true;
    }

    if (isSpecialCase) {
        if (currRow == 5)
            (currCol < rows[oldRow] / 2) ? (currCol = 0) : (currCol = 1);
        else
            (oldCol == 0) ? (currCol = rows[currRow] / 2 - 1) : (currCol = rows[currRow] / 2 + 1);
    } else {
        while (currCol >= rows[currRow])
            currCol--;
    }

    currFocus = 0;
    S8 i = 0;
    while (i < currRow) {
        currFocus += (U16)rows[i];
        i++;
    }

    currFocus += currCol;
}
