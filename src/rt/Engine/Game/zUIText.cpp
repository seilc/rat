#include "xCommon.h"
#include "zUIText.h"

#include "containers.h"
#include "xDebugTweak.h"
#include "xFont.h"
#include "xPad.h"
#include "xRegionSupport.h"
#include "xstransvc.h"
#include "xTextAsset.h"
#include "xutil.h"
#include "zEvent.h"
#include "zGlobals.h"

#include <new>
#include <stdio.h>

namespace
{
    bool isCJK;
    F32 HSCALE;
    F32 VSCALE;
} // namespace

void zUIText::DoInit()
{
    zUI::DoInit();
    if (linkCount)
        link = (const xLinkAsset*)(GetAsset() + 1);

    shadowColor = GetAsset()->shadowColor;

    cached = true;
    clearLayoutCache = false;
}

void zUIText::DoReset()
{
    zUI::DoReset();
    text = NULL;
    cached = true;
    clearLayoutCache = false;
    shadowColor = GetAsset()->shadowColor;
}

void zUIText::setup_textbox(xtextbox& tb, const zUITextAsset* asset) const
{
    tb.line_space = 0.0f;
    tb.tab_stop = 0.0f;
    tb.left_indent = tb.right_indent = 0.0f;

    tb.font.id = asset->font;
    tb.font.width = 1.0f / SCREEN_WIDTH * asset->fontSizeW;
    tb.font.height = 1.0f / SCREEN_HEIGHT * asset->fontSizeH;
    tb.font.space = 1.0f / SCREEN_WIDTH * asset->fontSpacingX - tb.font.width;
    tb.font.color = current.color;
    tb.font.shadowColor = shadowColor;
    tb.font.shadowOffsetX = asset->shadowOffsetX;
    tb.font.shadowOffsetY = asset->shadowOffsetY;

    tb.font.clip.assign(
        1.0f / SCREEN_WIDTH * current.x, 1.0f / SCREEN_HEIGHT * current.y,
        1.0f / SCREEN_WIDTH * current.width, 1.0f / SCREEN_HEIGHT * current.height);

    tb.bounds = tb.font.clip;

    tb.bounds.contract(
        1.0f / SCREEN_WIDTH * asset->textBoxInsetLeft, 1.0f / SCREEN_HEIGHT * asset->textBoxInsetTop,
        1.0f / SCREEN_WIDTH * asset->textBoxInsetRight, 1.0f / SCREEN_HEIGHT * asset->textBoxInsetBottom);

    tb.flags = 0;

    switch (asset->justifyX) {
    case 0:
        tb.flags |= 0x0;
        break;
    case 2:
        tb.flags |= 0x1;
        break;
    case 1:
    default:
        tb.flags |= 0x2;
        break;
    }

    switch (asset->justifyY) {
    case 0:
        tb.flags |= 0x0;
        break;
    case 2:
        tb.flags |= 0x4;
        break;
    case 1:
    default:
        tb.flags |= 0x8;
        break;
    }
}

void zUIText::DoRender() const
{
    if (current.width < 1.0f || current.height < 1.0f)
        return;

    if (!text) {
        text = xTextFindString(GetAsset()->textID, NULL);
        if (!text)
            text = "";
    }

    if (*text == '\0')
        return;

    const zUITextAsset* asset = GetAsset();

    xtextbox tb;
    setup_textbox(tb, asset);

    tb.set_text(text);

    F32 hscale = 1.0f, vscale = 1.0f;
    if (isCJK) {
        if (asset->textFlags & 0x1) {
            hscale = HSCALE;
            vscale = VSCALE;
        }
    }
    else if (asset->textFlags & 0x2) {
        F32 oldWidth = tb.bounds.w, oldHeight = tb.bounds.h;
        tb.bounds.w = 100000000.0f;
        tb.bounds.h = 100000000.0f;
        const xtextbox::layout& layout = tb.temp_layout(true);
        F32 actualWidth = 0.0f, actualHeight = 0.0f;
        layout.get_max_bounds(actualWidth, actualHeight);
        tb.bounds.w = oldWidth;
        tb.bounds.h = oldHeight;

        if (actualWidth != 0.0f && actualWidth > 0.95f * tb.bounds.w)
            hscale = 0.95f * tb.bounds.w / actualWidth;
        if (actualHeight != 0.0f && actualHeight > 0.95f * tb.bounds.h)
            vscale = 0.95f * tb.bounds.h / actualHeight;
    }

    tb.font.width *= hscale;
    if (hscale < 1.0f)
        tb.font.space = 1.0f / SCREEN_WIDTH * (asset->fontSpacingX * hscale) - tb.font.width;
    tb.font.height *= vscale;

    if (vscale != 1.0f) {
        tb.bounds.y -= tb.font.height * (vscale - 1.0f) / 2.0f;
        tb.font.clip.y -= tb.font.height * (vscale - 1.0f) / 2.0f;
    }

    if (isCJK)
        tb.font.space = 0.0f;

    if (tb.font.width <= 0.0f || tb.font.height <= 0.0f)
        return;

    if (clearLayoutCache) {
        clearLayoutCache = false;
        xtextbox::clear_layout_cache();
    }

    tb.render(cached);
    if (!cached)
        cached = true;
}

void zUIText::DoHandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    switch (toEvent) {
    case eEventSetText:
        text = xTextFindString(toParamWidgetID, NULL);
        if (!text)
            text = "";
        break;

    default:
        zUI::DoHandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
        break;
    }
}

void zUIText::DoApplyMotionFrame(const zUIMotionFrame* frame)
{
    zUI::DoApplyMotionFrame(frame);
    shadowColor.a = frame->color.a * GetAsset()->shadowColor.a / 255;
}

#if DEBUG || RELEASE
void zUIText::DoAddTweaks(const char* baseName)
{
    zUI::DoAddTweaks(baseName);

    zUITextAsset* asset = GetAsset();
    char buffer[128];

    static char* fontNames[] = { "Font 0", "Font 1", "Font 2", "Font 3" };
    static char* justifyXNames[] = { "Left", "Center", "Right" };
    static char* justifyYNames[] = { "Top", "Center", "Bottom" };

    sprintf(buffer, "%s|Font", baseName);
    xTWEAK_SELECT(buffer, &asset->font, (const char**)fontNames, NULL, ARRAY_LENGTH(fontNames), NULL, NULL, 0x2);
    sprintf(buffer, "%s|FontSizeW", baseName);
    xTWEAK(buffer, &asset->fontSizeW, 0, 255, NULL, NULL, 0x2);
    sprintf(buffer, "%s|FontSizeH", baseName);
    xTWEAK(buffer, &asset->fontSizeH, 0, 255, NULL, NULL, 0x2);
    sprintf(buffer, "%s|FontSpacingX", baseName);
    xTWEAK(buffer, &asset->fontSpacingX, 0, 255, NULL, NULL, 0x2);
    sprintf(buffer, "%s|FontSpacingY", baseName);
    xTWEAK(buffer, &asset->fontSpacingY, 0, 255, NULL, NULL, 0x2);
    sprintf(buffer, "%s|TextBoxInsetTop", baseName);
    xTWEAK(buffer, &asset->textBoxInsetTop, 0, 255, NULL, NULL, 0x2);
    sprintf(buffer, "%s|TextBoxInsetLeft", baseName);
    xTWEAK(buffer, &asset->textBoxInsetLeft, 0, 255, NULL, NULL, 0x2);
    sprintf(buffer, "%s|TextBoxInsetRight", baseName);
    xTWEAK(buffer, &asset->textBoxInsetRight, 0, 255, NULL, NULL, 0x2);
    sprintf(buffer, "%s|TextBoxInsetBottom", baseName);
    xTWEAK(buffer, &asset->textBoxInsetBottom, 0, 255, NULL, NULL, 0x2);
    sprintf(buffer, "%s|CJK Scale", baseName);
    xTWEAK_FLAG(buffer, &asset->textFlags, 0x1, NULL, NULL, 0x2);
    sprintf(buffer, "%s|Horizontal Justification", baseName);
    xTWEAK_SELECT(buffer, &asset->justifyX, (const char**)justifyXNames, NULL, ARRAY_LENGTH(justifyXNames), NULL, NULL, 0x2);
    sprintf(buffer, "%s|Vertical Justification", baseName);
    xTWEAK_SELECT(buffer, &asset->justifyY, (const char**)justifyYNames, NULL, ARRAY_LENGTH(justifyYNames), NULL, NULL, 0x2);
}
#endif

void zUIText_Init(xBase& data, xDynAsset& asset, size_t)
{
    zUIText_Init((zUIText*)&data, (zUITextAsset*)&asset);
}

void zUIText_Init(zUIText* text, zUITextAsset* asset)
{
    new (text) zUIText();

    if (xSTGetLocalizationEnum() == eRegionKR)
        if (asset->color == 0xFFFFC55F)
            asset->color = 0xFFDFDFDF;

    zUI_Init(text, asset);
}

void zUITextInitSystem()
{
    xRegion region = xSTGetLocalizationEnum();
    switch (region) {
    case eRegionCH:
    case eRegionTW:
        isCJK = true;
        HSCALE = 1.2f;
        VSCALE = 1.2f;
        break;

    case eRegionJP:
        isCJK = true;
        HSCALE = 1.15f;
        VSCALE = 1.15f;
        break;

    case eRegionKR:
        isCJK = true;
        HSCALE = 1.2f;
        VSCALE = 1.2f;
        break;

    default:
        isCJK = false;
        break;
    }

#if DEBUG || RELEASE
    xTWEAK("Widgets|UI|\001HSCALE", &HSCALE, 0.0f, 100.0f, NULL, NULL, 0);
    xTWEAK("Widgets|UI|\001VSCALE", &VSCALE, 0.0f, 100.0f, NULL, NULL, 0);
    xTWEAK_BOOL("Widgets|UI|\001Is CJK?", &isCJK, NULL, NULL, 0);
#endif
}
